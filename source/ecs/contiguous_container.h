// Copyright Ildus Nezametdinov 2017.
// Distributed under the Boost Software License, Version 1.0.
//(See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef CONTIGUOUS_CONTAINER_H
#define CONTIGUOUS_CONTAINER_H

#include "common_storage_types.h"

#include <initializer_list>
#include <functional>
#include <cassert>

namespace ecs
{
template <typename Storage>
struct contiguous_container : Storage
{
        // types:
        using traits = ecs::storage_traits<Storage>;
        using value_type = typename traits::value_type;

        using pointer = typename traits::pointer;
        using const_pointer = typename traits::const_pointer;

        using reference = value_type&;
        using const_reference = const value_type&;

        using size_type = typename traits::size_type;
        using difference_type = typename traits::difference_type;

        using iterator = pointer;
        using const_iterator = const_pointer;

        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        // construct/copy/destroy:
        using Storage::Storage;

        constexpr contiguous_container& operator=(std::initializer_list<value_type> il)
        {
                assign(il);
                return *this;
        }

        template <typename InputIterator, typename = check_input_iterator<InputIterator>>
        constexpr bool assign(InputIterator first, InputIterator last)
        {
                return assign_(first, last,
                               typename std::iterator_traits<InputIterator>::iterator_category{});
        }

        constexpr bool assign(std::initializer_list<value_type> il)
        {
                return assign_(il.begin(), il.end(), std::forward_iterator_tag{});
        }

        constexpr bool assign(size_type n, const_reference u)
        {
                return assign_n_(n, make_identity_iterator(std::addressof(u)));
        }

        // iterators:
        constexpr iterator begin() noexcept
        {
                return traits::begin(*this);
        }

        constexpr const_iterator begin() const noexcept
        {
                return traits::begin(*this);
        }

        constexpr iterator end() noexcept
        {
                return traits::end(*this);
        }

        constexpr const_iterator end() const noexcept
        {
                return traits::end(*this);
        }

        //
        constexpr reverse_iterator rbegin() noexcept
        {
                return reverse_iterator{end()};
        }

        constexpr const_reverse_iterator rbegin() const noexcept
        {
                return const_reverse_iterator{end()};
        }

        constexpr reverse_iterator rend() noexcept
        {
                return reverse_iterator{begin()};
        }

        constexpr const_reverse_iterator rend() const noexcept
        {
                return const_reverse_iterator{begin()};
        }

        //
        constexpr const_iterator cbegin() const noexcept
        {
                return begin();
        }

        constexpr const_iterator cend() const noexcept
        {
                return end();
        }

        //
        constexpr const_reverse_iterator crbegin() const noexcept
        {
                return rbegin();
        }

        constexpr const_reverse_iterator crend() const noexcept
        {
                return rend();
        }

        // capacity:
        constexpr bool empty() const noexcept
        {
                return traits::empty(*this);
        }

        constexpr bool full() const noexcept
        {
                return traits::full(*this);
        }

        //
        constexpr size_type size() const noexcept
        {
                return traits::size(*this);
        }

        constexpr size_type max_size() const noexcept
        {
                return traits::max_size(*this);
        }

        constexpr size_type capacity() const noexcept
        {
                return traits::capacity(*this);
        }

        //
        constexpr bool resize(size_type sz)
        {
                return resize_(sz);
        }

        constexpr bool resize(size_type sz, const_reference x)
        {
                return resize_(sz, x);
        }

        constexpr bool reserve(size_type n)
        {
                if(n <= capacity())
                        return true;

                return traits::reallocate(*this, n);
        }

        // element access:
        constexpr reference operator[](size_type i) noexcept
        {
                assert(i < size());
                return data()[i];
        }

        constexpr const_reference operator[](size_type i) const noexcept
        {
                assert(i < size());
                return data()[i];
        }

        //
        constexpr reference at(size_type i)
        {
                if(i >= size())
                        throw std::out_of_range("contiguous_container::at");

                return data()[i];
        }

        constexpr const_reference at(size_type i) const
        {
                if(i >= size())
                        throw std::out_of_range("contiguous_container::at");

                return data()[i];
        }

        //
        constexpr reference front() noexcept
        {
                assert(!empty());
                return *begin();
        }

        constexpr const_reference front() const noexcept
        {
                assert(!empty());
                return *begin();
        }

        constexpr reference back() noexcept
        {
                assert(!empty());
                return *(--end());
        }

        constexpr const_reference back() const noexcept
        {
                assert(!empty());
                return *(--end());
        }

