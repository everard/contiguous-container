// Copyright Ildus Nezametdinov 2017.
// Distributed under the Boost Software License, Version 1.0.
//(See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef STORAGE_TYPES_H
#define STORAGE_TYPES_H

#include "storage_traits.h"
#include <stdexcept>

namespace ecs
{
namespace detail
{
template <typename Storage, typename... Args>
void initialize_next(Storage& storage, Args&&... args)
{
        using traits = storage_traits<Storage>;
        traits::construct(storage, traits::end(storage), std::forward<Args>(args)...);
        traits::inc_size(storage);
}

template <typename Storage, typename Size, typename ForwardIterator>
Storage& assign_n(Storage& storage, Size n, ForwardIterator first)
{
        using traits = storage_traits<Storage>;

        if(n > traits::capacity(storage))
                traits::reallocate_assign(storage, n, first);
        else
                traits::assign(storage, n, first);

        return storage;
}

template <typename Storage>
void destroy_elements(Storage& storage) noexcept
{
        using traits = storage_traits<Storage>;
        for_each_iter(traits::begin(storage), traits::end(storage),
                      [&storage](auto i) { traits::destroy(storage, i); });
}

//
} // namespace detail

template <typename T, std::size_t N>
struct inplace_storage
{
        // types:
        using traits = storage_traits<inplace_storage>;
        using value_type = T;

        // friend declaration:
        friend struct storage_traits<inplace_storage>;

        // construct:
        inplace_storage() = default;

        explicit inplace_storage(std::size_t n) : inplace_storage{}
        {
                for(n = std::min(n, capacity()); n > 0; --n)
                        detail::initialize_next(*this);
        }

        inplace_storage(std::size_t n, const value_type& x) : inplace_storage{}
        {
                for(n = std::min(n, capacity()); n > 0; --n)
                        detail::initialize_next(*this, x);
        }

        template <typename InputIterator, typename = check_input_iterator<InputIterator>>
        inplace_storage(InputIterator first, InputIterator last) : inplace_storage{}
        {
                for(; first != last && !traits::full(*this); ++first)
                        detail::initialize_next(*this, *first);
        }

        inplace_storage(std::initializer_list<value_type> il)
                : inplace_storage{il.begin(), il.end()}
        {
        }

        // copy/move construct:
        inplace_storage(const inplace_storage& other) : inplace_storage{}
        {
                traits::assign(*this, traits::size(other), traits::begin(other));
        }

        inplace_storage(inplace_storage&& other) : inplace_storage{}
        {
                traits::assign(
                        *this, traits::size(other), std::make_move_iterator(traits::begin(other)));

                detail::destroy_elements(other);
                traits::set_size(other, 0);
        }

        // copy/move assign:
        inplace_storage& operator=(const inplace_storage& other)
        {
                if(this == std::addressof(other))
                        return *this;

                traits::assign(*this, traits::size(other), traits::begin(other));
                return *this;
        }

        inplace_storage& operator=(inplace_storage&& other)
        {
                if(this == std::addressof(other))
                        return *this;

                traits::assign(
                        *this, traits::size(other), std::make_move_iterator(traits::begin(other)));

                detail::destroy_elements(other);
                traits::set_size(other, 0);

                return *this;
        }

        // swap:
        void swap(inplace_storage& other)
        {
                auto& x = size() >= other.size() ? other : *this;
                auto& y = size() >= other.size() ? *this : other;

                auto n = x.size();
                auto f = y.begin() + n, l = y.end();

                std::swap_ranges(x.begin(), x.end(), y.begin());

                for_each_iter(f, l, [&x](auto i) { detail::initialize_next(x, std::move(*i)); });
                for_each_iter(f, l, [&y](auto i) { traits::destroy(y, i); });

                traits::set_size(y, n);
        }

protected:
        ~inplace_storage()
        {
                if /*constexpr*/ (!std::is_trivially_destructible<value_type>::value)
                        detail::destroy_elements(*this);
        }

private:
        value_type* begin() noexcept
        {
                return reinterpret_cast<value_type*>(data_);
        }

        const value_type* begin() const noexcept
        {
                return reinterpret_cast<const value_type*>(data_);
        }

        void set_size(std::size_t n) noexcept
        {
                size_ = n;
        }

        std::size_t size() const noexcept
        {
                return size_;
        }

