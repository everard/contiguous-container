// Copyright Ildus Nezametdinov 2016.
// Distributed under the Boost Software License, Version 1.0.
//(See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef TUPLE_H
#define TUPLE_H

#include <experimental/tuple>
#include <cstddef>
#include <utility>

namespace ecs
{
template <typename Tuple, std::size_t... I>
constexpr auto tuple_slice(Tuple&& t, std::index_sequence<I...>)
{
        return std::make_tuple(std::get<I>(std::forward<Tuple>(t))...);
}

//
} // namespace ecs

#endif // TUPLE_H
