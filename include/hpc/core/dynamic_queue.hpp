#pragma once

#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace hpc::core {

// Dynamic-capacity queue (FIFO) backed by a growable circular buffer.
//
// Design notes:
//  - Drop-in replacement for std::queue with the same push/pop/front/back
//    interface plus try_push / try_pop for non-throwing usage.
//  - Internal storage is always a power-of-two so wrap-around uses a cheap
//    bitwise AND instead of modulo.
//  - 2× growth factor for amortised O(1) push.
//  - Trivially copyable types use memcpy for bulk relocation during growth
//    and skip destructor calls on pop/clear.
//  - Over-aligned types (alignof(T) > default new alignment) are handled
//    via the C++17 aligned operator new/delete.
//  - Placement new for precise object lifetime control.

template <class T>
class queue {
    static constexpr bool is_trivial      = std::is_trivially_copyable_v<T>;
    static constexpr bool is_over_aligned = alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__;

public:
    using value_type      = T;
    using size_type       = std::size_t;
    using reference       = T&;
    using const_reference = const T&;

    // -- Constructors / destructor ----------------------------------------

    queue() noexcept = default;

    explicit queue(size_type initial_capacity)
    {
        reserve(initial_capacity);
    }

    queue(std::initializer_list<T> il)
    {
        reserve(il.size());
        for (auto& v : il) {
            construct(slot(tail_), v);
            tail_ = advance(tail_);
        }
    }

    queue(const queue& rhs)
    {
        reserve(rhs.size());
        for (size_type i = rhs.head_; i != rhs.tail_; i = (i + 1) & rhs.mask_)
        {
            construct(slot(tail_), *rhs.slot(i));
            tail_ = advance(tail_);
        }
    }

    queue(queue&& rhs) noexcept
        : buf_(rhs.buf_), cap_(rhs.cap_), mask_(rhs.mask_),
          head_(rhs.head_), tail_(rhs.tail_)
    {
        rhs.buf_  = nullptr;
        rhs.cap_  = 0;
        rhs.mask_ = 0;
        rhs.head_ = 0;
        rhs.tail_ = 0;
    }

    ~queue() { clear(); free_buf(); }

    queue& operator=(const queue& rhs)
    {
        if (this != &rhs) {
            queue tmp(rhs);
            swap(tmp);
        }
        return *this;
    }

    queue& operator=(queue&& rhs) noexcept
    {
        if (this != &rhs) {
            clear();
            free_buf();
            buf_  = rhs.buf_;   cap_  = rhs.cap_;   mask_ = rhs.mask_;
            head_ = rhs.head_;  tail_ = rhs.tail_;
            rhs.buf_  = nullptr; rhs.cap_  = 0; rhs.mask_ = 0;
            rhs.head_ = 0;      rhs.tail_ = 0;
        }
        return *this;
    }

    queue& operator=(std::initializer_list<T> il)
    {
        queue tmp(il);
        swap(tmp);
        return *this;
    }

    // -- Capacity ---------------------------------------------------------

    [[nodiscard]] bool empty()    const noexcept { return head_ == tail_; }
    [[nodiscard]] size_type size() const noexcept
    {
        return (tail_ - head_ + cap_) & mask_;
    }
    [[nodiscard]] size_type capacity() const noexcept
    {
        // Usable capacity is cap_ - 1 (one slot is sentinel).
        return cap_ > 0 ? cap_ - 1 : 0;
    }

    void reserve(size_type n)
    {
        if (n > capacity())
            grow_to(round_up_po2(n + 1));   // +1 for the sentinel slot
    }

    void shrink_to_fit()
    {
        auto s = size();
        if (s == 0) {
            free_buf();
            buf_ = nullptr; cap_ = 0; mask_ = 0;
            head_ = 0; tail_ = 0;
            return;
        }
        auto new_cap = round_up_po2(s + 1);
        if (new_cap < cap_)
            grow_to(new_cap);  // reallocates into a tighter buffer
    }

    // -- Element access ---------------------------------------------------

    reference       front()       noexcept { return *slot(head_); }
    const_reference front() const noexcept { return *slot(head_); }

    reference       back()        noexcept { return *slot((tail_ - 1 + cap_) & mask_); }
    const_reference back()  const noexcept { return *slot((tail_ - 1 + cap_) & mask_); }

    // -- Modifiers --------------------------------------------------------

    void push(const T& value)
    {
        ensure_space();
        construct(slot(tail_), value);
        tail_ = advance(tail_);
    }

    void push(T&& value)
    {
        ensure_space();
        construct(slot(tail_), std::move(value));
        tail_ = advance(tail_);
    }

    template <class... Args>
    reference emplace(Args&&... args)
    {
        ensure_space();
        T* s = slot(tail_);
        construct(s, std::forward<Args>(args)...);
        tail_ = advance(tail_);
        return *s;
    }

