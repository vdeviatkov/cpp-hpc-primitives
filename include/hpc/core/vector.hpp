#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace hpc::core {

// Dynamic contiguous container, drop-in replacement for std::vector.
//
// Design notes:
//  - 2× growth factor for amortised O(1) push_back.
//  - Trivially copyable types use memcpy/memmove instead of per-element
//    move-construct + destroy loops (reallocate, insert, erase).
//  - Over-aligned types (alignof(T) > default new alignment) are handled
//    via the C++17 aligned operator new/delete.
//  - Placement new for precise object lifetime control.

template <class T>
class vector {
    static constexpr bool is_trivial      = std::is_trivially_copyable_v<T>;
    static constexpr bool is_over_aligned = alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__;

public:
    using value_type      = T;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference       = T&;
    using const_reference = const T&;
    using pointer         = T*;
    using const_pointer   = const T*;
    using iterator        = T*;
    using const_iterator  = const T*;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    vector() noexcept = default;

    explicit vector(size_type count)
    {
        grow_to(count);
        for (size_type i = 0; i < count; ++i)
            construct(data_ + i);
        size_ = count;
    }

    vector(size_type count, const T& value)
    {
        grow_to(count);
        for (size_type i = 0; i < count; ++i)
            construct(data_ + i, value);
        size_ = count;
    }

    template <class It, class = std::enable_if_t<!std::is_integral_v<It>>>
    vector(It first, It last)
    {
        for (; first != last; ++first)
            push_back(*first);
    }

    vector(std::initializer_list<T> il)
    {
        grow_to(il.size());
        for (auto& v : il)
            construct(data_ + size_++, v);
    }

    vector(const vector& rhs)
    {
        grow_to(rhs.size_);
        for (size_type i = 0; i < rhs.size_; ++i)
            construct(data_ + i, rhs.data_[i]);
        size_ = rhs.size_;
    }

    vector(vector&& rhs) noexcept
        : data_(rhs.data_), size_(rhs.size_), cap_(rhs.cap_)
    {
        rhs.data_ = nullptr;
        rhs.size_ = 0;
        rhs.cap_  = 0;
    }

    ~vector() { clear(); free(data_); }

    vector& operator=(const vector& rhs)
    {
        if (this != &rhs) {
            vector tmp(rhs);
            swap(tmp);
        }
        return *this;
    }

    vector& operator=(vector&& rhs) noexcept
    {
        if (this != &rhs) {
            clear();
            free(data_);
            data_ = rhs.data_;  size_ = rhs.size_;  cap_ = rhs.cap_;
            rhs.data_ = nullptr;  rhs.size_ = 0;  rhs.cap_ = 0;
        }
        return *this;
    }

    vector& operator=(std::initializer_list<T> il)
    {
        vector tmp(il);
        swap(tmp);
        return *this;
    }

    // Element access.

    reference       operator[](size_type i)       noexcept { return data_[i]; }
    const_reference operator[](size_type i) const noexcept { return data_[i]; }

    reference at(size_type i)
    {
        if (i >= size_) throw std::out_of_range("hpc::core::vector::at");
        return data_[i];
    }
    const_reference at(size_type i) const
    {
        if (i >= size_) throw std::out_of_range("hpc::core::vector::at");
        return data_[i];
    }

    reference       front()       noexcept { return data_[0]; }
    const_reference front() const noexcept { return data_[0]; }
    reference       back()        noexcept { return data_[size_ - 1]; }
    const_reference back()  const noexcept { return data_[size_ - 1]; }

    pointer       data()       noexcept { return data_; }
    const_pointer data() const noexcept { return data_; }

    // Iterators.

    iterator       begin()        noexcept { return data_; }
    const_iterator begin()  const noexcept { return data_; }
    const_iterator cbegin() const noexcept { return data_; }
    iterator       end()          noexcept { return data_ + size_; }
    const_iterator end()    const noexcept { return data_ + size_; }
    const_iterator cend()   const noexcept { return data_ + size_; }

