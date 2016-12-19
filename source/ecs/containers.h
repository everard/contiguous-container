// Copyright Ildus Nezametdinov 2016.
// Distributed under the Boost Software License, Version 1.0.
//(See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef CONTAINERS_H
#define CONTAINERS_H

#include "contiguous_container.h"

#include <memory>

namespace ecs
{
namespace detail
{
template <typename Storage, typename Initializer>
void initialize_elements(Storage& storage, Initializer init)
{
        using traits = storage_traits<Storage>;
        auto current = traits::begin(storage), last = traits::end(storage);

        try
        {
                for(; current != last; ++current)
                        init(storage, current);
        }
        catch(...)
        {
                for(auto i = traits::begin(storage); i != current; ++i)
                        traits::destroy(storage, i);

                throw;
        }
}

//
} // namespace detail

template <typename Storage>
struct allocator_aware_storage : Storage
{
        // requirements:
        //
        template <typename InputIterator>
        using check_input_iterator = std::enable_if_t<!std::is_integral<InputIterator>::value>;

        // types:
        //
        using base = Storage;
        using traits = storage_traits<base>;

        //
        using value_type = typename base::value_type;
        using allocator_type = typename base::allocator_type;
        using allocator_traits = typename std::allocator_traits<allocator_type>;

        //
        using pointer = typename allocator_traits::pointer;
        using const_pointer = typename allocator_traits::const_pointer;

        using size_type = typename allocator_traits::size_type;
        using difference_type = typename allocator_traits::difference_type;

        // friend declaration:
        //
        friend struct storage_traits<allocator_aware_storage>;

        // assertions:
        //
        static_assert(std::is_same<value_type, typename allocator_traits::value_type>::value);

        // construct/copy/destroy:
        //
        allocator_aware_storage() noexcept(noexcept(allocator_type{})) : base{}
        {
        }

        explicit allocator_aware_storage(const allocator_type& a) noexcept : base{a}
        {
        }

        explicit allocator_aware_storage(size_type n, const allocator_type& a = allocator_type{})
                : base{a, n}
        {
                detail::initialize_elements(
                        *this, [](auto& storage, auto i) { traits::construct(storage, i); });
        }

        allocator_aware_storage(size_type n, const value_type& x,
                                const allocator_type& a = allocator_type{})
                : base{a, n}
        {
                detail::initialize_elements(
                        *this, [&x](auto& storage, auto i) { traits::construct(storage, i, x); });
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
                : allocator_aware_storage{il.begin(), il.end(), a}
        {
        }

protected:
        ~allocator_aware_storage()
        {
                for_each_iter(traits::begin(*this), traits::end(*this),
                              [this](auto i) { traits::destroy(*this, i); });
        }

private:
        template <typename InputIterator>
        allocator_aware_storage(InputIterator first, InputIterator last, const allocator_type& a,
                                std::input_iterator_tag)
                : base{a}
        {
                for(auto i = traits::begin(*this); first != last; ++first, ++i)
                {
                        if(traits::full(*this))
                                traits::reallocate(*this, traits::capacity(*this) + 1);

                        traits::construct(*this, i, *first), traits::inc_size(*this);
                }
        }

        template <typename InputIterator>
        allocator_aware_storage(InputIterator first, InputIterator last, const allocator_type& a,
                                std::forward_iterator_tag)
                : base{a, static_cast<size_type>(std::distance(first, last))}
        {
                detail::initialize_elements(*this, [&first](auto& s, auto i) {
                        traits::construct(s, i, *first), ++first;
                });
        }
};

template <typename T, typename Allocator>
struct vector_storage
{
        // types:
        //
        using value_type = T;
        using allocator_type = Allocator;
        using allocator_traits = std::allocator_traits<allocator_type>;

        //
        using pointer = typename allocator_traits::pointer;
        using const_pointer = typename allocator_traits::const_pointer;

        using size_type = typename allocator_traits::size_type;
        using difference_type = typename allocator_traits::difference_type;

        //
        friend struct storage_traits<vector_storage>;
        using traits = storage_traits<vector_storage>;

protected:
        struct impl : allocator_type
        {
                impl() noexcept(noexcept(allocator_type{})) : allocator_type{}
                {
                }

