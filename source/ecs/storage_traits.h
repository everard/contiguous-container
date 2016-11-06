// Copyright Ildus Nezametdinov 2016.
// Distributed under the Boost Software License, Version 1.0.
//(See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef STORAGE_TRAITS_H
#define STORAGE_TRAITS_H

#include <experimental/type_traits>
#include <limits>
#include <memory>

#include "utility.h"

namespace ecs
{
namespace detail
{
using std::enable_if_t;
using std::make_unsigned_t;

using std::pointer_traits;
using std::experimental::detected_or_t;
using std::experimental::is_detected_v;
using std::experimental::is_detected_exact_v;

// detection facilities:
//
template <typename T>
using value_type = typename T::value_type;

template <typename T>
using pointer_type_detector = typename T::pointer;
template <typename T>
using const_pointer_type_detector = typename T::const_pointer;

template <typename T>
using size_type_detector = typename T::size_type;
template <typename T>
using difference_type_detector = typename T::difference_type;

//
template <typename T>
using pointer_type = detected_or_t<value_type<T>*, pointer_type_detector, T>;
template <typename T>
using const_pointer_type = detected_or_t<
        typename pointer_traits<pointer_type<T>>::template rebind<const value_type<T>>,
        const_pointer_type_detector, T>;

template <typename T>
using difference_type = detected_or_t<typename pointer_traits<pointer_type<T>>::difference_type,
                                      difference_type_detector, T>;
template <typename T>
using size_type = detected_or_t<std::make_unsigned_t<difference_type<T>>, size_type_detector, T>;

//
template <typename T>
using construct_function = decltype(std::declval<T>().construct(std::declval<pointer_type<T>>()));
template <typename T>
using destroy_function = decltype(std::declval<T>().destroy(std::declval<pointer_type<T>>()));
template <typename T>
using end_function = decltype(std::declval<T>().end());
template <typename T>
using reallocate_function = decltype(std::declval<T>().reallocate(std::declval<size_type<T>>()));

template <typename T>
using empty_function = decltype(std::declval<T>().empty());
template <typename T>
using full_function = decltype(std::declval<T>().full());
template <typename T>
using max_size_function = decltype(std::declval<T>().max_size());
template <typename T>
using swap_function = decltype(std::declval<T>().swap(std::declval<T>()));

//
template <typename R, template <typename...> typename Op, typename... Args>
using exists = enable_if_t<is_detected_v<Op, Args...>, R>;
template <typename R, template <typename...> typename Op, typename... Args>
using exists_exact = enable_if_t<is_detected_exact_v<R, Op, Args...>, R>;

template <typename R, template <typename...> typename Op, typename... Args>
using absent = enable_if_t<!is_detected_v<Op, Args...>, R>;
template <typename R, template <typename...> typename Op, typename... Args>
using absent_exact = enable_if_t<!is_detected_exact_v<R, Op, Args...>, R>;

//
} // namespace detail

//
template <typename Storage>
struct storage_traits
{
        // types:
        //
        using storage_type = Storage;
        using value_type = typename Storage::value_type;

        using size_type = detail::size_type<storage_type>;
        using difference_type = detail::difference_type<storage_type>;

        using pointer = detail::pointer_type<storage_type>;
        using const_pointer = detail::const_pointer_type<storage_type>;

        // construct/destroy:
        //
        template <typename T = storage_type, typename... Args>
        static constexpr detail::exists<pointer, detail::construct_function, T> construct(
                storage_type& storage, pointer location, Args&&... args)
        {
                storage.construct(location, std::forward<Args>(args)...);
                return location;
        }
        template <typename T = storage_type, typename... Args>
        static constexpr detail::absent<pointer, detail::construct_function, T> construct(
                storage_type&, pointer location, Args&&... args)
        {
                ::new((void*)ptr_cast(location)) value_type{std::forward<Args>(args)...};
                return location;
        }

        template <typename T = storage_type>
        static constexpr detail::exists<void, detail::destroy_function, T> destroy(
                storage_type& storage, pointer location) noexcept
        {
                storage.destroy(location);
        }
        template <typename T = storage_type>
        static constexpr detail::absent<void, detail::destroy_function, T> destroy(
                storage_type&, pointer location) noexcept
        {
                if(!std::experimental::is_trivially_destructible_v<value_type>)
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
        template <typename T = storage_type>
        static constexpr detail::exists_exact<pointer, detail::end_function, T> end(
                storage_type& storage) noexcept
        {
                return storage.end();
        }
        template <typename T = storage_type>
        static constexpr detail::absent_exact<pointer, detail::end_function, T> end(
                storage_type& storage) noexcept
        {
                return begin(storage) + static_cast<difference_type>(storage.size());
        }

        template <typename T = const storage_type>
        static constexpr detail::exists_exact<const_pointer, detail::end_function, T> end(
                const storage_type& storage) noexcept
        {
                return storage.end();
        }
        template <typename T = const storage_type>
        static constexpr detail::absent_exact<const_pointer, detail::end_function, T> end(
                const storage_type& storage) noexcept
        {
                return begin(storage) + static_cast<difference_type>(storage.size());
        }

        // capacity:
        //
        template <typename T = storage_type>
        static constexpr detail::exists_exact<bool, detail::reallocate_function, T> reallocate(
                storage_type& storage, size_type n)
        {
                return storage.reallocate(n);
        }
        template <typename T = storage_type>
        static constexpr detail::absent_exact<bool, detail::reallocate_function, T> reallocate(
                storage_type&, size_type)
        {
                return false;
        }

        //
        template <typename T = const storage_type>
        static constexpr detail::exists_exact<bool, detail::empty_function, T> empty(
                const storage_type& storage) noexcept
        {
                return storage.empty();
        }
        template <typename T = const storage_type>
        static constexpr detail::absent_exact<bool, detail::empty_function, T> empty(
                const storage_type& storage) noexcept
        {
                return static_cast<size_type>(storage.size()) == 0;
        }

        template <typename T = const storage_type>
        static constexpr detail::exists_exact<bool, detail::full_function, T> full(
                const storage_type& storage) noexcept
        {
                return storage.full();
        }
        template <typename T = const storage_type>
        static constexpr detail::absent_exact<bool, detail::full_function, T> full(
                const storage_type& storage) noexcept
        {
                return static_cast<size_type>(storage.size()) == capacity(storage);
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
        template <typename T = const storage_type>
        static constexpr detail::exists_exact<size_type, detail::max_size_function, T> max_size(
                const storage_type& storage) noexcept
        {
                return storage.max_size();
        }
        template <typename T = const storage_type>
        static constexpr detail::absent_exact<size_type, detail::max_size_function, T> max_size(
                const storage_type&) noexcept
        {
                return static_cast<size_type>(
                        static_cast<size_type>(std::numeric_limits<difference_type>::max()) /
                        sizeof(value_type));
        }

        static constexpr size_type capacity(const storage_type& storage) noexcept
        {
                return storage.capacity();
        }

        // swap:
        //
        template <typename T = storage_type>
        static constexpr detail::exists<void, detail::swap_function, T> swap(
                storage_type& lhs, storage_type& rhs) noexcept(noexcept(lhs.swap(rhs)))
        {
                lhs.swap(rhs);
        }
        template <typename T = storage_type>
        static constexpr detail::absent<void, detail::swap_function, T> swap(
                storage_type& lhs, storage_type& rhs) noexcept(noexcept(std::swap(lhs, rhs)))
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
