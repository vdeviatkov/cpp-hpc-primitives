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

// Double-ended queue with chunked storage.
//
// Design notes:
//  - Matches std::deque iterator/reference invalidation guarantees:
//    · push_back / push_front: all iterators invalidated, but
//      references to existing elements remain valid.
//    · pop_back / pop_front: only iterators/references to the erased
//      element (and past-the-end) are invalidated.
//  - Fixed-size chunks (~512 bytes each) are individually heap-allocated
//    and tracked via a central "map" (array of chunk pointers).
//  - Elements never relocate once constructed (chunks are stable), which
//    is the key property enabling reference stability.
//  - Trivially copyable types skip per-element destructor calls.
//  - Over-aligned types use aligned operator new/delete.
//  - Placement new for precise object lifetime control.

template <class T>
class deque {
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

    /// Elements per chunk.  Targets ~512 bytes per chunk, minimum 1.
    static constexpr size_type chunk_elems =
        sizeof(T) < 512 ? 512 / sizeof(T) : 1;

    // -- Iterator ---------------------------------------------------------

    template <bool Const>
    class iterator_impl {
        friend class deque;
        friend class iterator_impl<!Const>;

        T*  cur_   = nullptr;
        T*  first_ = nullptr;   // start of current chunk
        T*  last_  = nullptr;   // one-past-end of current chunk
        T** node_  = nullptr;   // pointer into the map array

        void set_node(T** n) noexcept {
            node_  = n;
            first_ = *n;
            last_  = first_ + static_cast<difference_type>(chunk_elems);
        }

        iterator_impl(T* cur, T** node) noexcept
            : cur_(cur), first_(*node),
              last_(*node + static_cast<difference_type>(chunk_elems)),
              node_(node) {}

    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = std::conditional_t<Const, const T*, T*>;
        using reference         = std::conditional_t<Const, const T&, T&>;

        iterator_impl() noexcept = default;

        // non-const → const conversion
        template <bool C2, class = std::enable_if_t<Const && !C2>>
        iterator_impl(const iterator_impl<C2>& o) noexcept
            : cur_(o.cur_), first_(o.first_), last_(o.last_), node_(o.node_) {}

        reference operator*()  const noexcept { return *cur_; }
        pointer   operator->() const noexcept { return  cur_; }

        iterator_impl& operator++() noexcept {
            ++cur_;
            if (cur_ == last_) { set_node(node_ + 1); cur_ = first_; }
            return *this;
        }
        iterator_impl operator++(int) noexcept { auto t = *this; ++*this; return t; }

        iterator_impl& operator--() noexcept {
            if (cur_ == first_) { set_node(node_ - 1); cur_ = last_; }
            --cur_;
            return *this;
        }
        iterator_impl operator--(int) noexcept { auto t = *this; --*this; return t; }

        iterator_impl& operator+=(difference_type n) noexcept {
            const auto ce = static_cast<difference_type>(chunk_elems);
            difference_type off = n + (cur_ - first_);
            if (off >= 0 && off < ce) {
                cur_ += n;
            } else {
                difference_type node_off =
                    off > 0 ? off / ce
                            : -((-off - 1) / ce) - 1;
                set_node(node_ + node_off);
                cur_ = first_ + (off - node_off * ce);
            }
            return *this;
        }
        iterator_impl& operator-=(difference_type n) noexcept { return *this += -n; }

        reference operator[](difference_type n) const noexcept {
            return *(*this + n);
        }

        friend iterator_impl operator+(iterator_impl i, difference_type n) noexcept { return i += n; }
        friend iterator_impl operator+(difference_type n, iterator_impl i) noexcept { return i += n; }
        friend iterator_impl operator-(iterator_impl i, difference_type n) noexcept { return i -= n; }
        friend difference_type operator-(const iterator_impl& a,
                                         const iterator_impl& b) noexcept {
            return static_cast<difference_type>(chunk_elems)
                       * (a.node_ - b.node_ - 1)
                   + (a.cur_ - a.first_)
                   + (b.last_ - b.cur_);
        }

