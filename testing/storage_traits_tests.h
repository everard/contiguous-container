// Copyright Ildus Nezametdinov 2017.
// Distributed under the Boost Software License, Version 1.0.
//(See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "../source/ecs/contiguous_container.h"
#include <vector>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

namespace storage_traits_testing
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

        void set_size(std::size_t n) noexcept
        {
                size_ = n;
        }

        std::size_t size() const noexcept
        {
                return size_;
        }

        std::size_t capacity() const noexcept
        {
                return size_;
        }

protected:
        std::size_t size_{};
};

TEST_CASE("base_storage", "[ecs::storage_traits]")
{
        using traits = ecs::storage_traits<base_storage>;

        static_assert(std::is_same<traits::pointer, int*>::value);
        static_assert(std::is_same<traits::const_pointer, const int*>::value);

        static_assert(std::is_same<traits::size_type, std::size_t>::value);
        static_assert(std::is_same<traits::difference_type, std::ptrdiff_t>::value);

        int value{};
        base_storage storage, other_storage;
        const base_storage& const_storage = storage;

        //
        traits::construct(storage, &value, 5);
        traits::destroy(storage, &value);
        REQUIRE(value == 5);

        //
        REQUIRE(traits::begin(storage) == traits::end(storage));
        REQUIRE(traits::begin(const_storage) == traits::end(const_storage));

        //
        REQUIRE(!traits::reallocate(storage, 0));
        REQUIRE(!traits::reallocate_assign(storage, 0, &value));

        //
        REQUIRE(traits::empty(const_storage));
        REQUIRE(traits::full(const_storage));

        //
        traits::set_size(storage, 5);
        REQUIRE(traits::size(const_storage) == 5);

        traits::inc_size(storage);
        REQUIRE(traits::size(const_storage) == 6);
        traits::inc_size(storage, 3);
        REQUIRE(traits::size(const_storage) == 9);

        traits::dec_size(storage);
        REQUIRE(traits::size(const_storage) == 8);
        traits::dec_size(storage, 7);
        REQUIRE(traits::size(const_storage) == 1);

        //
        REQUIRE(traits::capacity(const_storage) == 1);
        REQUIRE(traits::max_size(const_storage) ==
                (std::numeric_limits<std::ptrdiff_t>::max() / sizeof(int)));

        REQUIRE(traits::size(other_storage) == 0);
        REQUIRE(traits::size(const_storage) == 1);
        traits::swap(storage, other_storage);
        REQUIRE(traits::size(other_storage) == 1);
        REQUIRE(traits::size(const_storage) == 0);
}

// type, which tracks calls to optional member functions of storage:
struct optional_memfn_call_tracker
{
        void reset()
        {
                *this = optional_memfn_call_tracker{};
        }

        bool construct_called{};
        bool destroy_called{};

        bool end_called{}, end_const_called{};
        bool reallocate_called{}, reallocate_assign_called{};

        bool empty_called{}, full_called{};
        bool inc_size_called{}, dec_size_called{}, max_size_called{}, swap_called{};
} call_tracker;

// storage type, which fails to provide custom member functions for optional requirements:
struct bad_storage : base_storage
{
        // wrong parameter lists for construct() and destroy():
        void construct(int) const
        {
                call_tracker.construct_called = true;
        }

        void destroy(long*)
        {
                call_tracker.destroy_called = true;
        }

        // wrong return types for end():
        int end() noexcept
        {
                call_tracker.end_called = true;
                return 0;
        }

        int end() const noexcept
        {
                call_tracker.end_const_called = true;
                return 0;
        }

        // wrong parameter lists for reallocate() and reallocate_assign():
        bool reallocate(int*)
        {
                call_tracker.reallocate_called = true;
                return false;
        }

        bool reallocate_assign(int, int)
        {
                call_tracker.reallocate_assign_called = true;
                return false;
        }

        // non-const empty() and full():
        bool empty() noexcept
        {
                call_tracker.empty_called = true;
                return false;
        }

        bool full() noexcept
        {
                call_tracker.full_called = true;
                return false;
        }

        //
        // wrong parameter lists for inc_size() and dec_size():
        void inc_size(int*) noexcept
        {
                call_tracker.inc_size_called = true;
        }

