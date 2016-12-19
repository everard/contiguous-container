// Copyright Ildus Nezametdinov 2016.
// Distributed under the Boost Software License, Version 1.0.
//(See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef STORAGE_TRAITS_H
#define STORAGE_TRAITS_H

#include <type_traits>
#include <limits>
#include <memory>

#include "utility.h"

namespace ecs
{
template <typename Storage>
struct storage_traits
{
        // implementation of meta-traits:
        //
        struct meta
        {
                template <typename...>
                using void_t = void;

                struct none
                {
                        none() = delete;
                        ~none() = delete;

                        none(const none&) = delete;
                        none(none&&) = delete;

                        none& operator=(const none&) = delete;
                        none& operator=(none&&) = delete;
                };

                template <typename D, typename, template <typename...> typename X, typename... Args>
                struct detector
                {
                        using result = std::false_type;
                        using type = D;
                };

                template <typename D, template <typename...> typename X, typename... Args>
                struct detector<D, void_t<X<Args...>>, X, Args...>
                {
                        using result = std::true_type;
                        using type = X<Args...>;
                };

                //
                template <typename D, template <typename...> typename X, typename... Args>
                using select_type = typename detector<D, void, X, Args...>::type;

                template <template <typename...> typename X, typename... Args>
                static constexpr bool exists = detector<none, void, X, Args...>::result::value;

                template <typename E, template <typename...> typename X, typename... Args>
                static constexpr bool exists_exact =
                        std::is_same<E, typename detector<none, void, X, Args...>::type>::value;
        };

        // type selectors:
        //
        template <typename S>
        using value_type_trait = typename S::value_type;

        template <typename S>
        using pointer_trait = typename S::pointer;
        template <typename S>
        using const_pointer_trait = typename S::const_pointer;

        template <typename S>
        using size_type_trait = typename S::size_type;
        template <typename S>
        using difference_type_trait = typename S::difference_type;

        //
        template <typename S>
        using select_pointer_type =
                typename meta::template select_type<value_type_trait<S>*, pointer_trait, S>;

        template <typename S>
        using select_const_pointer_type = typename meta::template select_type<
                typename std::pointer_traits<select_pointer_type<S>>::template rebind<
                        const value_type_trait<S>>,
                const_pointer_trait, S>;

        //
        template <typename S>
        using select_difference_type = typename meta::template select_type<
                typename std::pointer_traits<select_pointer_type<S>>::difference_type,
                difference_type_trait, S>;

        template <typename S>
        using select_size_type =
                typename meta::template select_type<std::make_unsigned_t<select_difference_type<S>>,
                                                    size_type_trait, S>;

        // types:
        //
        using storage_type = Storage;
        using value_type = typename storage_type::value_type;

        using pointer = select_pointer_type<storage_type>;
        using const_pointer = select_const_pointer_type<storage_type>;

        using size_type = select_size_type<storage_type>;
        using difference_type = select_difference_type<storage_type>;

        // member function traits:
        //
        template <typename S>
        using construct_trait = decltype(std::declval<S>().construct(std::declval<pointer>()));
        static constexpr bool construct_exists =
                meta::template exists<construct_trait, storage_type>;

        template <typename S>
        using destroy_trait = decltype(std::declval<S>().destroy(std::declval<pointer>()));
        static constexpr bool destroy_exists = meta::template exists<destroy_trait, storage_type>;

        //
        template <typename S>
        using end_trait = decltype(std::declval<S>().end());
        static constexpr bool end_exists =
                meta::template exists_exact<pointer, end_trait, storage_type>;
        static constexpr bool end_const_exists =
                meta::template exists_exact<const_pointer, end_trait, const storage_type>;

        //
        template <typename S>
        using reallocate_trait = decltype(std::declval<S>().reallocate(std::declval<size_type>()));
        static constexpr bool reallocate_exists =
                meta::template exists_exact<bool, reallocate_trait, storage_type>;

        template <typename S>
        using empty_trait = decltype(std::declval<S>().empty());
        static constexpr bool empty_exists =
                meta::template exists_exact<bool, empty_trait, storage_type>;

        template <typename S>
        using full_trait = decltype(std::declval<S>().full());
        static constexpr bool full_exists =
                meta::template exists_exact<bool, full_trait, storage_type>;

        //
        template <typename S>
        using inc_size_trait = decltype(std::declval<S>().inc_size(std::declval<size_type>()));
        static constexpr bool inc_size_exists = meta::template exists<inc_size_trait, storage_type>;

        template <typename S>
        using dec_size_trait = decltype(std::declval<S>().dec_size(std::declval<size_type>()));
        static constexpr bool dec_size_exists = meta::template exists<dec_size_trait, storage_type>;

        //
        template <typename S>
        using max_size_trait = decltype(std::declval<S>().max_size());
        static constexpr bool max_size_exists =
                meta::template exists_exact<size_type, max_size_trait, storage_type>;

        //
        template <typename S>
        using swap_trait = decltype(std::declval<S>().swap(std::declval<S>()));
        static constexpr bool swap_exists = meta::template exists<swap_trait, storage_type>;

        // constants:
        //
        static constexpr size_type max_ptrdiff =
                static_cast<size_type>(std::numeric_limits<difference_type>::max());

        // construct/destroy:
        //
        template <bool E = construct_exists, std::enable_if_t<E, int> = 0, typename... Args>
        static constexpr pointer construct(storage_type& storage, pointer location, Args&&... args)
        {
                storage.construct(location, std::forward<Args>(args)...);
                return location;
        }