        friend bool operator==(const iterator_impl& a, const iterator_impl& b) noexcept { return a.cur_ == b.cur_; }
        friend bool operator!=(const iterator_impl& a, const iterator_impl& b) noexcept { return a.cur_ != b.cur_; }
        friend bool operator< (const iterator_impl& a, const iterator_impl& b) noexcept {
            return a.node_ == b.node_ ? a.cur_ < b.cur_ : a.node_ < b.node_;
        }
        friend bool operator> (const iterator_impl& a, const iterator_impl& b) noexcept { return b < a; }
        friend bool operator<=(const iterator_impl& a, const iterator_impl& b) noexcept { return !(b < a); }
        friend bool operator>=(const iterator_impl& a, const iterator_impl& b) noexcept { return !(a < b); }
    };

    using iterator               = iterator_impl<false>;
    using const_iterator         = iterator_impl<true>;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // -- Constructors / destructor ----------------------------------------

    deque() { init_map(0); }

    explicit deque(size_type count)
    {
        init_map(count);
        default_fill();
    }

    deque(size_type count, const T& value)
    {
        init_map(count);
        value_fill(value);
    }

    deque(std::initializer_list<T> il)
    {
        init_map(il.size());
        auto dst = start_;
        for (auto& v : il) { construct(dst.cur_, v); ++dst; }
    }

    template <class It, class = std::enable_if_t<!std::is_integral_v<It>>>
    deque(It first, It last)
    {
        init_map(0);
        for (; first != last; ++first) push_back(*first);
    }

    deque(const deque& rhs)
    {
        init_map(rhs.size());
        copy_from(rhs);
    }

    deque(deque&& rhs) noexcept
        : map_(rhs.map_), map_size_(rhs.map_size_),
          start_(rhs.start_), finish_(rhs.finish_)
    {
        rhs.map_ = nullptr; rhs.map_size_ = 0;
        rhs.start_ = {}; rhs.finish_ = {};
    }

    ~deque() { teardown(); }

    deque& operator=(const deque& rhs)
    {
        if (this != &rhs) { deque tmp(rhs); swap(tmp); }
        return *this;
    }

    deque& operator=(deque&& rhs) noexcept
    {
        if (this != &rhs) {
            teardown();
            map_ = rhs.map_; map_size_ = rhs.map_size_;
            start_ = rhs.start_; finish_ = rhs.finish_;
            rhs.map_ = nullptr; rhs.map_size_ = 0;
            rhs.start_ = {}; rhs.finish_ = {};
        }
        return *this;
    }

    deque& operator=(std::initializer_list<T> il)
    {
        deque tmp(il); swap(tmp); return *this;
    }

    // -- Capacity ---------------------------------------------------------

    [[nodiscard]] bool      empty() const noexcept { return start_.cur_ == finish_.cur_; }
    [[nodiscard]] size_type size()  const noexcept {
        if (!map_) return 0;
        return static_cast<size_type>(finish_ - start_);
    }

    // -- Element access ---------------------------------------------------

    reference       operator[](size_type i)       noexcept { return start_[static_cast<difference_type>(i)]; }
    const_reference operator[](size_type i) const noexcept { return const_iterator(start_)[static_cast<difference_type>(i)]; }

    reference at(size_type i)
    {
        if (i >= size()) throw std::out_of_range("hpc::core::deque::at");
        return (*this)[i];
    }
    const_reference at(size_type i) const
    {
        if (i >= size()) throw std::out_of_range("hpc::core::deque::at");
        return (*this)[i];
    }

    reference       front()       noexcept { return *start_.cur_; }
    const_reference front() const noexcept { return *start_.cur_; }
    reference       back()        noexcept { auto t = finish_; --t; return *t; }
    const_reference back()  const noexcept { const_iterator t = finish_; --t; return *t; }

    // -- Iterators --------------------------------------------------------

