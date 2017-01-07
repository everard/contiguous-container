// Copyright Ildus Nezametdinov 2016.
// Distributed under the Boost Software License, Version 1.0.
//(See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "../source/ecs/containers.h"
#include <vector>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

// test storage_traits:
namespace test_storage_traits
{
// base storage type, which provides minimal functionality:
struct base_storage
{
        using value_type = int;

        // minimal interface:
        value_type* begin() noexcept
        {
                return nullptr;
        }

        const value_type* begin() const noexcept
        {
                return nullptr;
        }

        void set_size(std::size_t) noexcept
        {
        }

        std::size_t size() const noexcept
        {
                return 0;
        }

        std::size_t capacity() const noexcept
        {
                return 0;
        }
};

using base_storage_traits = ecs::storage_traits<base_storage>;

// test base_storage_traits:
static_assert(std::is_same<base_storage_traits::pointer, int*>::value);
static_assert(std::is_same<base_storage_traits::const_pointer, const int*>::value);

static_assert(std::is_same<base_storage_traits::size_type, std::size_t>::value);
static_assert(std::is_same<base_storage_traits::difference_type, std::ptrdiff_t>::value);

static_assert(!base_storage_traits::meta::construct_exists);
static_assert(!base_storage_traits::meta::destroy_exists);

static_assert(!base_storage_traits::meta::end_exists);
static_assert(!base_storage_traits::meta::end_const_exists);

static_assert(!base_storage_traits::meta::reallocate_exists);
static_assert(!base_storage_traits::meta::reallocate_assign_exists);

static_assert(!base_storage_traits::meta::empty_exists);
static_assert(!base_storage_traits::meta::full_exists);

static_assert(!base_storage_traits::meta::inc_size_exists);
static_assert(!base_storage_traits::meta::dec_size_exists);
static_assert(!base_storage_traits::meta::max_size_exists);

static_assert(!base_storage_traits::meta::swap_exists);

// storage type, which fails to provide custom member functions for optional requirements:
struct bad_storage : base_storage
{
        // wrong parameter lists for construct() and destroy():
        void construct(int) const
        {
        }

        void destroy(long*)
        {
        }

        // wrong return types for end():
        int end() noexcept
        {
                return 0;
        }

        int end() const noexcept
        {
                return 0;
        }

        // wrong parameter lists for reallocate() and reallocate_assign():
        bool reallocate(int*)
        {
                return false;
        }

        bool reallocate_assign(int, int)
        {
                return false;
        }

        // non-const empty() and full():
        bool empty() noexcept
        {
                return false;
        }

        bool full() noexcept
        {
                return false;
        }

        //
        // wrong parameter lists for inc_size() and dec_size():
        void inc_size(int*) noexcept
        {
        }

        void dec_size(int*) noexcept
        {
        }

        //
        // non-const max_size():
        std::size_t max_size() noexcept
        {
                return 0;
        }

        // wrong parameter list for swap():
        void swap(int)
        {
        }
};

using bad_storage_traits = ecs::storage_traits<bad_storage>;

// test defaulted_storage_traits:
static_assert(std::is_same<bad_storage_traits::pointer, int*>::value);
static_assert(std::is_same<bad_storage_traits::const_pointer, const int*>::value);

static_assert(std::is_same<bad_storage_traits::size_type, std::size_t>::value);
static_assert(std::is_same<bad_storage_traits::difference_type, std::ptrdiff_t>::value);

static_assert(!bad_storage_traits::meta::construct_exists);
static_assert(!bad_storage_traits::meta::destroy_exists);

static_assert(!bad_storage_traits::meta::end_exists);
static_assert(!bad_storage_traits::meta::end_const_exists);

static_assert(!bad_storage_traits::meta::reallocate_exists);
static_assert(!bad_storage_traits::meta::reallocate_assign_exists);

static_assert(!bad_storage_traits::meta::empty_exists);
static_assert(!bad_storage_traits::meta::full_exists);

static_assert(!bad_storage_traits::meta::inc_size_exists);
static_assert(!bad_storage_traits::meta::dec_size_exists);
static_assert(!bad_storage_traits::meta::max_size_exists);

static_assert(!bad_storage_traits::meta::swap_exists);

// storage type, which satisfies all optional requirements:
struct good_storage : base_storage
{
        struct const_pointer
        {
                using element_type = const value_type;
        };