        void dec_size(int*) noexcept
        {
                call_tracker.dec_size_called = true;
        }

        //
        // non-const max_size():
        std::size_t max_size() noexcept
        {
                call_tracker.max_size_called = true;
                return 0;
        }

        // wrong parameter list for swap():
        void swap(int)
        {
                call_tracker.swap_called = true;
        }
};

TEST_CASE("bad_storage", "[ecs::storage_traits]")
{
        using traits = ecs::storage_traits<bad_storage>;

        static_assert(std::is_same<traits::pointer, int*>::value);
        static_assert(std::is_same<traits::const_pointer, const int*>::value);

        static_assert(std::is_same<traits::size_type, std::size_t>::value);
        static_assert(std::is_same<traits::difference_type, std::ptrdiff_t>::value);

        int value{};
        bad_storage storage, other_storage;
        const bad_storage& const_storage = storage;

        //
        call_tracker.reset();
        REQUIRE(!call_tracker.construct_called);
        REQUIRE(!call_tracker.destroy_called);

        REQUIRE(!call_tracker.end_called);
        REQUIRE(!call_tracker.end_const_called);

        REQUIRE(!call_tracker.reallocate_called);
        REQUIRE(!call_tracker.reallocate_assign_called);

        REQUIRE(!call_tracker.empty_called);
        REQUIRE(!call_tracker.full_called);

        REQUIRE(!call_tracker.inc_size_called);
        REQUIRE(!call_tracker.dec_size_called);
        REQUIRE(!call_tracker.max_size_called);
        REQUIRE(!call_tracker.swap_called);

        //
        traits::construct(storage, &value, 5);
        traits::destroy(storage, &value);
        REQUIRE(value == 5);

        REQUIRE(!call_tracker.construct_called);
        REQUIRE(!call_tracker.destroy_called);

        //
        REQUIRE(traits::begin(storage) == traits::end(storage));
        REQUIRE(traits::begin(const_storage) == traits::end(const_storage));

        REQUIRE(!call_tracker.end_called);
        REQUIRE(!call_tracker.end_const_called);

        //
        REQUIRE(!traits::reallocate(storage, 0));
        REQUIRE(!traits::reallocate_assign(storage, 0, &value));

        REQUIRE(!call_tracker.reallocate_called);
        REQUIRE(!call_tracker.reallocate_assign_called);

        //
        REQUIRE(traits::empty(const_storage));
        REQUIRE(traits::full(const_storage));

        REQUIRE(!call_tracker.empty_called);
        REQUIRE(!call_tracker.full_called);

        //
        traits::set_size(storage, 5);
        REQUIRE(traits::size(const_storage) == 5);

        traits::inc_size(storage);
        REQUIRE(traits::size(const_storage) == 6);
        traits::inc_size(storage, 3);
        REQUIRE(traits::size(const_storage) == 9);

        traits::dec_size(storage);
        REQUIRE(traits::size(const_storage) == 8);
        traits::dec_size(storage, 7);
        REQUIRE(traits::size(const_storage) == 1);

        REQUIRE(!call_tracker.inc_size_called);
        REQUIRE(!call_tracker.dec_size_called);

        //
        REQUIRE(traits::capacity(const_storage) == 1);
        REQUIRE(traits::max_size(const_storage) ==
                (std::numeric_limits<std::ptrdiff_t>::max() / sizeof(int)));

        REQUIRE(!call_tracker.max_size_called);

        //
        REQUIRE(traits::size(other_storage) == 0);
        REQUIRE(traits::size(const_storage) == 1);
        traits::swap(storage, other_storage);
        REQUIRE(traits::size(other_storage) == 1);
        REQUIRE(traits::size(const_storage) == 0);

        REQUIRE(!call_tracker.swap_called);
}

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

        using size_type = unsigned long long;
        using difference_type = signed long long;

        void swap(good_storage&)
        {
                call_tracker.swap_called = true;
        }

