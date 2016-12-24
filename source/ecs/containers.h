// Copyright Ildus Nezametdinov 2016.
// Distributed under the Boost Software License, Version 1.0.
//(See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef CONTAINERS_H
#define CONTAINERS_H

#include "contiguous_container.h"

namespace ecs
{
namespace detail
{
template <typename Storage, typename Initializer>
void initialize_elements(Storage& storage, Initializer init)
{
        using traits = storage_traits<Storage>;
        auto first = traits::begin(storage), last = traits::end(storage);

        try
        {
                for(; first != last; ++first)
                        init(storage, first);
        }
        catch(...)
        {
                for(auto i = traits::begin(storage); i != first; ++i)
                        traits::destroy(storage, i);

                throw;
        }
}

template <typename Storage, typename ForwardIterator>
Storage& assign_n(Storage& storage, typename storage_traits<Storage>::size_type n,
                  ForwardIterator first)
{
        using traits = storage_traits<Storage>;

        if(n > traits::capacity(storage))
        {
                storage.reallocate_fill_n(n, first);
                return storage;
        }

        auto d = static_cast<typename traits::difference_type>(n);
        if(n <= traits::size(storage))
        {
                for_each_iter(std::copy_n(first, n, traits::begin(storage)), traits::end(storage),
                              [&storage](auto i) { traits::destroy(storage, i); });
                traits::set_size(storage, n);
        }
        else
        {
                auto mid = first;
                std::advance(mid, traits::size(storage));

                for_each_iter(std::copy(first, mid, traits::begin(storage)),
                              traits::begin(storage) + d, [&storage, &mid](auto i) {
                                      traits::construct(storage, i, *mid),
                                              traits::inc_size(storage), (void)++mid;
                              });
        }

        return storage;
}

//
} // namespace detail

template <typename Storage>
struct allocator_aware_storage : Storage
{
        // types:
        //
        using base = Storage;
        using traits = storage_traits<base>;

        //
        using typename base::value_type;
        using typename base::allocator_type;

        //
        using allocator_traits = std::allocator_traits<allocator_type>;

        //
        using pointer = typename allocator_traits::pointer;
        using const_pointer = typename allocator_traits::const_pointer;

        using size_type = typename allocator_traits::size_type;
        using difference_type = typename allocator_traits::difference_type;

        // friend declarations:
        //
        friend struct storage_traits<allocator_aware_storage>;

        template <typename S, typename ForwardIterator>
        friend Storage& detail::assign_n(S&, typename storage_traits<S>::size_type,
                                         ForwardIterator);

        // assertions:
        //
        static_assert(std::is_same<value_type, typename allocator_traits::value_type>::value);

        // construct:
        //
        allocator_aware_storage() noexcept(noexcept(allocator_type{})) : base{}
        {
        }

        explicit allocator_aware_storage(const allocator_type& a) noexcept : base{a}
        {
        }

        explicit allocator_aware_storage(size_type n, const allocator_type& a = allocator_type{})
                : base{n, a}
        {
                detail::initialize_elements(
                        *this, [](auto& storage, auto i) { traits::construct(storage, i); });
        }

        allocator_aware_storage(size_type n, const value_type& x,
                                const allocator_type& a = allocator_type{})
                : base{n, a}
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

        // copy/move construct:
        //
        allocator_aware_storage(const allocator_aware_storage& other)
                : allocator_aware_storage{
                          other, allocator_traits::select_on_container_copy_construction(
                                         other.get_allocator_ref())}
        {
        }

        allocator_aware_storage(allocator_aware_storage&&) = default;

        //
        allocator_aware_storage(const allocator_aware_storage& other, const allocator_type& a)
                : base{traits::size(other), a}
        {
                detail::initialize_elements(
                        *this, [first = traits::begin(other)](auto& storage, auto i) mutable {
                                traits::construct(storage, i, *first), (void)++first;
                        });
        }

        allocator_aware_storage(allocator_aware_storage&& other, const allocator_type& a)
                : base{std::move(other), a}
        {
        }

        // copy/move assign:
        //
        allocator_aware_storage& operator=(const allocator_aware_storage& other)
        {
                if(this == std::addressof(other))
                        return *this;

                // copy allocator if needed
                if /*constexpr*/ (allocator_traits::propagate_on_container_copy_assignment::value)
                {
                        // deallocate memory if allocators are not equal
                        if(!allocator_traits::is_always_equal::value &&
                           this->get_allocator_ref() != other.get_allocator_ref())
                        {
                                destroy_range(traits::begin(*this), traits::end(*this));
                                this->deallocate();
                        }

                        this->get_allocator_ref() = other.get_allocator_ref();
                }

                return detail::assign_n(*this, traits::size(other), traits::begin(other));
        }

        allocator_aware_storage& operator=(allocator_aware_storage&&) = default;

protected: //
        // destroy:
        //
        ~allocator_aware_storage()
        {
                destroy_range(traits::begin(*this), traits::end(*this));
        }

private:
        template <typename InputIterator>
        allocator_aware_storage(InputIterator first, InputIterator last, const allocator_type& a,
                                std::input_iterator_tag)
                : base{a}
        {
                for(; first != last; ++first)
                {
                        if(traits::full(*this))
                                traits::reallocate(*this, traits::capacity(*this) + 1);

                        traits::construct(*this, traits::end(*this), *first);
                        traits::inc_size(*this);
                }
        }

        template <typename InputIterator>
        allocator_aware_storage(InputIterator first, InputIterator last, const allocator_type& a,
                                std::forward_iterator_tag)
                : base{static_cast<size_type>(std::distance(first, last)), a}
        {
                detail::initialize_elements(*this, [&first](auto& storage, auto i) {
                        traits::construct(storage, i, *first), ++first;
                });
        }

