// Copyright Ildus Nezametdinov 2016.
// Distributed under the Boost Software License, Version 1.0.
//(See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef STORAGE_TRAITS_H
#define STORAGE_TRAITS_H

#include <experimental/type_traits>
#include <cstddef>
#include <utility>

namespace ecs
{
//
template <typename Storage>
struct storage_traits
{
        // types:
        using no_exceptions = typename Storage::no_exceptions;

        //
        using storage_type = Storage;
        using value_type = typename Storage::value_type;

        using pointer = value_type*;
        using const_pointer = const value_type*;

        // construct/destroy:
        //
        template <typename... Args>
        static constexpr auto construct(storage_type& storage, pointer location, Args&&... args)
        {
                storage.construct(location, std::forward<Args>(args)...);
                return location;
        }

        static constexpr void destroy(storage_type& storage, pointer location) noexcept
        {
                storage.destroy(location);
        }

        // iterators:
        //
        static constexpr auto begin(storage_type& storage) noexcept
        {
                return static_cast<pointer>(storage.begin());
        }

        static constexpr auto begin(const storage_type& storage) noexcept
        {
                return static_cast<const_pointer>(storage.begin());
        }

        //
        static constexpr auto end(storage_type& storage) noexcept
        {
                return storage_traits::begin(storage) + static_cast<std::size_t>(storage.size());
        }

        static constexpr auto end(const storage_type& storage) noexcept
        {
                return storage_traits::begin(storage) + static_cast<std::size_t>(storage.size());
        }

        // capacity:
        //
        static constexpr auto reallocate(storage_type& storage, std::size_t n)
        {
                return storage.reallocate(n);
        }

        //
        static constexpr bool empty(const storage_type& storage) noexcept
        {
                return static_cast<std::size_t>(storage.size()) == 0;
        }

        static constexpr bool full(const storage_type& storage) noexcept
        {
                return static_cast<std::size_t>(storage.size()) ==
                       storage_traits::capacity(storage);
        }

        //
        static constexpr auto& size(storage_type& storage) noexcept
        {
                return storage.size();
        }

        static constexpr auto& size(const storage_type& storage) noexcept
        {
                return storage.size();
        }

        //
        static constexpr auto max_size(const storage_type& storage) noexcept
        {
                return static_cast<std::size_t>(storage.max_size());
        }

        static constexpr auto capacity(const storage_type& storage) noexcept
        {
                return static_cast<std::size_t>(storage.capacity());
        }

        // swap: TO DO
        //
        static constexpr void swap(storage_type& lhs,
                                   storage_type& rhs) noexcept(noexcept(std::swap(lhs, rhs)))
        {
                std::swap(lhs, rhs);
        }
};

//
} // namespace ecs

#endif // STORAGE_TRAITS_H