    iterator       begin()        noexcept { return start_; }
    const_iterator begin()  const noexcept { return start_; }
    const_iterator cbegin() const noexcept { return start_; }
    iterator       end()          noexcept { return finish_; }
    const_iterator end()    const noexcept { return finish_; }
    const_iterator cend()   const noexcept { return finish_; }

    reverse_iterator       rbegin()        noexcept { return reverse_iterator(end()); }
    const_reverse_iterator rbegin()  const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }
    reverse_iterator       rend()          noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator rend()    const noexcept { return const_reverse_iterator(begin()); }
    const_reverse_iterator crend()   const noexcept { return const_reverse_iterator(begin()); }

    // -- Modifiers (back) -------------------------------------------------

    void push_back(const T& value)
    {
        if (finish_.cur_ != finish_.last_ - 1) {
            construct(finish_.cur_, value);
            ++finish_.cur_;
        } else {
            push_back_aux(value);
        }
    }

    void push_back(T&& value)
    {
        if (finish_.cur_ != finish_.last_ - 1) {
            construct(finish_.cur_, std::move(value));
            ++finish_.cur_;
        } else {
            push_back_aux(std::move(value));
        }
    }

    template <class... Args>
    reference emplace_back(Args&&... args)
    {
        if (finish_.cur_ != finish_.last_ - 1) {
            construct(finish_.cur_, std::forward<Args>(args)...);
            T& ref = *finish_.cur_;
            ++finish_.cur_;
            return ref;
        }
        return emplace_back_aux(std::forward<Args>(args)...);
    }

    void pop_back() noexcept
    {
        if (finish_.cur_ != finish_.first_) {
            --finish_.cur_;
            destroy(finish_.cur_);
        } else {
            pop_back_aux();
        }
    }

    // -- Modifiers (front) ------------------------------------------------

    void push_front(const T& value)
    {
        if (start_.cur_ != start_.first_) {
            construct(start_.cur_ - 1, value);
            --start_.cur_;
        } else {
            push_front_aux(value);
        }
    }

    void push_front(T&& value)
    {
        if (start_.cur_ != start_.first_) {
            construct(start_.cur_ - 1, std::move(value));
            --start_.cur_;
        } else {
            push_front_aux(std::move(value));
        }
    }

    template <class... Args>
    reference emplace_front(Args&&... args)
    {
        if (start_.cur_ != start_.first_) {
            construct(start_.cur_ - 1, std::forward<Args>(args)...);
            --start_.cur_;
            return *start_.cur_;
        }
        return emplace_front_aux(std::forward<Args>(args)...);
    }

    void pop_front() noexcept
    {
        if (start_.cur_ != start_.last_ - 1) {
            destroy(start_.cur_);
            ++start_.cur_;
        } else {
            pop_front_aux();
        }
    }

    // -- General modifiers ------------------------------------------------

    void clear() noexcept
    {
        if (!map_) return;
        // Destroy elements and free inner chunks (keep first chunk).
        for (T** node = start_.node_ + 1; node < finish_.node_; ++node) {
            destroy_range(*node, *node + chunk_elems);
            free_chunk(*node);
        }
        if (start_.node_ != finish_.node_) {
            destroy_range(start_.cur_, start_.last_);
            destroy_range(finish_.first_, finish_.cur_);
            free_chunk(*finish_.node_);
        } else {
            destroy_range(start_.cur_, finish_.cur_);
        }
        finish_.set_node(start_.node_);
        finish_.cur_ = start_.cur_ = start_.first_;
    }

    void resize(size_type count)
    {
        auto s = size();
        if (count < s) {
            for (size_type i = count; i < s; ++i) pop_back();
        } else {
            for (size_type i = s; i < count; ++i) emplace_back();
        }
    }

    void resize(size_type count, const T& value)
    {
        auto s = size();
        if (count < s) {
            for (size_type i = count; i < s; ++i) pop_back();
        } else {
            for (size_type i = s; i < count; ++i) push_back(value);
        }
    }

    void swap(deque& o) noexcept
    {
        std::swap(map_, o.map_);   std::swap(map_size_, o.map_size_);
        std::swap(start_, o.start_); std::swap(finish_, o.finish_);
    }

    // -- Comparison -------------------------------------------------------

    friend bool operator==(const deque& a, const deque& b) noexcept
    {
        if (a.size() != b.size()) return false;
        auto ai = a.cbegin(), bi = b.cbegin();
        for (auto ae = a.cend(); ai != ae; ++ai, ++bi)
            if (!(*ai == *bi)) return false;
        return true;
    }
    friend bool operator!=(const deque& a, const deque& b) noexcept { return !(a == b); }