        std::size_t capacity() const noexcept
        {
                return N;
        }

private:
        alignas(value_type) unsigned char data_[N * sizeof(value_type)];
        std::size_t size_{};
};

template <typename Storage>
struct allocator_aware_storage : Storage
{
        // types:
        using typename Storage::value_type;
        using typename Storage::allocator_type;

        using pointer = typename std::allocator_traits<allocator_type>::pointer;
        using const_pointer = typename std::allocator_traits<allocator_type>::const_pointer;

        using size_type = typename std::allocator_traits<allocator_type>::size_type;
        using difference_type = typename std::allocator_traits<allocator_type>::difference_type;

        // friend declaration:
        friend struct storage_traits<allocator_aware_storage>;

        // construct:
        allocator_aware_storage() noexcept(noexcept(allocator_type{})) : Storage{}
        {
        }

        explicit allocator_aware_storage(const allocator_type& a) noexcept : Storage{a}
        {
        }

        explicit allocator_aware_storage(size_type n, const allocator_type& a = allocator_type{})
                : allocator_aware_storage{n, a, principal_tag_{}}
        {
                for(; n > 0; --n)
                        detail::initialize_next(*this);
        }

        allocator_aware_storage(size_type n, const value_type& x,
                                const allocator_type& a = allocator_type{})
                : allocator_aware_storage{n, a, principal_tag_{}}
        {
                for(; n > 0; --n)
                        detail::initialize_next(*this, x);
        }

        template <typename InputIterator, typename = check_input_iterator<InputIterator>>
        allocator_aware_storage(InputIterator first, InputIterator last,
                                const allocator_type& a = allocator_type{})
                : allocator_aware_storage{
                          first, last, a,
                          typename std::iterator_traits<InputIterator>::iterator_category{}}
        {
        }

        allocator_aware_storage(std::initializer_list<value_type> il,
                                const allocator_type& a = allocator_type{})
                : allocator_aware_storage{il.begin(), il.end(), a, std::forward_iterator_tag{}}
        {
        }

        // copy/move construct:
        allocator_aware_storage(const allocator_aware_storage& other)
                : allocator_aware_storage{
                          other, alloc_traits_::select_on_container_copy_construction(
                                         other.get_allocator_ref())}
        {
        }

        allocator_aware_storage(allocator_aware_storage&&) = default;

        //
        allocator_aware_storage(const allocator_aware_storage& other, const allocator_type& a)
                : allocator_aware_storage{traits_::size(other), a, principal_tag_{}}
        {
                for(auto& x : other)
                        detail::initialize_next(*this, x);
        }

        allocator_aware_storage(allocator_aware_storage&& other, const allocator_type& a)
                : Storage{std::move(other), a}
        {
                detail::destroy_elements(other);
                traits_::set_size(other, 0);
        }

        // copy/move assign:
        allocator_aware_storage& operator=(const allocator_aware_storage& other)
        {
                if(this == std::addressof(other))
                        return *this;

                // copy allocator if needed
                if /*constexpr*/ (alloc_traits_::propagate_on_container_copy_assignment::value)
                {
                        // deallocate memory if allocators are not equal
                        if(!alloc_traits_::is_always_equal::value &&
                           this->get_allocator_ref() != other.get_allocator_ref())
                        {
                                detail::destroy_elements(*this);
                                this->deallocate();
                        }

                        this->get_allocator_ref() = other.get_allocator_ref();
                }

                return detail::assign_n(*this, traits_::size(other), traits_::begin(other));
        }

        allocator_aware_storage& operator=(allocator_aware_storage&&) = default;

        // returns copy of current allocator:
        allocator_type get_allocator() const noexcept
        {
                return this->get_allocator_ref();
        }

protected:
        ~allocator_aware_storage()
        {
                detail::destroy_elements(*this);
        }

private: //
        // additional types:
        struct principal_tag_
        {
        };

        using traits_ = storage_traits<allocator_aware_storage>;
        using alloc_traits_ = std::allocator_traits<allocator_type>;

        // requirement on allocator type:
        static_assert(std::is_same<value_type, typename alloc_traits_::value_type>::value);

        // additional constructors:
        allocator_aware_storage(size_type n, const allocator_type& a, principal_tag_)
                : Storage{n, a}
        {
        }

