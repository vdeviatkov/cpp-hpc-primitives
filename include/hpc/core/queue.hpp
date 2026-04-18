#pragma once

#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <hpc/support/cache_line.hpp>

namespace hpc::core {

// Fixed-capacity queue (FIFO) backed by a circular buffer.
//
// Design notes:
//  - Template capacity avoids heap allocation; the entire queue lives in
//    contiguous, cache-friendly memory (ideal for hot-path message passing).
//  - Internal storage is a power-of-two-sized circular buffer so wrap-around
//    uses a cheap bitwise AND instead of modulo.
//  - Storage is aligned to the maximum of alignof(T) and cache_line_size
//    to avoid false sharing when the queue sits next to other hot data.
//  - Trivially copyable types skip destructor calls on pop/clear for a
//    tighter inner loop.
//  - Placement new for precise object lifetime control.
//  - All mutators are noexcept where possible; try_push / try_pop never
//    throw and return a bool indicating success.

template <class T, std::size_t Capacity>
class fixed_queue {
    static_assert(Capacity > 0, "Capacity must be greater than zero");
    static_assert(std::is_nothrow_destructible_v<T>,
                  "T must be nothrow destructible");

    static constexpr bool is_trivial = std::is_trivially_copyable_v<T>;

    // Round up to next power of two for cheap masking.
    static constexpr std::size_t round_up_po2(std::size_t v) noexcept
    {
        --v;
        v |= v >> 1; v |= v >> 2; v |= v >> 4;
        v |= v >> 8; v |= v >> 16;
        if constexpr (sizeof(std::size_t) > 4) v |= v >> 32;
        return v + 1;
    }

    // Internal buffer size (power-of-two >= Capacity + 1 so that head==tail means empty).
    static constexpr std::size_t buf_cap  = round_up_po2(Capacity + 1);
    static constexpr std::size_t buf_mask = buf_cap - 1;

    static constexpr std::size_t storage_align =
        alignof(T) > hpc::support::cache_line_size
            ? alignof(T)
            : hpc::support::cache_line_size;

public:
    using value_type      = T;
    using size_type       = std::size_t;
    using reference       = T&;
    using const_reference = const T&;

    // -- Constructors / destructor ----------------------------------------

    fixed_queue() noexcept = default;

    fixed_queue(std::initializer_list<T> il)
    {
        for (auto& v : il) {
            if (size() == Capacity)
                throw std::overflow_error("hpc::core::fixed_queue: capacity exceeded");
            construct(slot(tail_), v);
            tail_ = advance(tail_);
        }
    }

    fixed_queue(const fixed_queue& rhs)
        noexcept(std::is_nothrow_copy_constructible_v<T>)
        : head_(0), tail_(0)
    {
        for (size_type i = rhs.head_; i != rhs.tail_; i = rhs.advance(i))
        {
            construct(slot(tail_), *rhs.slot(i));
            tail_ = advance(tail_);
        }
    }

    fixed_queue(fixed_queue&& rhs)
        noexcept(std::is_nothrow_move_constructible_v<T>)
        : head_(0), tail_(0)
    {
        for (size_type i = rhs.head_; i != rhs.tail_; i = rhs.advance(i))
        {
            construct(slot(tail_), std::move(*rhs.slot(i)));
            tail_ = advance(tail_);
        }
        rhs.clear();
    }

    ~fixed_queue() { clear(); }

    fixed_queue& operator=(const fixed_queue& rhs)
        noexcept(std::is_nothrow_copy_constructible_v<T>)
    {
        if (this != &rhs) {
            clear();
            for (size_type i = rhs.head_; i != rhs.tail_; i = rhs.advance(i))
            {
                construct(slot(tail_), *rhs.slot(i));
                tail_ = advance(tail_);
            }
        }
        return *this;
    }

    fixed_queue& operator=(fixed_queue&& rhs)
        noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        if (this != &rhs) {
            clear();
            for (size_type i = rhs.head_; i != rhs.tail_; i = rhs.advance(i))
            {
                construct(slot(tail_), std::move(*rhs.slot(i)));
                tail_ = advance(tail_);
            }
            rhs.clear();
        }
        return *this;
    }

    // -- Capacity ---------------------------------------------------------

    [[nodiscard]] bool      empty() const noexcept { return head_ == tail_; }
    [[nodiscard]] bool      full()  const noexcept { return size() == Capacity; }

    [[nodiscard]] size_type size() const noexcept
    {
        return (tail_ - head_ + buf_cap) & buf_mask;
    }