        // data access:
        constexpr value_type* data() noexcept
        {
                return traits::ptr_cast(traits::begin(*this));
        }

        constexpr const value_type* data() const noexcept
        {
                return traits::ptr_cast(traits::begin(*this));
        }

        // modifiers:
        template <typename... Args>
        constexpr iterator emplace_back(Args&&... args)
        {
                if(full() && !traits::reallocate(*this, capacity() + 1))
                        return end();

                auto position = traits::construct(*this, end(), std::forward<Args>(args)...);
                return traits::inc_size(*this), position;
        }

        constexpr iterator push_back(const_reference x)
        {
                return emplace_back(x);
        }

        constexpr iterator push_back(value_type&& x)
        {
                return emplace_back(std::move(x));
        }

        constexpr void pop_back() noexcept
        {
                assert(!empty());
                traits::dec_size(*this), traits::destroy(*this, end());
        }

        //
        template <typename... Args>
        constexpr iterator emplace(const_iterator position, Args&&... args)
        {
                assert(iter_check_(position));

                if(position == end())
                        return emplace_back(std::forward<Args>(args)...);

                value_type x{std::forward<Args>(args)...};
                return insert_n_(
                        iter_cast_(position), 1, std::make_move_iterator(std::addressof(x)));
        }

        constexpr iterator insert(const_iterator position, const_reference x)
        {
                return emplace(position, x);
        }

        constexpr iterator insert(const_iterator position, value_type&& x)
        {
                return emplace(position, std::move(x));
        }

        //
        template <typename InputIterator, typename = check_input_iterator<InputIterator>>
        constexpr iterator insert(const_iterator position, InputIterator first, InputIterator last)
        {
                assert(iter_check_(position));
                return insert_(position, first, last,
                               typename std::iterator_traits<InputIterator>::iterator_category{});
        }

        constexpr iterator insert(const_iterator position, std::initializer_list<value_type> il)
        {
                assert(iter_check_(position));
                return insert_(position, il.begin(), il.end(), std::forward_iterator_tag{});
        }

        constexpr iterator insert(const_iterator position, size_type n, const_reference x)
        {
                assert(iter_check_(position));
                return insert_n_(iter_cast_(position), static_cast<difference_type>(n),
                                 make_identity_iterator(std::addressof(x)));
        }

        //
        constexpr iterator erase(const_iterator position)
        {
                assert(iter_check_(position) &&
                       std::not_equal_to<const_iterator>{}(position, end()));
                return erase_n_(iter_cast_(position));
        }

        constexpr iterator erase(const_iterator first, const_iterator last)
        {
                assert(iter_check_(first) && iter_check_(last) &&
                       std::less_equal<const_iterator>{}(first, last));
                return erase_n_(iter_cast_(first), last - first);
        }

        //
        constexpr void clear() noexcept
        {
                destroy_range_(begin(), end());
                traits::set_size(*this, 0);
        }

        constexpr void swap(contiguous_container& x) noexcept(noexcept(traits::swap(x, x)))
        {
                traits::swap(*this, x);
        }

private:
        template <typename InputIterator>
        constexpr bool assign_(InputIterator first, InputIterator last, std::input_iterator_tag)
        {
                auto assigned = begin(), sentinel = end();
                for(; first != last && assigned != sentinel; ++first, (void)++assigned)
                        *assigned = *first;

                if(first == last)
                {
                        traits::dec_size(*this, static_cast<size_type>(sentinel - assigned));
                        destroy_range_(assigned, sentinel);
                        return true;
                }

                for(iterator p{}; first != last; ++first)
                        if((void)(p = emplace_back(*first)), p == end())
                                return false;

                return true;
        }

        template <typename ForwardIterator>
        constexpr bool assign_(ForwardIterator first, ForwardIterator last,
                               std::forward_iterator_tag)
        {
                return assign_n_(static_cast<size_type>(std::distance(first, last)), first);
        }

        template <typename ForwardIterator>
        constexpr bool assign_n_(size_type n, ForwardIterator first)
        {
                if(n > capacity())
                        return traits::reallocate_assign(*this, n, first);

                traits::assign(*this, n, first);
                return true;
        }

        //
        template <typename... Args>
        constexpr bool resize_(size_type sz, const Args&... x)
        {
                if(sz > capacity() && !traits::reallocate(*this, sz))
                        return false;

                auto target = begin() + static_cast<difference_type>(sz);
                if(sz < size())
                {
                        destroy_range_(target, end());
                        traits::set_size(*this, sz);
                }
                else
                {
                        for_each_iter(end(), target, [this, &x...](auto i) {
                                traits::construct(*this, i, x...), traits::inc_size(*this);
                        });
                }

                return true;
        }

