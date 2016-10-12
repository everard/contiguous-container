// Copyright Ildus Nezametdinov 2016.
// Distributed under the Boost Software License, Version 1.0.
//(See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef CONTIGUOUS_CONTAINER_H
#define CONTIGUOUS_CONTAINER_H

#include <experimental/tuple>
#include <initializer_list>
#include <type_traits>
#include <algorithm>
#include <stdexcept>

#include <iterator>
#include <utility>

#include <cassert>
#include <cstddef>
#include <memory>

#include "storage_traits.h"

namespace std::experimental
{
inline namespace detail
{
template <typename Tuple, std::size_t... I>
constexpr auto tuple_slice(Tuple&& t, std::index_sequence<I...>)
{
        return std::make_tuple(std::get<I>(std::forward<Tuple>(t))...);
}

template <typename InputIterator, typename... Rest>
constexpr auto for_each_iter(InputIterator first, InputIterator last, Rest&&... rest)
{
        static_assert(sizeof...(Rest) != 0);
        auto args = std::forward_as_tuple(first, std::forward<Rest>(rest)...);

        auto&& f = std::get<sizeof...(Rest)>(args);
        auto iterators = tuple_slice(args, std::make_index_sequence<sizeof...(Rest)>{});

        while(std::get<0>(iterators) != last)
        {
                std::experimental::apply(f, iterators);
                std::experimental::apply([](auto&... i) { ((void)++i, ...); }, iterators);
        }

        return iterators;
}

//
} // namespace detail

//
template <typename Storage>
struct contiguous_container : Storage
{
        // requirements:
        //
        template <typename ForwardIterator>
        using check_forward_iterator = std::enable_if_t<!std::is_integral<ForwardIterator>::value>;

        // types:
        //
        using traits = storage_traits<Storage>;
        using value_type = typename Storage::value_type;

        //
        using pointer = value_type*;
        using const_pointer = const value_type*;

        using reference = value_type&;
        using const_reference = const value_type&;

        //
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        //
        using iterator = pointer;
        using const_iterator = const_pointer;

        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        // construct/copy/destroy:
        //
        using Storage::Storage;

        constexpr auto& operator=(std::initializer_list<value_type> il)
        {
                assign(il);
                return *this;
        }

        //
        template <typename ForwardIterator, typename = check_forward_iterator<ForwardIterator>>
        constexpr auto assign(ForwardIterator first, ForwardIterator last)
        {
                auto n = static_cast<size_type>(std::distance(first, last));
                return assign(n, [&first]() -> auto& { return *first++; });
        }

        constexpr auto assign(std::initializer_list<value_type> il)
        {
                return assign(il.begin(), il.end());
        }

        constexpr auto assign(size_type n, const_reference u)
        {
                return assign(n, [&u]() -> auto& { return u; });
        }

        // iterators:
        //
        constexpr auto begin() noexcept
        {
                return traits::begin(*this);
        }

        constexpr auto begin() const noexcept
        {
                return traits::begin(*this);
        }

        constexpr auto end() noexcept
        {
                return traits::end(*this);
        }

        constexpr auto end() const noexcept
        {
                return traits::end(*this);
        }

        //
        constexpr auto rbegin() noexcept
        {
                return reverse_iterator{end()};
        }

        constexpr auto rbegin() const noexcept
        {
                return const_reverse_iterator{end()};
        }

        constexpr auto rend() noexcept
        {
                return reverse_iterator{begin()};
        }

        constexpr auto rend() const noexcept
        {
                return const_reverse_iterator{begin()};
        }

        //
        constexpr auto cbegin() const noexcept
        {
                return begin();
        }

        constexpr auto cend() const noexcept
        {
                return end();
        }

        //
        constexpr auto crbegin() const noexcept
        {
                return rbegin();
        }

        constexpr auto crend() const noexcept
        {
                return rend();
        }

        // capacity:
        //
        constexpr bool empty() const noexcept
        {
                return traits::empty(*this);
        }

        constexpr bool full() const noexcept
        {
                return traits::full(*this);
        }

        //
        constexpr auto size() const noexcept
        {
                return static_cast<size_type>(traits::size(*this));
        }

        constexpr auto max_size() const noexcept
        {
                return traits::max_size(*this);
        }

        constexpr auto capacity() const noexcept
        {
                return traits::capacity(*this);
        }

        // element access:
        //
        constexpr auto& operator[](size_type i) noexcept
        {
                return data()[i];
        }

        constexpr auto& operator[](size_type i) const noexcept
        {
                return data()[i];
        }

        //
        constexpr auto& at(size_type i)
        {
                if(i >= size())
                        throw std::out_of_range("");

                return data()[i];
        }

        constexpr auto& at(size_type i) const
        {
                if(i >= size())
                        throw std::out_of_range("");

                return data()[i];
        }

        //
        constexpr auto& front() noexcept
        {
                assert(!empty());
                return *begin();
        }

        constexpr auto& front() const noexcept
        {
                assert(!empty());
                return *begin();
        }

        constexpr auto& back() noexcept
        {
                assert(!empty());
                return *(--end());
        }

