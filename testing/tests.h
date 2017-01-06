// Copyright Ildus Nezametdinov 2016.
// Distributed under the Boost Software License, Version 1.0.
//(See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "../source/ecs/containers.h"

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

// helper types:
using count = unsigned long long;

// input iterator adaptor:
template <typename Iterator>
struct input_iterator_adaptor
{
        // types:
        using iterator_type = Iterator;
        using iterator_category = std::input_iterator_tag;

        using value_type = typename std::iterator_traits<iterator_type>::value_type;
        using difference_type = typename std::iterator_traits<iterator_type>::difference_type;

        using pointer = typename std::iterator_traits<iterator_type>::pointer;
        using reference = typename std::iterator_traits<iterator_type>::reference;

        //
        constexpr input_iterator_adaptor() = default;
        constexpr explicit input_iterator_adaptor(iterator_type i) : base_{i}
        {
        }

        //
        constexpr iterator_type base() const
        {
                return base_;
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
        constexpr input_iterator_adaptor& operator++()
        {
                ++base_;
                return *this;
        }

        constexpr input_iterator_adaptor operator++(int)
        {
                auto t = *this;
                ++(*this);
                return t;
        }

        //
        constexpr bool operator==(const input_iterator_adaptor& other)
        {
                return base_ == other.base_;
        }

        constexpr bool operator!=(const input_iterator_adaptor& other)
        {
                return !(*this == other);
        }

private:
        iterator_type base_{};
};

template <typename Iterator>
constexpr auto make_input_iterator(Iterator i)
{
        return input_iterator_adaptor<Iterator>{i};
}

// type, which is used for testing the behavior:
struct tracker
{
        // construct/destroy:
        tracker()
        {
                ++n_default_constructor_calls;
        }

        tracker(int x_) : x{x_}
        {
                ++n_non_default_constructor_calls;
        }

        tracker(const tracker& other) : x{other.x}
        {
                ++n_copy_constructor_calls;
        }

        tracker(tracker&& other) : x{other.x}
        {
                ++n_move_constructor_calls;
        }

        ~tracker()
        {
                ++n_destructor_calls;
        }

        // assign:
        tracker& operator=(const tracker& other)
        {
                ++n_copy_assignments;

                x = other.x;
                return *this;
        }

        tracker& operator=(tracker&& other)
        {
                ++n_move_assignments;

                x = other.x;
                return *this;
        }

        // data member
        int x{};

        // static data members for construction/destruction tracking:
        static count n_default_constructor_calls, n_non_default_constructor_calls;
        static count n_copy_constructor_calls, n_move_constructor_calls;
        static count n_copy_assignments, n_move_assignments;
        static count n_destructor_calls;
};

count tracker::n_default_constructor_calls{}, tracker::n_non_default_constructor_calls{};
count tracker::n_copy_constructor_calls{}, tracker::n_move_constructor_calls{};
count tracker::n_copy_assignments{}, tracker::n_move_assignments{};
count tracker::n_destructor_calls{};

// simple Storage type, which tracks calls to construct/destroy
template <typename T, std::size_t N>
struct tracked_storage
{
        using value_type = T;

        ~tracked_storage()
        {
                for(std::size_t i = 0; i < size_; ++i)
                        destroy(begin() + i);
        }

        T* begin() noexcept
        {
                return reinterpret_cast<T*>(storage_);
        }

        const T* begin() const noexcept
        {
                return reinterpret_cast<const T*>(storage_);
        }

        auto size() const noexcept
        {
                return size_;
        }

        auto capacity() const noexcept
        {
                return N;
        }

        static void reset_meta_data()
        {
                n_construct_calls = n_destroy_calls = 0;
        }

        static count n_construct_calls, n_destroy_calls;

private:
        template <typename... Args>
        void construct(T* location, Args&&... args)
        {
                ++n_construct_calls;
                ::new(location) T{std::forward<Args>(args)...};
        }

        void destroy(T* location) noexcept
        {
                ++n_destroy_calls;
                location->~T();
        }

        void set_size(std::size_t n) noexcept
        {
                size_ = n;
        }

        friend struct ecs::storage_traits<tracked_storage>;

private:
        alignas(T) unsigned char storage_[N * sizeof(T)];
        std::size_t size_{};
};

template <typename T, std::size_t N>
count tracked_storage<T, N>::n_construct_calls{};
template <typename T, std::size_t N>
count tracked_storage<T, N>::n_destroy_calls{};

// tests:
template <typename T, std::size_t N>
using container = ecs::contiguous_container<tracked_storage<T, N>>;