private:
        template <typename... Args>
        void construct(pointer, Args&&...)
        {
                call_tracker.construct_called = true;
        }

        void destroy(pointer) noexcept
        {
                call_tracker.destroy_called = true;
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
                call_tracker.end_called = true;
                return pointer{};
        }

        const_pointer end() const noexcept
        {
                call_tracker.end_const_called = true;
                return const_pointer{};
        }

        //
        bool reallocate(std::size_t)
        {
                call_tracker.reallocate_called = true;
                return false;
        }

        template <typename ForwardIterator>
        bool reallocate_assign(std::size_t, ForwardIterator)
        {
                call_tracker.reallocate_assign_called = true;
                return false;
        }

        //
        bool empty() const noexcept
        {
                call_tracker.empty_called = true;
                return true;
        }

        bool full() const noexcept
        {
                call_tracker.full_called = true;
                return true;
        }

        //
        void inc_size(size_type) noexcept
        {
                call_tracker.inc_size_called = true;
        }

        void dec_size(size_type) noexcept
        {
                call_tracker.dec_size_called = true;
        }

        size_type max_size() const noexcept
        {
                call_tracker.max_size_called = true;
                return 0;
        }

        friend struct ecs::storage_traits<good_storage>;
};

TEST_CASE("good_storage", "[ecs::storage_traits]")
{
        using traits = ecs::storage_traits<good_storage>;

        static_assert(std::is_same<traits::pointer, good_storage::pointer>::value);
        static_assert(std::is_same<traits::const_pointer, good_storage::const_pointer>::value);

        static_assert(std::is_same<traits::size_type, good_storage::size_type>::value);
        static_assert(std::is_same<traits::difference_type, good_storage::difference_type>::value);

        good_storage storage, other_storage;
        const good_storage& const_storage = storage;

        //
        call_tracker.reset();
        REQUIRE(!call_tracker.construct_called);
        REQUIRE(!call_tracker.destroy_called);

        REQUIRE(!call_tracker.end_called);
        REQUIRE(!call_tracker.end_const_called);

        REQUIRE(!call_tracker.reallocate_called);
        REQUIRE(!call_tracker.reallocate_assign_called);

        REQUIRE(!call_tracker.empty_called);
        REQUIRE(!call_tracker.full_called);

        REQUIRE(!call_tracker.inc_size_called);
        REQUIRE(!call_tracker.dec_size_called);
        REQUIRE(!call_tracker.max_size_called);
        REQUIRE(!call_tracker.swap_called);

        //
        traits::construct(storage, good_storage::pointer{}, 5);
        traits::destroy(storage, good_storage::pointer{});

        REQUIRE(call_tracker.construct_called);
        REQUIRE(call_tracker.destroy_called);

        //
        traits::end(storage);
        traits::end(const_storage);

        REQUIRE(call_tracker.end_called);
        REQUIRE(call_tracker.end_const_called);

        //
        REQUIRE(!traits::reallocate(storage, 0));
        REQUIRE(!traits::reallocate_assign(storage, 0, (int*)nullptr));

        REQUIRE(call_tracker.reallocate_called);
        REQUIRE(call_tracker.reallocate_assign_called);

        //
        REQUIRE(traits::empty(const_storage));
        REQUIRE(traits::full(const_storage));

        REQUIRE(call_tracker.empty_called);
        REQUIRE(call_tracker.full_called);

        //
        traits::set_size(storage, 5);
        REQUIRE(traits::size(const_storage) == 5);

        traits::inc_size(storage);
        REQUIRE(traits::size(const_storage) == 5);
        traits::inc_size(storage, 3);
        REQUIRE(traits::size(const_storage) == 5);

        traits::dec_size(storage);
        REQUIRE(traits::size(const_storage) == 5);
        traits::dec_size(storage, 3);
        REQUIRE(traits::size(const_storage) == 5);

        REQUIRE(call_tracker.inc_size_called);
        REQUIRE(call_tracker.dec_size_called);

        //
        REQUIRE(traits::capacity(const_storage) == 5);
        REQUIRE(traits::max_size(const_storage) == 0);

        REQUIRE(call_tracker.max_size_called);

        //
        REQUIRE(traits::size(other_storage) == 0);
        REQUIRE(traits::size(const_storage) == 5);
        traits::swap(storage, other_storage);
        REQUIRE(traits::size(other_storage) == 0);
        REQUIRE(traits::size(const_storage) == 5);

        REQUIRE(call_tracker.swap_called);
}

//
} // namespace storage_traits_testing