        //
        template <typename InputIterator>
        constexpr iterator insert_(const_iterator position, InputIterator first, InputIterator last,
                                   std::input_iterator_tag)
        {
                auto index = position - begin();

                for(auto p = iter_cast_(position); first != last; ++first, (void)++p)
                        if((void)(p = emplace(p, *first)), p == end())
                                return end();

                return begin() + index;
        }

        template <typename ForwardIterator>
        constexpr iterator insert_(const_iterator position, ForwardIterator first,
                                   ForwardIterator last, std::forward_iterator_tag)
        {
                return insert_n_(iter_cast_(position),
                                 static_cast<difference_type>(std::distance(first, last)), first);
        }

        template <typename ForwardIterator>
        constexpr iterator insert_n_(iterator position, difference_type n, ForwardIterator first)
        {
                if(n == 0)
                        return position;

                auto sz = static_cast<size_type>(n) + size();
                if(sz > capacity() || sz < size())
                {
                        auto index = position - begin();
                        if(!traits::reallocate(*this, sz))
                                return end();

                        position = begin() + index;
                }

                // relocate elements
                auto m = std::min(n, end() - position);
                auto last = end(), first_to_relocate = last - m, first_to_construct = position + m;

                if(m != n)
                {
                        auto mid = first;
                        std::advance(mid, m);

                        for_each_iter(first_to_construct, position + n, [this, &mid](auto i) {
                                traits::construct(*this, i, *mid), traits::inc_size(*this), ++mid;
                        });
                }

                for_each_iter(
                        first_to_relocate, last, first_to_relocate + n, [this](auto i, auto j) {
                                traits::construct(*this, j, std::move(*i)), traits::inc_size(*this);
                        });

                std::move_backward(position, first_to_relocate, last);
                for_each_iter(position, first_to_construct, first, [](auto i, auto j) { *i = *j; });

                return position;
        }

        //
        constexpr iterator erase_n_(iterator position, difference_type n = 1)
        {
                if(n != 0)
                {
                        destroy_range_(std::move(position + n, end(), position), end());
                        traits::dec_size(*this, static_cast<size_type>(n));
                }

                return position;
        }

        //
        constexpr void destroy_range_(iterator first, iterator last) noexcept
        {
                for_each_iter(first, last, [this](auto i) { traits::destroy(*this, i); });
        }

        constexpr iterator iter_cast_(const_iterator position) noexcept
        {
                return begin() + (position - cbegin());
        }

        constexpr bool iter_check_(const_iterator position) noexcept
        {
                using compare = std::less_equal<const_iterator>;
                return compare{}(begin(), position) && compare{}(position, end());
        }
};

//
template <typename Storage>
constexpr bool operator==(const contiguous_container<Storage>& lhs,
                          const contiguous_container<Storage>& rhs)
{
        return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Storage>
constexpr bool operator!=(const contiguous_container<Storage>& lhs,
                          const contiguous_container<Storage>& rhs)
{
        return !(lhs == rhs);
}

//
template <typename Storage>
constexpr bool operator<(const contiguous_container<Storage>& lhs,
                         const contiguous_container<Storage>& rhs)
{
        return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Storage>
constexpr bool operator>(const contiguous_container<Storage>& lhs,
                         const contiguous_container<Storage>& rhs)
{
        return rhs < lhs;
}

template <typename Storage>
constexpr bool operator<=(const contiguous_container<Storage>& lhs,
                          const contiguous_container<Storage>& rhs)
{
        return !(lhs > rhs);
}

template <typename Storage>
constexpr bool operator>=(const contiguous_container<Storage>& lhs,
                          const contiguous_container<Storage>& rhs)
{
        return !(lhs < rhs);
}

//
template <typename Storage>
constexpr void swap(contiguous_container<Storage>& lhs,
                    contiguous_container<Storage>& rhs) noexcept(noexcept(lhs.swap(rhs)))
{
        lhs.swap(rhs);
}

// common container types:
template <typename T, std::size_t N>
using inplace_vector = contiguous_container<inplace_storage<T, N>>;

template <typename T, typename Allocator = std::allocator<T>>
using vector = contiguous_container<allocator_aware_storage<vector_storage<T, Allocator>>>;

//
} // namespace ecs

#endif // CONTIGUOUS_CONTAINER_H