    reverse_iterator       rbegin()        noexcept { return reverse_iterator(end()); }
    const_reverse_iterator rbegin()  const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }
    reverse_iterator       rend()          noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator rend()    const noexcept { return const_reverse_iterator(begin()); }
    const_reverse_iterator crend()   const noexcept { return const_reverse_iterator(begin()); }

    // Capacity.

    [[nodiscard]] bool empty()    const noexcept { return size_ == 0; }
    size_type          size()     const noexcept { return size_; }
    size_type          capacity() const noexcept { return cap_; }

    void reserve(size_type n) { if (n > cap_) grow_to(n); }

    void shrink_to_fit()
    {
        if (size_ < cap_)
            reallocate(size_);
    }

    // Modifiers.

    void clear() noexcept
    {
        destroy_n(data_, size_);
        size_ = 0;
    }

    void push_back(const T& v) { ensure_space(1); construct(data_ + size_++, v); }
    void push_back(T&&      v) { ensure_space(1); construct(data_ + size_++, std::move(v)); }

    template <class... Args>
    reference emplace_back(Args&&... args)
    {
        ensure_space(1);
        T* slot = data_ + size_++;
        construct(slot, std::forward<Args>(args)...);
        return *slot;
    }

    void pop_back() noexcept
    {
        --size_;
        if constexpr (!is_trivial)
            data_[size_].~T();
    }

    iterator insert(const_iterator pos, const T& v) { return insert_impl(pos, v); }
    iterator insert(const_iterator pos, T&&      v) { return insert_impl(pos, std::move(v)); }

    template <class... Args>
    iterator emplace(const_iterator pos, Args&&... args)
    {
        auto off = pos - data_;
        ensure_space(1);
        auto* p = data_ + off;
        shift_right(p, data_ + size_, 1);
        construct(p, std::forward<Args>(args)...);
        ++size_;
        return p;
    }

    iterator erase(const_iterator pos)
    {
        auto* p = const_cast<iterator>(pos);
        if constexpr (!is_trivial) p->~T();
        shift_left(p + 1, data_ + size_, 1);
        --size_;
        return p;
    }

    iterator erase(const_iterator first, const_iterator last)
    {
        auto* f = const_cast<iterator>(first);
        auto* l = const_cast<iterator>(last);
        auto  n = static_cast<size_type>(l - f);
        if (n == 0) return f;

        destroy_n(f, n);
        shift_left(l, data_ + size_, n);
        size_ -= n;
        return f;
    }

    void resize(size_type count)
    {
        if (count < size_) {
            destroy_n(data_ + count, size_ - count);
        } else if (count > size_) {
            ensure_space(count - size_);
            for (size_type i = size_; i < count; ++i)
                construct(data_ + i);
        }
        size_ = count;
    }

    void resize(size_type count, const T& value)
    {
        if (count < size_) {
            destroy_n(data_ + count, size_ - count);
        } else if (count > size_) {
            ensure_space(count - size_);
            for (size_type i = size_; i < count; ++i)
                construct(data_ + i, value);
        }
        size_ = count;
    }

    void swap(vector& o) noexcept
    {
        std::swap(data_, o.data_);
        std::swap(size_, o.size_);
        std::swap(cap_,  o.cap_);
    }

    void assign(size_type count, const T& value)
    {
        clear();
        reserve(count);
        for (size_type i = 0; i < count; ++i)
            construct(data_ + i, value);
        size_ = count;
    }

    template <class It, class = std::enable_if_t<!std::is_integral_v<It>>>
    void assign(It first, It last)
    {
        clear();
        for (; first != last; ++first)
            push_back(*first);
    }

    void assign(std::initializer_list<T> il)
    {
        clear();
        reserve(il.size());
        for (auto& v : il)
            construct(data_ + size_++, v);
    }

    // Comparisons.

    friend bool operator==(const vector& a, const vector& b) noexcept
    {
        if (a.size_ != b.size_) return false;
        if constexpr (is_trivial)
            return std::memcmp(a.data_, b.data_, a.size_ * sizeof(T)) == 0;
        else {
            for (size_type i = 0; i < a.size_; ++i)
                if (!(a.data_[i] == b.data_[i])) return false;
            return true;
        }
    }

    friend bool operator!=(const vector& a, const vector& b) noexcept { return !(a == b); }

    friend bool operator<(const vector& a, const vector& b) noexcept
    {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
    }

    friend bool operator<=(const vector& a, const vector& b) noexcept { return !(b < a); }
    friend bool operator>(const vector& a, const vector& b) noexcept  { return  (b < a); }
    friend bool operator>=(const vector& a, const vector& b) noexcept { return !(a < b); }