        template <typename InputIterator>
        allocator_aware_storage(InputIterator first, InputIterator last, const allocator_type& a,
                                std::input_iterator_tag)
                : allocator_aware_storage{a}
        {
                for(; first != last; ++first)
                {
                        if(traits_::full(*this))
                                traits_::reallocate(*this, traits_::capacity(*this) + 1);

                        detail::initialize_next(*this, *first);
                }
        }

        template <typename ForwardIterator>
        allocator_aware_storage(ForwardIterator first, ForwardIterator last,
                                const allocator_type& a, std::forward_iterator_tag)
                : allocator_aware_storage{
                          static_cast<size_type>(std::distance(first, last)), a, principal_tag_{}}
        {
                for_each_iter(first, last, [this](auto i) { detail::initialize_next(*this, *i); });
        }
};

template <typename T, typename Allocator>
struct vector_storage
{
        // types:
        using value_type = T;
        using allocator_type = Allocator;

        // friend declaration:
        friend struct storage_traits<vector_storage>;

        // deleted copy constructor and copy assignment operator:
        vector_storage(const vector_storage&) = delete;
        vector_storage& operator=(const vector_storage&) = delete;

protected: //
        // additional types:
        using traits_ = storage_traits<vector_storage>;
        using alloc_traits_ = std::allocator_traits<allocator_type>;

        using pointer_ = typename alloc_traits_::pointer;
        using const_pointer_ = typename alloc_traits_::const_pointer;

        using size_type_ = typename alloc_traits_::size_type;
        using difference_type_ = typename alloc_traits_::difference_type;

        struct implementation_ : allocator_type
        {
                implementation_() noexcept(noexcept(allocator_type{})) : allocator_type{}
                {
                }

                implementation_(const allocator_type& a) noexcept : allocator_type{a}
                {
                }

                implementation_(const implementation_&) = delete;
                implementation_(implementation_&&) = default;

                implementation_& operator=(const implementation_&) = delete;
                implementation_& operator=(implementation_&&) = default;

                //
                void swap(implementation_& other) noexcept
                {
                        std::swap(beg_, other.beg_);
                        std::swap(end_, other.end_);
                        std::swap(cap_, other.cap_);
                }

                //
                pointer_ beg_{}, end_{}, cap_{};
        };

        // construct/destroy:
        vector_storage() noexcept(noexcept(implementation_{})) : impl_{}
        {
        }

        vector_storage(const allocator_type& a) noexcept : impl_{a}
        {
        }

        vector_storage(size_type_ n, const allocator_type& a) : impl_{a}
        {
                impl_.beg_ = impl_.end_ = impl_.cap_ = alloc_traits_::allocate(impl_, n);
                impl_.cap_ += static_cast<difference_type_>(n);
        }

        ~vector_storage()
        {
                if(impl_.beg_)
                        alloc_traits_::deallocate(impl_, impl_.beg_, capacity());
        }

        // move construct:
        vector_storage(vector_storage&& other) noexcept : impl_{std::move(other.impl_)}
        {
                other.impl_.beg_ = other.impl_.end_ = other.impl_.cap_ = pointer_{};
        }

        vector_storage(vector_storage&& other,
                       const allocator_type& a) noexcept(alloc_traits_::is_always_equal::value)
                : impl_{a}
        {
                if(alloc_traits_::is_always_equal::value ||
                   get_allocator_ref() == other.get_allocator_ref())
                {
                        impl_.swap(other.impl_);
                        return;
                }

                if(other.empty())
                        return;

                impl_.beg_ = impl_.end_ = impl_.cap_ = alloc_traits_::allocate(impl_, other.size());
                impl_.cap_ += static_cast<difference_type_>(other.size());

                try
                {
                        for(auto& i : other)
                                detail::initialize_next(*this, std::move(*i));
                }
                catch(...)
                {
                        detail::destroy_elements(*this);
                        throw;
                }
        }

        // move assign:
        vector_storage& operator=(vector_storage&& other) noexcept(
                alloc_traits_::propagate_on_container_move_assignment::value ||
                alloc_traits_::is_always_equal::value)
        {
                if(alloc_traits_::propagate_on_container_move_assignment::value ||
                   alloc_traits_::is_always_equal::value ||
                   get_allocator_ref() == other.get_allocator_ref())
                {
                        detail::destroy_elements(*this);
                        deallocate();

                        impl_.swap(other.impl_);

                        if /*constexpr*/ (
                                alloc_traits_::propagate_on_container_move_assignment::value)
                                get_allocator_ref() = std::move(other.get_allocator_ref());

                        return *this;
                }

                detail::assign_n(*this, other.size(), std::make_move_iterator(other.begin()));

                detail::destroy_elements(other);
                other.deallocate();

                return *this;
        }