                impl(const allocator_type& a) noexcept : allocator_type{a}
                {
                }

                ~impl()
                {
                        free_memory();
                }

                void allocate_memory(size_type n)
                {
                        beg_ = this->allocate(n);

                        end_ = cap_ = beg_;
                        cap_ += static_cast<difference_type>(n);
                }

                void free_memory() noexcept
                {
                        allocator_traits::deallocate(
                                *this, beg_, static_cast<size_type>(cap_ - beg_));

                        beg_ = end_ = cap_ = pointer{};
                }

                pointer beg_{}, end_{}, cap_{};
        };

        // construct/copy/destroy:
        //
        vector_storage() noexcept(noexcept(impl{})) : impl_{}
        {
        }

        vector_storage(const allocator_type& a) noexcept : impl_{a}
        {
        }

        vector_storage(const allocator_type& a, size_type n) : vector_storage{a}
        {
                impl_.allocate_memory(n);
        }

        ~vector_storage() = default;

        // interface:
        //
        template <typename... Args>
        void construct(pointer location, Args&&... args)
        {
                allocator_traits::construct(
                        impl_, traits::ptr_cast(location), std::forward<Args>(args)...);
        }

        void destroy(pointer location) noexcept
        {
                allocator_traits::destroy(impl_, traits::ptr_cast(location));
        }

        //
        pointer begin() noexcept
        {
                return impl_.beg_;
        }

        const_pointer begin() const noexcept
        {
                return impl_.beg_;
        }

        pointer end() noexcept
        {
                return impl_.end_;
        }

        const_pointer end() const noexcept
        {
                return impl_.end_;
        }

        //
        bool reallocate(size_type n)
        {
                if(n > max_size() || n < capacity())
                        throw std::length_error("");

                if(impl_.beg_ == pointer{})
                {
                        impl_.allocate_memory(n);
                        return true;
                }

                auto current_size = size();
                auto new_capacity = std::max(current_size + current_size, n);
                new_capacity = (new_capacity < current_size || new_capacity > max_size())
                                       ? max_size()
                                       : new_capacity;

                auto ptr = allocator_traits::allocate(impl_, new_capacity);
                auto first = ptr, last = first;

                try
                {
                        for_each_iter(begin(), end(), first, [this, &last](auto i, auto j) {
                                this->construct(j, std::move_if_noexcept(*i)), ++last;
                        });
                }
                catch(...)
                {
                        for_each_iter(first, last, [this](auto i) { this->destroy(i); });
                        allocator_traits::deallocate(impl_, ptr, new_capacity);

                        throw;
                }

                for_each_iter(begin(), end(), [this](auto i) { this->destroy(i); });
                allocator_traits::deallocate(impl_, impl_.beg_, capacity());

                impl_.beg_ = ptr;
                impl_.end_ = ptr + static_cast<difference_type>(current_size);
                impl_.cap_ = ptr + static_cast<difference_type>(new_capacity);

                return true;
        }

        bool empty() const noexcept
        {
                return impl_.beg_ == impl_.end_;
        }

        bool full() const noexcept
        {
                return impl_.end_ == impl_.cap_;
        }

        //
        void set_size(size_type n) noexcept
        {
                impl_.end_ = impl_.beg_ + static_cast<difference_type>(n);
        }

        void inc_size(size_type n) noexcept
        {
                impl_.end_ += static_cast<difference_type>(n);
        }

        void dec_size(size_type n) noexcept
        {
                impl_.end_ -= static_cast<difference_type>(n);
        }

        //
        size_type size() const noexcept
        {
                return static_cast<size_type>(impl_.end_ - impl_.beg_);
        }

        size_type max_size() const noexcept
        {
                return allocator_traits::max_size(impl_);
        }

        size_type capacity() const noexcept
        {
                return static_cast<size_type>(impl_.cap_ - impl_.beg_);
        }

private:
        impl impl_;
};

template <typename T, typename Allocator = std::allocator<T>>
using vector = contiguous_container<allocator_aware_storage<vector_storage<T, Allocator>>>;

//
} // namespace ecs

#endif // CONTAINERS_H