        struct pointer
        {
                using element_type = value_type;

                template <typename U>
                using rebind = const_pointer;
        };

        using size_type = unsigned char;
        using difference_type = signed char;

        void swap(good_storage&)
        {
        }

private:
        template <typename... Args>
        void construct(pointer, Args&&...)
        {
        }

        void destroy(pointer) noexcept
        {
        }

        //
        pointer begin() noexcept
        {
                return pointer{};
        }

        const_pointer begin() const noexcept
        {
                return const_pointer{};
        }

        pointer end() noexcept
        {
                return pointer{};
        }

        const_pointer end() const noexcept
        {
                return const_pointer{};
        }

        //
        bool reallocate(std::size_t)
        {
                return false;
        }

        template <typename ForwardIterator>
        bool reallocate_assign(std::size_t, ForwardIterator)
        {
                return false;
        }

        //
        bool empty() const noexcept
        {
                return true;
        }

        bool full() const noexcept
        {
                return true;
        }

        //
        void inc_size(size_type) noexcept
        {
        }

        void dec_size(size_type) noexcept
        {
        }

        size_type max_size() const noexcept
        {
                return 0;
        }

        friend struct ecs::storage_traits<good_storage>;
};

using good_storage_traits = ecs::storage_traits<good_storage>;

// test custom_storage_straits:
static_assert(std::is_same<good_storage_traits::pointer, good_storage::pointer>::value);
static_assert(std::is_same<good_storage_traits::const_pointer, good_storage::const_pointer>::value);

static_assert(std::is_same<good_storage_traits::size_type, good_storage::size_type>::value);
static_assert(
        std::is_same<good_storage_traits::difference_type, good_storage::difference_type>::value);

static_assert(good_storage_traits::meta::construct_exists);
static_assert(good_storage_traits::meta::destroy_exists);

static_assert(good_storage_traits::meta::end_exists);
static_assert(good_storage_traits::meta::end_const_exists);

static_assert(good_storage_traits::meta::reallocate_exists);
static_assert(good_storage_traits::meta::reallocate_assign_exists);

static_assert(good_storage_traits::meta::empty_exists);
static_assert(good_storage_traits::meta::full_exists);

static_assert(good_storage_traits::meta::inc_size_exists);
static_assert(good_storage_traits::meta::dec_size_exists);
static_assert(good_storage_traits::meta::max_size_exists);

static_assert(good_storage_traits::meta::swap_exists);

//
} // namespace test_storage_traits

// helper types:
using count = unsigned long long;

// input iterator adaptor:
template <typename Iterator>
struct input_iterator_adaptor
{
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

// helper type, which is used in logging:
struct log_entry
{
        enum class op_type
        {
                default_construct,
                non_default_construct,
                copy_construct,
                move_construct,
                destroy,
                copy,
                move
        };

        log_entry(op_type op_, int value_src_, int value_dst_)
                : op{op_}, value_src{value_src_}, value_dst{value_dst_}
        {
        }

        op_type op;
        int value_src, value_dst;
};

std::vector<log_entry> global_log{};

// type, which is used for testing the behavior:
struct tracker
{
        // construct/destroy:
        tracker()
        {
                ++n_default_constructor_calls;
                global_log.emplace_back(log_entry::op_type::default_construct, 0, 0);
        }

        tracker(int x_) : x{x_}
        {
                ++n_non_default_constructor_calls;
                global_log.emplace_back(log_entry::op_type::non_default_construct, x, x);
        }

        tracker(const tracker& other) : x{other.x}
        {
                ++n_copy_constructor_calls;
                global_log.emplace_back(log_entry::op_type::copy_construct, x, x);
        }

        tracker(tracker&& other) : x{other.x}
        {
                ++n_move_constructor_calls;
                global_log.emplace_back(log_entry::op_type::move_construct, x, x);
        }

        ~tracker()
        {
                ++n_destructor_calls;
                global_log.emplace_back(log_entry::op_type::destroy, x, x);
        }

        // assign:
        tracker& operator=(const tracker& other)
        {
                global_log.emplace_back(log_entry::op_type::copy, other.x, x);

                ++n_copy_assignments;
                x = other.x;

                return *this;
        }

        tracker& operator=(tracker&& other)
        {
                global_log.emplace_back(log_entry::op_type::move, other.x, x);

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

TEST_CASE("", "[contiguous_container]")
{
}