private:
    T**       map_      = nullptr;
    size_type map_size_ = 0;
    iterator  start_;
    iterator  finish_;

    // -- Chunk allocation -------------------------------------------------

    static T* alloc_chunk()
    {
        void* p;
        if constexpr (is_over_aligned)
            p = ::operator new(chunk_elems * sizeof(T), std::align_val_t{alignof(T)});
        else
            p = ::operator new(chunk_elems * sizeof(T));
        return static_cast<T*>(p);
    }

    static void free_chunk(T* p) noexcept
    {
        if constexpr (is_over_aligned)
            ::operator delete(static_cast<void*>(p), std::align_val_t{alignof(T)});
        else
            ::operator delete(static_cast<void*>(p));
    }

    // -- Map allocation ---------------------------------------------------

    static T** alloc_map(size_type n)
    {
        return static_cast<T**>(::operator new(n * sizeof(T*)));
    }

    static void free_map(T** p) noexcept
    {
        ::operator delete(static_cast<void*>(p));
    }

    // -- Element helpers --------------------------------------------------

    template <class... Args>
    static void construct(T* p, Args&&... args)
    {
        ::new (static_cast<void*>(p)) T(std::forward<Args>(args)...);
    }

    static void destroy(T* p) noexcept
    {
        if constexpr (!is_trivial) p->~T();
    }

    static void destroy_range(T* first, T* last) noexcept
    {
        if constexpr (!is_trivial)
            for (; first != last; ++first) first->~T();
    }

    // -- Initialization ---------------------------------------------------

    void init_map(size_type num_elements)
    {
        size_type num_nodes = num_elements / chunk_elems + 1;
        map_size_ = std::max(size_type(8), num_nodes + 2);
        map_ = alloc_map(map_size_);
        std::fill(map_, map_ + map_size_, nullptr);

        // Center nodes in the map for equal room at both ends.
        T** nstart  = map_ + (map_size_ - num_nodes) / 2;
        T** nfinish = nstart + num_nodes;

        for (T** cur = nstart; cur < nfinish; ++cur)
            *cur = alloc_chunk();

        start_.set_node(nstart);
        start_.cur_ = start_.first_;
        finish_.set_node(nfinish - 1);
        finish_.cur_ = finish_.first_
                       + static_cast<difference_type>(num_elements % chunk_elems);
    }

    void default_fill()
    {
        for (T** node = start_.node_; node < finish_.node_; ++node)
            for (T* p = *node; p < *node + chunk_elems; ++p)
                construct(p);
        for (T* p = finish_.first_; p < finish_.cur_; ++p)
            construct(p);
    }

    void value_fill(const T& value)
    {
        for (T** node = start_.node_; node < finish_.node_; ++node)
            for (T* p = *node; p < *node + chunk_elems; ++p)
                construct(p, value);
        for (T* p = finish_.first_; p < finish_.cur_; ++p)
            construct(p, value);
    }

    void copy_from(const deque& rhs)
    {
        auto src = rhs.start_;
        auto dst = start_;
        for (size_type i = 0, n = rhs.size(); i < n; ++i, ++src, ++dst)
            construct(dst.cur_, *src);
    }

    // -- Teardown ---------------------------------------------------------

    void teardown() noexcept
    {
        if (!map_) return;
        // Destroy live elements.
        for (T** node = start_.node_; node < finish_.node_; ++node)
            destroy_range(node == start_.node_ ? start_.cur_ : *node,
                          *node + chunk_elems);
        if (start_.node_ == finish_.node_)
            destroy_range(start_.cur_, finish_.cur_);
        else
            destroy_range(finish_.first_, finish_.cur_);
        // Free all allocated chunks.
        for (T** node = start_.node_; node <= finish_.node_; ++node)
            free_chunk(*node);
        free_map(map_);
        map_ = nullptr;
    }

    // -- Push / pop aux ---------------------------------------------------

    template <class... Args>
    void push_back_aux(Args&&... args)
    {
        reserve_map_at_back();
        *(finish_.node_ + 1) = alloc_chunk();
        construct(finish_.cur_, std::forward<Args>(args)...);
        finish_.set_node(finish_.node_ + 1);
        finish_.cur_ = finish_.first_;
    }

    template <class... Args>
    reference emplace_back_aux(Args&&... args)
    {
        reserve_map_at_back();
        *(finish_.node_ + 1) = alloc_chunk();
        construct(finish_.cur_, std::forward<Args>(args)...);
        T& ref = *finish_.cur_;
        finish_.set_node(finish_.node_ + 1);
        finish_.cur_ = finish_.first_;
        return ref;
    }

    template <class... Args>
    void push_front_aux(Args&&... args)
    {
        reserve_map_at_front();
        *(start_.node_ - 1) = alloc_chunk();
        start_.set_node(start_.node_ - 1);
        start_.cur_ = start_.last_ - 1;
        construct(start_.cur_, std::forward<Args>(args)...);
    }

    template <class... Args>
    reference emplace_front_aux(Args&&... args)
    {
        reserve_map_at_front();
        *(start_.node_ - 1) = alloc_chunk();
        start_.set_node(start_.node_ - 1);
        start_.cur_ = start_.last_ - 1;
        construct(start_.cur_, std::forward<Args>(args)...);
        return *start_.cur_;
    }

    void pop_back_aux() noexcept
    {
        free_chunk(*finish_.node_);
        finish_.set_node(finish_.node_ - 1);
        finish_.cur_ = finish_.last_ - 1;
        destroy(finish_.cur_);
    }

    void pop_front_aux() noexcept
    {
        destroy(start_.cur_);
        free_chunk(*start_.node_);
        start_.set_node(start_.node_ + 1);
        start_.cur_ = start_.first_;
    }

    // -- Map management ---------------------------------------------------

    void reserve_map_at_back(size_type nodes_to_add = 1)
    {
        if (nodes_to_add + 1 > map_size_
                - static_cast<size_type>(finish_.node_ - map_))
            reallocate_map(nodes_to_add, false);
    }

    void reserve_map_at_front(size_type nodes_to_add = 1)
    {
        if (nodes_to_add > static_cast<size_type>(start_.node_ - map_))
            reallocate_map(nodes_to_add, true);
    }

    void reallocate_map(size_type nodes_to_add, bool add_at_front)
    {
        size_type old_num = static_cast<size_type>(finish_.node_ - start_.node_) + 1;
        size_type new_num = old_num + nodes_to_add;

        T** new_nstart;
        if (map_size_ > 2 * new_num) {
            // Map is big enough; just shift entries to re-center.
            new_nstart = map_ + (map_size_ - new_num) / 2
                         + (add_at_front ? nodes_to_add : 0);
            if (new_nstart < start_.node_)
                std::copy(start_.node_, finish_.node_ + 1, new_nstart);
            else
                std::copy_backward(start_.node_, finish_.node_ + 1,
                                   new_nstart + old_num);
        } else {
            size_type new_map_size = map_size_
                                     + std::max(map_size_, nodes_to_add) + 2;
            T** new_map = alloc_map(new_map_size);
            new_nstart = new_map + (new_map_size - new_num) / 2
                         + (add_at_front ? nodes_to_add : 0);
            std::copy(start_.node_, finish_.node_ + 1, new_nstart);
            free_map(map_);
            map_ = new_map;
            map_size_ = new_map_size;
        }
        start_.set_node(new_nstart);
        finish_.set_node(new_nstart + old_num - 1);
    }
};

} // namespace hpc::core