        template <bool E = construct_exists, std::enable_if_t<!E, int> = 0, typename... Args>
        static constexpr pointer construct(storage_type&, pointer location, Args&&... args)
        {
                ::new((void*)ptr_cast(location)) value_type{std::forward<Args>(args)...};
                return location;
        }

        //
        template <bool E = destroy_exists, std::enable_if_t<E, int> = 0>
        static constexpr void destroy(storage_type& storage, pointer location) noexcept
        {
                storage.destroy(location);
        }

        template <bool E = destroy_exists, std::enable_if_t<!E, int> = 0>
        static constexpr void destroy(storage_type&, pointer location) noexcept
        {
                if(!std::is_trivially_destructible<value_type>::value)
                        ptr_cast(location)->~value_type();
        }

        // iterators:
        //
        static constexpr pointer begin(storage_type& storage) noexcept
        {
                return storage.begin();
        }

        static constexpr const_pointer begin(const storage_type& storage) noexcept
        {
                return storage.begin();
        }

        //
        template <bool E = end_exists, std::enable_if_t<E, int> = 0>
        static constexpr pointer end(storage_type& storage) noexcept
        {
                return storage.end();
        }

        template <bool E = end_exists, std::enable_if_t<!E, int> = 0>
        static constexpr pointer end(storage_type& storage) noexcept
        {
                return begin(storage) + static_cast<difference_type>(storage.size());
        }

        //
        template <bool E = end_const_exists, std::enable_if_t<E, int> = 0>
        static constexpr const_pointer end(const storage_type& storage) noexcept
        {
                return storage.end();
        }

        template <bool E = end_const_exists, std::enable_if_t<!E, int> = 0>
        static constexpr const_pointer end(const storage_type& storage) noexcept
        {
                return begin(storage) + static_cast<difference_type>(storage.size());
        }

        // capacity:
        //
        template <bool E = reallocate_exists, std::enable_if_t<E, int> = 0>
        static constexpr bool reallocate(storage_type& storage, size_type n)
        {
                return storage.reallocate(n);
        }

        template <bool E = reallocate_exists, std::enable_if_t<!E, int> = 0>
        static constexpr bool reallocate(storage_type&, size_type) noexcept
        {
                return false;
        }

        //
        template <bool E = empty_exists, std::enable_if_t<E, int> = 0>
        static constexpr bool empty(const storage_type& storage) noexcept
        {
                return storage.empty();
        }

        template <bool E = empty_exists, std::enable_if_t<!E, int> = 0>
        static constexpr bool empty(const storage_type& storage) noexcept
        {
                return storage.size() == 0;
        }

        //
        template <bool E = full_exists, std::enable_if_t<E, int> = 0>
        static constexpr bool full(const storage_type& storage) noexcept
        {
                return storage.full();
        }

        template <bool E = full_exists, std::enable_if_t<!E, int> = 0>
        static constexpr bool full(const storage_type& storage) noexcept
        {
                return storage.size() == storage.capacity();
        }

        //
        static constexpr size_type capacity(const storage_type& storage) noexcept
        {
                return storage.capacity();
        }

        // size:
        //
        static constexpr void set_size(storage_type& storage, size_type n) noexcept
        {
                storage.set_size(n);
        }

        //
        template <bool E = inc_size_exists, std::enable_if_t<E, int> = 0>
        static constexpr void inc_size(storage_type& storage, size_type n = 1) noexcept
        {
                storage.inc_size(n);
        }

        template <bool E = inc_size_exists, std::enable_if_t<!E, int> = 0>
        static constexpr void inc_size(storage_type& storage, size_type n = 1) noexcept
        {
                storage.set_size(storage.size() + n);
        }

        //
        template <bool E = dec_size_exists, std::enable_if_t<E, int> = 0>
        static constexpr void dec_size(storage_type& storage, size_type n = 1) noexcept
        {
                storage.dec_size(n);
        }

        template <bool E = dec_size_exists, std::enable_if_t<!E, int> = 0>
        static constexpr void dec_size(storage_type& storage, size_type n = 1) noexcept
        {
                storage.set_size(storage.size() - n);
        }

        //
        static constexpr size_type size(const storage_type& storage) noexcept
        {
                return storage.size();
        }

        //
        template <bool E = max_size_exists, std::enable_if_t<E, int> = 0>
        static constexpr size_type max_size(const storage_type& storage) noexcept
        {
                return storage.max_size();
        }

        template <bool E = max_size_exists, std::enable_if_t<!E, int> = 0>
        static constexpr size_type max_size(const storage_type&) noexcept
        {
                return static_cast<size_type>(max_ptrdiff / sizeof(value_type));
        }

        // swap:
        //
        template <bool E = swap_exists, std::enable_if_t<E, int> = 0>
        static constexpr void swap(storage_type& lhs,
                                   storage_type& rhs) noexcept(noexcept(lhs.swap(rhs)))
        {
                lhs.swap(rhs);
        }

        template <bool E = swap_exists, std::enable_if_t<!E, int> = 0>
        static constexpr void swap(storage_type& lhs,
                                   storage_type& rhs) noexcept(noexcept(std::swap(lhs, rhs)))
        {
                std::swap(lhs, rhs);
        }

        // pointer manipulation:
        //
        template <typename T>
        static constexpr T* ptr_cast(T* ptr) noexcept
        {
                return ptr;
        }

        template <typename T>
        static constexpr auto ptr_cast(T ptr) noexcept -> decltype(std::addressof(*ptr))
        {
                return ptr ? std::addressof(*ptr) : nullptr;
        }
};

//
} // namespace ecs

#endif // STORAGE_TRAITS_H