    static constexpr size_type capacity() noexcept { return Capacity; }

    // -- Element access ---------------------------------------------------

    reference       front()       noexcept { return *slot(head_); }
    const_reference front() const noexcept { return *slot(head_); }

    reference       back()        noexcept { return *slot((tail_ - 1 + buf_cap) & buf_mask); }
    const_reference back()  const noexcept { return *slot((tail_ - 1 + buf_cap) & buf_mask); }

    // -- Modifiers --------------------------------------------------------

    /// Push a copy. Returns false if the queue is full.
    [[nodiscard]] bool try_push(const T& value)
        noexcept(std::is_nothrow_copy_constructible_v<T>)
    {
        if (size() == Capacity) return false;
        construct(slot(tail_), value);
        tail_ = advance(tail_);
        return true;
    }

    /// Push by move. Returns false if the queue is full.
    [[nodiscard]] bool try_push(T&& value)
        noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        if (size() == Capacity) return false;
        construct(slot(tail_), std::move(value));
        tail_ = advance(tail_);
        return true;
    }

    /// Push a copy; throws std::overflow_error if full.
    void push(const T& value)
    {
        if (!try_push(value))
            throw std::overflow_error("hpc::core::fixed_queue::push: queue is full");
    }

    /// Push by move; throws std::overflow_error if full.
    void push(T&& value)
    {
        if (!try_push(std::move(value)))
            throw std::overflow_error("hpc::core::fixed_queue::push: queue is full");
    }

    /// Construct an element in-place. Returns false if the queue is full.
    template <class... Args>
    [[nodiscard]] bool try_emplace(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<T, Args...>)
    {
        if (size() == Capacity) return false;
        construct(slot(tail_), std::forward<Args>(args)...);
        tail_ = advance(tail_);
        return true;
    }

    /// Construct an element in-place; throws std::overflow_error if full.
    template <class... Args>
    reference emplace(Args&&... args)
    {
        if (size() == Capacity)
            throw std::overflow_error("hpc::core::fixed_queue::emplace: queue is full");
        T* s = slot(tail_);
        construct(s, std::forward<Args>(args)...);
        tail_ = advance(tail_);
        return *s;
    }

    /// Pop the front element. Returns false if the queue is empty.
    [[nodiscard]] bool try_pop() noexcept
    {
        if (empty()) return false;
        destroy(slot(head_));
        head_ = advance(head_);
        return true;
    }

    /// Pop the front element; throws std::underflow_error if empty.
    void pop()
    {
        if (empty())
            throw std::underflow_error("hpc::core::fixed_queue::pop: queue is empty");
        destroy(slot(head_));
        head_ = advance(head_);
    }

    /// Pop the front element into @p out. Returns false if empty.
    [[nodiscard]] bool try_pop(T& out)
        noexcept(std::is_nothrow_move_assignable_v<T>)
    {
        if (empty()) return false;
        out = std::move(*slot(head_));
        destroy(slot(head_));
        head_ = advance(head_);
        return true;
    }

    /// Clear all elements.
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

    /// Swap contents with another queue.
    void swap(fixed_queue& other)
        noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        fixed_queue tmp(std::move(other));
        other = std::move(*this);
        *this = std::move(tmp);
    }

    // -- Comparison -------------------------------------------------------

    bool operator==(const fixed_queue& rhs) const noexcept
    {
        if (size() != rhs.size()) return false;
        size_type li = head_, ri = rhs.head_;
        while (li != tail_) {
            if (!(*slot(li) == *rhs.slot(ri))) return false;
            li = advance(li);
            ri = rhs.advance(ri);
        }
        return true;
    }

    bool operator!=(const fixed_queue& rhs) const noexcept
    {
        return !(*this == rhs);
    }

private:
    // -- Storage ----------------------------------------------------------
    //
    // Circular buffer of buf_cap slots (power of two). head_ points to the
    // front element, tail_ points one past the back. head_==tail_ ⇒ empty.

    alignas(storage_align) std::byte storage_[sizeof(T) * buf_cap];
    size_type head_ = 0;
    size_type tail_ = 0;

    // -- Helpers ----------------------------------------------------------

    size_type advance(size_type idx) const noexcept
    {
        return (idx + 1) & buf_mask;
    }

    T* slot(size_type i) noexcept
    {
        return std::launder(reinterpret_cast<T*>(storage_ + i * sizeof(T)));
    }

    const T* slot(size_type i) const noexcept
    {
        return std::launder(reinterpret_cast<const T*>(storage_ + i * sizeof(T)));
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
};

} // namespace hpc::core

