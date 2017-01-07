// Copyright Ildus Nezametdinov 2016.
// Distributed under the Boost Software License, Version 1.0.
//(See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef STORAGE_TRAITS_H
#define STORAGE_TRAITS_H

#include "utility.h"
#include <limits>

namespace ecs
{
template <typename Storage>
struct storage_traits
{
        // non-deduced types:
        using storage_type = Storage;
        using value_type = typename storage_type::value_type;

        // implementation of meta-traits:
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

                //
                template <typename Ptr>
                using ptr_rebind =
                        typename std::pointer_traits<Ptr>::template rebind<const value_type>;

                template <typename Ptr>
                using ptr_difference_type = typename std::pointer_traits<Ptr>::difference_type;

                // type selection traits:
                template <typename S>
                using pointer_trait = typename S::pointer;
                template <typename S>
                using const_pointer_trait = typename S::const_pointer;

                template <typename S>
                using size_type_trait = typename S::size_type;
                template <typename S>
                using difference_type_trait = typename S::difference_type;

                // types:
                using pointer = select_type<value_type*, pointer_trait, storage_type>;
                using const_pointer =
                        select_type<ptr_rebind<pointer>, const_pointer_trait, storage_type>;

                using size_type = select_type<
                        std::make_unsigned_t<select_type<ptr_difference_type<pointer>,
                                                         difference_type_trait, storage_type>>,
                        size_type_trait, storage_type>;
                using difference_type = select_type<ptr_difference_type<pointer>,
                                                    difference_type_trait, storage_type>;

                // member function detection traits:
                template <typename S>
                using construct_trait = decltype(std::declval<S>().construct(pointer{}));
                template <typename S>
                using destroy_trait = decltype(std::declval<S>().destroy(pointer{}));

                template <typename S>
                using end_trait = decltype(std::declval<S>().end());
                template <typename S>
                using end_const_trait = decltype(std::declval<std::add_const_t<S>>().end());

                template <typename S>
                using reallocate_trait = decltype(std::declval<S>().reallocate(size_type{}));
                template <typename S>
                using reallocate_assign_trait =
                        decltype(std::declval<S>().reallocate_assign(size_type{}, pointer{}));

                template <typename S>
                using empty_trait = decltype(std::declval<std::add_const_t<S>>().empty());
                template <typename S>
                using full_trait = decltype(std::declval<std::add_const_t<S>>().full());

                template <typename S>
                using inc_size_trait = decltype(std::declval<S>().inc_size(size_type{}));
                template <typename S>
                using dec_size_trait = decltype(std::declval<S>().dec_size(size_type{}));
                template <typename S>
                using max_size_trait = decltype(std::declval<std::add_const_t<S>>().max_size());

                template <typename S>
                using swap_trait = decltype(std::declval<S>().swap(std::declval<S&>()));

                // member function existence flags:
                static constexpr bool construct_exists = exists<construct_trait, storage_type>;
                static constexpr bool destroy_exists = exists<destroy_trait, storage_type>;

                static constexpr bool end_exists = exists_exact<pointer, end_trait, storage_type>;
                static constexpr bool end_const_exists =
                        exists_exact<const_pointer, end_const_trait, storage_type>;

                static constexpr bool reallocate_exists =
                        exists_exact<bool, reallocate_trait, storage_type>;
                static constexpr bool reallocate_assign_exists =
                        exists_exact<bool, reallocate_assign_trait, storage_type>;

                static constexpr bool empty_exists = exists_exact<bool, empty_trait, storage_type>;
                static constexpr bool full_exists = exists_exact<bool, full_trait, storage_type>;

                static constexpr bool inc_size_exists = exists<inc_size_trait, storage_type>;
                static constexpr bool dec_size_exists = exists<dec_size_trait, storage_type>;
                static constexpr bool max_size_exists =
                        exists_exact<size_type, max_size_trait, storage_type>;

                static constexpr bool swap_exists = exists<swap_trait, storage_type>;
        };

        // deduced types:
        using pointer = typename meta::pointer;
        using const_pointer = typename meta::const_pointer;

        using size_type = typename meta::size_type;
        using difference_type = typename meta::difference_type;

        // constants:
        static constexpr size_type max_ptrdiff =
                static_cast<size_type>(std::numeric_limits<difference_type>::max());

        // construct/destroy:
        template <bool E = meta::construct_exists, std::enable_if_t<E, int> = 0, typename... Args>
        static constexpr pointer construct(storage_type& storage, pointer location, Args&&... args)
        {
                storage.construct(location, std::forward<Args>(args)...);
                return location;
        }

        template <bool E = meta::construct_exists, std::enable_if_t<!E, int> = 0, typename... Args>
        static constexpr pointer construct(storage_type&, pointer location, Args&&... args)
        {
                ::new((void*)ptr_cast(location)) value_type{std::forward<Args>(args)...};
                return location;
        }

        template <bool E = meta::destroy_exists, std::enable_if_t<E, int> = 0>
        static constexpr void destroy(storage_type& storage, pointer location) noexcept
        {
                storage.destroy(location);
        }

        template <bool E = meta::destroy_exists, std::enable_if_t<!E, int> = 0>
        static constexpr void destroy(storage_type&, pointer location) noexcept
        {
                if(!std::is_trivially_destructible<value_type>::value)
                        ptr_cast(location)->~value_type();
        }

        // iterators:
        static constexpr pointer begin(storage_type& storage) noexcept
        {
                return storage.begin();
        }

        static constexpr const_pointer begin(const storage_type& storage) noexcept
        {
                return storage.begin();
        }

        template <bool E = meta::end_exists, std::enable_if_t<E, int> = 0>
        static constexpr pointer end(storage_type& storage) noexcept
        {
                return storage.end();
        }