private:
    T*        data_{nullptr};
    size_type size_{0};
    size_type cap_ {0};

    // Placement-new wrapper — single call site keeps the ugly cast in one place.
    template <class... Args>
    static void construct(T* where, Args&&... args)
    {
        ::new (static_cast<void*>(where)) T(std::forward<Args>(args)...);
    }

    static T* alloc(size_type n)
    {
        if (n == 0) return nullptr;
        void* p;
        if constexpr (is_over_aligned)
            p = ::operator new(n * sizeof(T), std::align_val_t{alignof(T)});
        else
            p = ::operator new(n * sizeof(T));
        return static_cast<T*>(p);
    }

    static void free(T* p) noexcept
    {
        if constexpr (is_over_aligned)
            ::operator delete(static_cast<void*>(p), std::align_val_t{alignof(T)});
        else
            ::operator delete(static_cast<void*>(p));
    }

    // Destroy `count` objects starting at `first`.
    // No-op for trivially copyable types (their destructors are trivial).
    static void destroy_n(T* first, size_type count) noexcept
    {
        if constexpr (!is_trivial) {
            for (size_type i = 0; i < count; ++i)
                first[i].~T();
        }
        (void)first; (void)count;
    }

    void ensure_space(size_type extra)
    {
        if (size_ + extra <= cap_) return;
        size_type new_cap = cap_ ? cap_ * 2 : 4;
        while (new_cap < size_ + extra) new_cap *= 2;
        grow_to(new_cap);
    }

    void grow_to(size_type new_cap)
    {
        if (new_cap <= cap_) return;
        reallocate(new_cap);
    }

    // Relocate live elements into a fresh buffer of `new_cap` slots.
    // Trivially copyable types get a single memcpy; everything else is
    // element-wise move-construct + destroy.
    void reallocate(size_type new_cap)
    {
        T* buf = alloc(new_cap);
        if constexpr (is_trivial) {
            std::memcpy(buf, data_, size_ * sizeof(T));
        } else {
            for (size_type i = 0; i < size_; ++i) {
                construct(buf + i, std::move(data_[i]));
                data_[i].~T();
            }
        }
        free(data_);
        data_ = buf;
        cap_  = new_cap;
    }

    // Shared insert logic for lvalue / rvalue overloads.
    template <class U>
    iterator insert_impl(const_iterator pos, U&& value)
    {
        auto off = pos - data_;
        ensure_space(1);
        auto* p = data_ + off;
        shift_right(p, data_ + size_, 1);
        construct(p, std::forward<U>(value));
        ++size_;
        return p;
    }

    // Move elements [first, last) right by `n` slots.
    // Uses memmove for trivial types (handles overlap correctly).
    static void shift_right(T* first, T* last, size_type n)
    {
        auto count = static_cast<size_type>(last - first);
        if (count == 0) return;
        if constexpr (is_trivial) {
            std::memmove(first + n, first, count * sizeof(T));
        } else {
            for (auto* it = last; it != first;) {
                --it;
                construct(it + n, std::move(*it));
                it->~T();
            }
        }
    }

    // Move elements [first, last) left by `n` slots.
    static void shift_left(T* first, T* last, size_type n) noexcept(std::is_nothrow_move_assignable_v<T>)
    {
        auto count = static_cast<size_type>(last - first);
        if (count == 0) return;
        if constexpr (is_trivial) {
            std::memmove(first - n, first, count * sizeof(T));
        } else {
            for (auto* it = first; it != last; ++it) {
                construct(it - n, std::move(*it));
                it->~T();
            }
        }
    }
};

} // namespace hpc::core

