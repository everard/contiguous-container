// Copyright Ildus Nezametdinov 2016.
// Distributed under the Boost Software License, Version 1.0.
//(See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef UTILITY_H
#define UTILITY_H

#include <experimental/tuple>
#include <type_traits>

#include <algorithm>
#include <iterator>

#include <cstddef>
#include <utility>
#include <memory>

namespace ecs
{
// additional tuple creation function:
template <typename Tuple, std::size_t... I>
constexpr auto tuple_slice(Tuple&& t, std::index_sequence<I...>)
{
        return std::make_tuple(std::get<I>(std::forward<Tuple>(t))...);
}

// for_each_iter algorithm implementation:
template <typename Countable, typename... Rest>
constexpr auto for_each_iter(Countable first, Countable last, Rest&&... rest)
{
        static_assert(sizeof...(Rest) != 0);
        auto args = std::forward_as_tuple(first, std::forward<Rest>(rest)...);

        auto&& f = std::get<sizeof...(Rest)>(args);
        auto zip = tuple_slice(args, std::make_index_sequence<sizeof...(Rest)>{});

        while(std::get<0>(zip) != last)
        {
                std::experimental::apply(f, zip);
                std::experimental::apply([](auto&... i) { ((void)++i, ...); }, zip);
        }

        return zip;
}

// identity iterator:
template <typename Iterator>
struct identity_iterator
{
        using iterator_type = Iterator;
        using iterator_category = std::forward_iterator_tag;

        using value_type = typename std::iterator_traits<iterator_type>::value_type;
        using difference_type = typename std::iterator_traits<iterator_type>::difference_type;

        using pointer = typename std::iterator_traits<iterator_type>::pointer;
        using reference = typename std::iterator_traits<iterator_type>::reference;

        //
        constexpr identity_iterator() = default;
        constexpr explicit identity_iterator(iterator_type i) : base_{i}
        {
        }

        //
        constexpr reference operator*() const
        {
                return *base_;
        }

        constexpr pointer operator->() const
        {
                return std::addressof(operator*());
        }

        //
        constexpr identity_iterator& operator++() noexcept
        {
                return *this;
        }

        constexpr identity_iterator operator++(int)
        {
                return *this;
        }

        //
        constexpr bool operator==(const identity_iterator& rhs) const
        {
                return base_ == rhs.base_;
        }

        constexpr bool operator!=(const identity_iterator& rhs) const
        {
                return !(*this == rhs);
        }

private:
        iterator_type base_{};
};

template <typename Iterator>
constexpr identity_iterator<Iterator> make_identity_iterator(Iterator i)
{
        return identity_iterator<Iterator>{i};
}

// input iterator concept check:
template <typename InputIterator>
using check_input_iterator = std::enable_if_t<!std::is_integral<InputIterator>::value>;

//
} // namespace ecs

#endif // UTILITY_H