        // interface:
        allocator_type& get_allocator_ref() noexcept
        {
                return static_cast<allocator_type&>(impl_);
        }

        const allocator_type& get_allocator_ref() const noexcept
        {
                return static_cast<const allocator_type&>(impl_);
        }

        //
        void deallocate() noexcept
        {
                alloc_traits_::deallocate(impl_, impl_.beg_, capacity());
                impl_.beg_ = impl_.end_ = impl_.cap_ = pointer_{};
        }

        //
        template <typename... Args>
        void construct(pointer_ location, Args&&... args)
        {
                alloc_traits_::construct(
                        impl_, traits_::ptr_cast(location), std::forward<Args>(args)...);
        }

        void destroy(pointer_ location) noexcept
        {
                alloc_traits_::destroy(impl_, traits_::ptr_cast(location));
        }

        //
        pointer_ begin() noexcept
        {
                return impl_.beg_;
        }

        const_pointer_ begin() const noexcept
        {
                return impl_.beg_;
        }

        //
        pointer_ end() noexcept
        {
                return impl_.end_;
        }

        const_pointer_ end() const noexcept
        {
                return impl_.end_;
        }

        //
        bool reallocate(size_type_ n)
        {
                auto first = begin();
                reallocate_initialize_n_(n, impl_.end_ - impl_.beg_, [this, &first](auto i) {
                        this->construct(i, std::move_if_noexcept(*first)), (void)++first;
                });

                return true;
        }

        template <typename ForwardIterator>
        bool reallocate_assign(size_type_ n, ForwardIterator first)
        {
                reallocate_initialize_n_(
                        n, static_cast<difference_type_>(n),
                        [this, &first](auto i) { this->construct(i, *first), (void)++first; });

                return true;
        }

        //
        bool empty() const noexcept
        {
                return impl_.beg_ == impl_.end_;
        }

        bool full() const noexcept
        {
                return impl_.end_ == impl_.cap_;
        }

        //
        void set_size(size_type_ n) noexcept
        {
                impl_.end_ = impl_.beg_ + static_cast<difference_type_>(n);
        }

        void inc_size(size_type_ n) noexcept
        {
                impl_.end_ += static_cast<difference_type_>(n);
        }

        void dec_size(size_type_ n) noexcept
        {
                impl_.end_ -= static_cast<difference_type_>(n);
        }

        //
        size_type_ size() const noexcept
        {
                return static_cast<size_type_>(impl_.end_ - impl_.beg_);
        }

        size_type_ max_size() const noexcept
        {
                return alloc_traits_::max_size(impl_);
        }

        size_type_ capacity() const noexcept
        {
                return static_cast<size_type_>(impl_.cap_ - impl_.beg_);
        }

        //
        void swap(vector_storage& other) noexcept(
                alloc_traits_::propagate_on_container_swap::value ||
                alloc_traits_::is_always_equal::value)
        {
                impl_.swap(other.impl_);
                if(alloc_traits_::propagate_on_container_swap::value)
                        std::swap(get_allocator_ref(), other.get_allocator_ref());
        }

private:
        template <typename Initializer>
        void reallocate_initialize_n_(size_type_ sz, difference_type_ n, Initializer init)
        {
                if(sz > max_size() || sz < capacity())
                        throw std::length_error("");

                auto current_size = size();
                auto new_capacity = std::max(current_size + current_size, sz);
                new_capacity = (new_capacity < current_size || new_capacity > max_size())
                                       ? max_size()
                                       : new_capacity;

                auto ptr = alloc_traits_::allocate(impl_, new_capacity);
                auto first = ptr, last = first + n;

                try
                {
                        for(; first != last; ++first)
                                init(first);
                }
                catch(...)
                {
                        for_each_iter(ptr, first, [this](auto i) { this->destroy(i); });
                        alloc_traits_::deallocate(impl_, ptr, new_capacity);

                        throw;
                }

                detail::destroy_elements(*this);
                deallocate();

                impl_.beg_ = ptr;
                impl_.end_ = ptr + n;
                impl_.cap_ = ptr + static_cast<difference_type_>(new_capacity);
        }

        //
        implementation_ impl_;
};

//
} // namespace ecs

#endif // STORAGE_TYPES_H