        template <bool E = meta::end_exists, std::enable_if_t<!E, int> = 0>
        static constexpr pointer end(storage_type& storage) noexcept
        {
                return begin(storage) + static_cast<difference_type>(storage.size());
        }

        template <bool E = meta::end_const_exists, std::enable_if_t<E, int> = 0>
        static constexpr const_pointer end(const storage_type& storage) noexcept
        {
                return storage.end();
        }

        template <bool E = meta::end_const_exists, std::enable_if_t<!E, int> = 0>
        static constexpr const_pointer end(const storage_type& storage) noexcept
        {
                return begin(storage) + static_cast<difference_type>(storage.size());
        }

        // capacity/size:
        template <bool E = meta::reallocate_exists, std::enable_if_t<E, int> = 0>
        static constexpr bool reallocate(storage_type& storage, size_type n)
        {
                return storage.reallocate(n);
        }

        template <bool E = meta::reallocate_exists, std::enable_if_t<!E, int> = 0>
        static constexpr bool reallocate(storage_type&, size_type) noexcept
        {
                return false;
        }

        template <bool E = meta::reallocate_assign_exists, std::enable_if_t<E, int> = 0,
                  typename ForwardIterator>
        static constexpr bool reallocate_assign(storage_type& storage, size_type n,
                                                ForwardIterator first)
        {
                return storage.reallocate_assign(n, first);
        }

        template <bool E = meta::reallocate_assign_exists, std::enable_if_t<!E, int> = 0,
                  typename ForwardIterator>
        static constexpr bool reallocate_assign(storage_type& storage, size_type n,
                                                ForwardIterator first)
        {
                if(!reallocate(storage, n))
                        return false;

                assign(storage, n, first);
                return true;
        }

        //
        template <bool E = meta::empty_exists, std::enable_if_t<E, int> = 0>
        static constexpr bool empty(const storage_type& storage) noexcept
        {
                return storage.empty();
        }

        template <bool E = meta::empty_exists, std::enable_if_t<!E, int> = 0>
        static constexpr bool empty(const storage_type& storage) noexcept
        {
                return storage.size() == 0;
        }

        template <bool E = meta::full_exists, std::enable_if_t<E, int> = 0>
        static constexpr bool full(const storage_type& storage) noexcept
        {
                return storage.full();
        }

        template <bool E = meta::full_exists, std::enable_if_t<!E, int> = 0>
        static constexpr bool full(const storage_type& storage) noexcept
        {
                return storage.size() == storage.capacity();
        }

        //
        static constexpr void set_size(storage_type& storage, size_type n) noexcept
        {
                storage.set_size(n);
        }

        template <bool E = meta::inc_size_exists, std::enable_if_t<E, int> = 0>
        static constexpr void inc_size(storage_type& storage, size_type n = 1) noexcept
        {
                storage.inc_size(n);
        }

        template <bool E = meta::inc_size_exists, std::enable_if_t<!E, int> = 0>
        static constexpr void inc_size(storage_type& storage, size_type n = 1) noexcept
        {
                storage.set_size(storage.size() + n);
        }

        template <bool E = meta::dec_size_exists, std::enable_if_t<E, int> = 0>
        static constexpr void dec_size(storage_type& storage, size_type n = 1) noexcept
        {
                storage.dec_size(n);
        }

        template <bool E = meta::dec_size_exists, std::enable_if_t<!E, int> = 0>
        static constexpr void dec_size(storage_type& storage, size_type n = 1) noexcept
        {
                storage.set_size(storage.size() - n);
        }

        //
        static constexpr size_type size(const storage_type& storage) noexcept
        {
                return storage.size();
        }

        template <bool E = meta::max_size_exists, std::enable_if_t<E, int> = 0>
        static constexpr size_type max_size(const storage_type& storage) noexcept
        {
                return storage.max_size();
        }

        template <bool E = meta::max_size_exists, std::enable_if_t<!E, int> = 0>
        static constexpr size_type max_size(const storage_type&) noexcept
        {
                return static_cast<size_type>(max_ptrdiff / sizeof(value_type));
        }

        static constexpr size_type capacity(const storage_type& storage) noexcept
        {
                return storage.capacity();
        }

        // swap:
        template <bool E = meta::swap_exists, std::enable_if_t<E, int> = 0>
        static constexpr void swap(storage_type& lhs,
                                   storage_type& rhs) noexcept(noexcept(lhs.swap(rhs)))
        {
                lhs.swap(rhs);
        }

        template <bool E = meta::swap_exists, std::enable_if_t<!E, int> = 0>
        static constexpr void swap(storage_type& lhs,
                                   storage_type& rhs) noexcept(noexcept(std::swap(lhs, rhs)))
        {
                std::swap(lhs, rhs);
        }

        // pointer manipulation:
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

        // assignment:
        template <typename ForwardIterator>
        static constexpr void assign(storage_type& storage, size_type n, ForwardIterator first)
        {
                if(n > size(storage))
                {
                        auto target = begin(storage), mid = end(storage);
                        auto sentinel = target + static_cast<difference_type>(n);

                        std::tie(target, first) =
                                for_each_iter(target, mid, first, [](auto i, auto j) { *i = *j; });

                        for_each_iter(target, sentinel, first, [&storage](auto i, auto j) {
                                (void)construct(storage, i, *j), inc_size(storage);
                        });
                }
                else
                {
                        for_each_iter(std::copy_n(first, n, begin(storage)), end(storage),
                                      [&storage](auto i) { destroy(storage, i); });
                        set_size(storage, n);
                }
        }
};

//
} // namespace ecs

#endif // STORAGE_TRAITS_H