        constexpr auto& back() const noexcept
        {
                assert(!empty());
                return *(--end());
        }

        //
        constexpr auto data() noexcept
        {
                return begin();
        }

        constexpr auto data() const noexcept
        {
                return begin();
        }

        // modifiers:
        //
        template <typename... Args>
        constexpr auto emplace_back(Args&&... args)
        {
                if(full() && !traits::reserve(*this, capacity() + 1))
                {
                        if /*constexpr*/ (traits::no_exceptions::value)
                                return end();
                        else
                                throw std::runtime_error("");
                }

                auto position = traits::construct_at(*this, end(), std::forward<Args>(args)...);
                return (void)++traits::size(*this), position;
        }

        constexpr auto push_back(const_reference x)
        {
                return emplace_back(x);
        }

        constexpr auto push_back(value_type&& x)
        {
                return emplace_back(std::move(x));
        }

        constexpr void pop_back() noexcept
        {
                assert(!empty());
                (void)--traits::size(*this), traits::destroy_at(*this, end());
        }

        //
        template <typename... Args>
        constexpr auto emplace(const_iterator position, Args&&... args)
        {
                if(position == end())
                        return emplace_back(std::forward<Args>(args)...);

                value_type x{std::forward<Args>(args)...};
                return grow_at(cast(position), 1, [&x]() -> auto&& { return std::move(x); });
        }

        constexpr auto insert(const_iterator position, const_reference x)
        {
                return emplace(position, x);
        }

        constexpr auto insert(const_iterator position, value_type&& x)
        {
                return emplace(position, std::move(x));
        }

        //
        template <typename ForwardIterator, typename = check_forward_iterator<ForwardIterator>>
        constexpr auto insert(const_iterator position, ForwardIterator first, ForwardIterator last)
        {
                auto n = static_cast<difference_type>(std::distance(first, last));
                return grow_at(cast(position), n, [&first]() -> auto& { return *first++; });
        }

        constexpr auto insert(const_iterator position, std::initializer_list<value_type> il)
        {
                return insert(position, il.begin(), il.end());
        }

        constexpr auto insert(const_iterator position, size_type n, const_reference x)
        {
                return grow_at(cast(position), static_cast<difference_type>(n),
                               [&x]() -> auto& { return x; });
        }

        //
        constexpr auto erase(const_iterator position)
        {
                return shrink_at(cast(position));
        }

        constexpr auto erase(const_iterator first, const_iterator last)
        {
                return shrink_at(cast(first), last - first);
        }

        //
        constexpr void clear() noexcept
        {
                destroy(begin(), end());
                traits::size(*this) = 0;
        }

private:
        template <typename Fill>
        constexpr auto grow_at(iterator position, difference_type n, Fill fill)
        {
                if(n == 0)
                        return position;

                auto adjusted_size = static_cast<size_type>(n) + size();
                if(adjusted_size > max_size() || adjusted_size < size())
                        return end();

                if(adjusted_size > capacity())
                {
                        auto saved_index = position - begin();

                        if(!traits::reserve(*this, adjusted_size))
                        {
                                if /*constexpr*/ (traits::no_exceptions::value)
                                        return end();
                                else
                                        throw std::runtime_error("");
                        }

                        position = begin() + saved_index;
                }

                // relocate elements
                auto m = std::min(n, end() - position);
                auto last = end(), first_to_relocate = last - m;

                for_each_iter(first_to_relocate, last, first_to_relocate + n, [&](auto i, auto j) {
                        traits::construct_at(*this, j, std::move(*i));
                });
                std::move_backward(position, first_to_relocate, last);

                // insert n elements at position
                for_each_iter(std::generate_n(position, m, fill), position + n,
                              [&](auto i) { traits::construct_at(*this, i, fill()); });

                traits::size(*this) = adjusted_size;
                return position;
        }

        constexpr auto shrink_at(iterator position, difference_type n = 1)
        {
                if(n != 0)
                {
                        destroy(std::move(position + n, end(), position), end());
                        traits::size(*this) -= static_cast<size_type>(n);
                }

                return position;
        }

        template <typename Fill>
        constexpr auto assign(size_type n, Fill fill)
        {
                if(n > capacity() && !traits::reserve(*this, n))
                {
                        if /*constexpr*/ (traits::no_exceptions::value)
                                return false;
                        else
                                throw std::runtime_error("");
                }

                if(n <= size())
                        destroy(std::generate_n(begin(), n, fill), end());
                else
                        for_each_iter(std::generate_n(begin(), size(), fill), begin() + n,
                                      [&](auto i) { traits::construct_at(*this, i, fill()); });

                traits::size(*this) = n;
                return true;
        }

        //
        constexpr void destroy(iterator first, iterator last) noexcept
        {
                for_each_iter(first, last, [&](auto i) { traits::destroy_at(*this, i); });
        }

        constexpr auto cast(const_iterator position) noexcept
        {
                return begin() + (position - cbegin());
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

//
} // namespace std::experimental

#endif // CONTIGUOUS_CONTAINER_H
