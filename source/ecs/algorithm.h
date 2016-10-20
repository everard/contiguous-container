// Copyright Ildus Nezametdinov 2016.
// Distributed under the Boost Software License, Version 1.0.
//(See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef ALGORITHM_H
#define ALGORITHM_H

#include <algorithm>
#include "tuple.h"

namespace ecs
{
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

//
} // namespace ecs

#endif // ALGORITHM_H