        //
        void destroy_range(pointer first, pointer last) noexcept
        {
                for_each_iter(first, last, [this](auto i) { traits::destroy(*this, i); });
        }
};

template <typename T, typename Allocator>
struct vector_storage
{
        // types:
        //
        using value_type = T;
        using allocator_type = Allocator;

        //
        using traits = storage_traits<vector_storage>;
        using allocator_traits = std::allocator_traits<allocator_type>;

        //
        using pointer = typename allocator_traits::pointer;
        using const_pointer = typename allocator_traits::const_pointer;

        using size_type = typename allocator_traits::size_type;
        using difference_type = typename allocator_traits::difference_type;

        // friend declarations:
        //
        friend struct storage_traits<vector_storage>;

        //
        template <typename Storage, typename ForwardIterator>
        friend Storage& detail::assign_n(Storage&, typename storage_traits<Storage>::size_type,
                                         ForwardIterator);

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

                impl(const impl&) = delete;
                impl(impl&&) = default;

                impl& operator=(const impl&) = delete;
                impl& operator=(impl&&) = default;

                //
                void allocate_memory(size_type n)
                {
                        beg_ = this->allocate(n);
                        end_ = cap_ = beg_ + static_cast<difference_type>(n);
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

        vector_storage(size_type n, const allocator_type& a) : vector_storage{a}
        {
                impl_.allocate_memory(n);
        }

        //
        ~vector_storage() = default;

        //
        vector_storage(const vector_storage&) = delete;
        vector_storage(vector_storage&& other) noexcept : impl_{std::move(other.impl_)}
        {
                other.impl_.beg_ = other.impl_.end_ = other.impl_.cap_ = pointer{};
        }

        vector_storage(vector_storage&& other,
                       const allocator_type& a) noexcept(allocator_traits::is_always_equal::value)
                : impl_{a}
        {
                if(get_allocator_ref() == other.get_allocator_ref())
                {
                        std::swap(impl_, other.impl_);
                        return;
                }

                // TODO
        }

        // copy/move assign
        vector_storage& operator=(const vector_storage&) = delete;
        vector_storage& operator=(vector_storage&& other) noexcept(
                allocator_traits::propagate_on_container_move_assignment::value ||
                allocator_traits::is_always_equal::value)
        {
                if /*constexpr*/ (allocator_traits::propagate_on_container_move_assignment::value ||
                                  allocator_traits::is_always_equal::value)
                        return move_storage(std::move(other));

                if(get_allocator_ref() == other.get_allocator_ref())
                        return move_storage(std::move(other));

                detail::assign_n(*this, other.size(), std::make_move_iterator(other.begin()));

                for_each_iter(other.begin(), other.end(), [&other](auto i) { other.destroy(i); });
                other.deallocate();

                return *this;
        }

        // interface:
        //
        allocator_type& get_allocator_ref() noexcept
        {
                return static_cast<allocator_type&>(impl_);
        }

        const allocator_type& get_allocator_ref() const noexcept
        {
                return static_cast<const allocator_type&>(impl_);
        }

        void deallocate() noexcept
        {
                impl_.free_memory();
        }

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
        template <typename ForwardIterator>
        void reallocate_fill_n(size_type n, ForwardIterator first)
        {
                reallocate_initialize_n(n, static_cast<difference_type>(n), [this, &first](auto i) {
                        this->construct(i, *first), (void)++first;
                });
        }

        bool reallocate(size_type n)
        {
                reallocate_initialize_n(
                        n, impl_.end_ - impl_.beg_, [ this, first = begin() ](auto i) mutable {
                                this->construct(i, std::move_if_noexcept(*first)), (void)++first;
                        });

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
        template <typename Initializer>
        void reallocate_initialize_n(size_type sz, difference_type n, Initializer init)
        {
                if(sz > max_size() || sz < capacity())
                        throw std::length_error("");

                auto current_size = size();
                auto new_capacity = std::max(current_size + current_size, sz);
                new_capacity = (new_capacity < current_size || new_capacity > max_size())
                                       ? max_size()
                                       : new_capacity;

                auto ptr = allocator_traits::allocate(impl_, new_capacity);
                auto first = ptr, last = first + n;

                try
                {
                        for_each_iter(
                                first, last, [&init, &first](auto i) { init(i), (void)++first; });
                }
                catch(...)
                {
                        for_each_iter(ptr, first, [this](auto i) { this->destroy(i); });
                        allocator_traits::deallocate(impl_, ptr, new_capacity);

                        throw;
                }

                for_each_iter(begin(), end(), [this](auto i) { this->destroy(i); });
                allocator_traits::deallocate(impl_, impl_.beg_, capacity());

                impl_.beg_ = ptr;
                impl_.end_ = ptr + n;
                impl_.cap_ = ptr + static_cast<difference_type>(new_capacity);
        }

        vector_storage& move_storage(vector_storage&& other) noexcept
        {
                for_each_iter(begin(), end(), [this](auto i) { this->destroy(i); });
                deallocate();

                std::swap(impl_.beg_, other.impl_.beg_);
                std::swap(impl_.end_, other.impl_.end_);
                std::swap(impl_.cap_, other.impl_.cap_);

                if /*constexpr*/ (allocator_traits::propagate_on_container_move_assignment::value)
                        get_allocator_ref() = std::move(other.get_allocator_ref());

                return *this;
        }

        //
        impl impl_;
};

template <typename T, typename Allocator = std::allocator<T>>
using vector = contiguous_container<allocator_aware_storage<vector_storage<T, Allocator>>>;

//
} // namespace ecs

#endif // CONTAINERS_H