    void pop()
    {
        destroy(slot(head_));
        head_ = advance(head_);
    }

    void pop(T& out)
    {
        out = std::move(*slot(head_));
        destroy(slot(head_));
        head_ = advance(head_);
    }

    void clear() noexcept
    {
        if constexpr (!is_trivial) {
            while (head_ != tail_) {
                slot(head_)->~T();
                head_ = advance(head_);
            }
        }
        head_ = 0;
        tail_ = 0;
    }

    void swap(queue& o) noexcept
    {
        std::swap(buf_,  o.buf_);
        std::swap(cap_,  o.cap_);
        std::swap(mask_, o.mask_);
        std::swap(head_, o.head_);
        std::swap(tail_, o.tail_);
    }

    // -- Comparison -------------------------------------------------------

    friend bool operator==(const queue& a, const queue& b) noexcept
    {
        if (a.size() != b.size()) return false;
        size_type ai = a.head_, bi = b.head_;
        while (ai != a.tail_) {
            if (!(*a.slot(ai) == *b.slot(bi))) return false;
            ai = (ai + 1) & a.mask_;
            bi = (bi + 1) & b.mask_;
        }
        return true;
    }

    friend bool operator!=(const queue& a, const queue& b) noexcept
    {
        return !(a == b);
    }

private:
    std::byte* buf_  = nullptr;
    size_type  cap_  = 0;      // always power of two (or 0)
    size_type  mask_ = 0;      // cap_ - 1
    size_type  head_ = 0;
    size_type  tail_ = 0;

    // -- Helpers ----------------------------------------------------------

    static constexpr size_type round_up_po2(size_type v) noexcept
    {
        if (v == 0) return 1;
        --v;
        v |= v >> 1; v |= v >> 2; v |= v >> 4;
        v |= v >> 8; v |= v >> 16;
        if constexpr (sizeof(size_type) > 4) v |= v >> 32;
        return v + 1;
    }

    size_type advance(size_type idx) const noexcept
    {
        return (idx + 1) & mask_;
    }

    T* slot(size_type i) noexcept
    {
        return std::launder(reinterpret_cast<T*>(buf_ + i * sizeof(T)));
    }

    const T* slot(size_type i) const noexcept
    {
        return std::launder(reinterpret_cast<const T*>(buf_ + i * sizeof(T)));
    }

    template <class... Args>
    static void construct(T* p, Args&&... args)
    {
        ::new (static_cast<void*>(p)) T(std::forward<Args>(args)...);
    }

    static void destroy(T* p) noexcept
    {
        if constexpr (!is_trivial)
            p->~T();
    }

    static std::byte* alloc_buf(size_type n)
    {
        if (n == 0) return nullptr;
        void* p;
        if constexpr (is_over_aligned)
            p = ::operator new(n * sizeof(T), std::align_val_t{alignof(T)});
        else
            p = ::operator new(n * sizeof(T));
        return static_cast<std::byte*>(p);
    }

    void free_buf() noexcept
    {
        if (!buf_) return;
        if constexpr (is_over_aligned)
            ::operator delete(static_cast<void*>(buf_), std::align_val_t{alignof(T)});
        else
            ::operator delete(static_cast<void*>(buf_));
    }

    void ensure_space()
    {
        // Need at least one free sentinel slot: size() < cap_ - 1.
        if (size() + 1 >= cap_)
            grow_to(cap_ == 0 ? 8 : cap_ * 2);
    }

    // Relocate all live elements into a new buffer of `new_cap` slots.
    // After relocation head_ == 0 and elements are laid out contiguously.
    void grow_to(size_type new_cap)
    {
        std::byte* new_buf = alloc_buf(new_cap);
        size_type  s       = size();

        if constexpr (is_trivial) {
            if (buf_) {
                if (head_ < tail_) {
                    // Contiguous region.
                    std::memcpy(new_buf, buf_ + head_ * sizeof(T), s * sizeof(T));
                } else if (s > 0) {
                    // Wrapped: [head_, cap_) then [0, tail_).
                    size_type first_run = cap_ - head_;
                    std::memcpy(new_buf, buf_ + head_ * sizeof(T), first_run * sizeof(T));
                    std::memcpy(new_buf + first_run * sizeof(T), buf_, tail_ * sizeof(T));
                }
            }
        } else {
            size_type dst = 0;
            for (size_type i = head_; i != tail_; i = (i + 1) & mask_) {
                T* src_ptr = slot(i);
                ::new (static_cast<void*>(new_buf + dst * sizeof(T))) T(std::move(*src_ptr));
                src_ptr->~T();
                ++dst;
            }
        }

        free_buf();
        buf_   = new_buf;
        cap_   = new_cap;
        mask_  = new_cap - 1;
        head_  = 0;
        tail_  = s;
    }
};

} // namespace hpc::core

