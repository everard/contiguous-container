// Copyright Ildus Nezametdinov 2017.
// Distributed under the Boost Software License, Version 1.0.
//(See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "storage_traits_tests.h"

namespace contiguous_container_testing
{
// helper types:
using count = unsigned long long;
using identifier = unsigned long long;

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

        log_entry(op_type op_, identifier src_, identifier dst_) : op{op_}, src{src_}, dst{dst_}
        {
        }

        //
        bool operator==(const log_entry& other)
        {
                return op == other.op && src == other.src && dst == other.dst;
        }

        bool operator!=(const log_entry& other)
        {
                return !(*this == other);
        }

        //
        op_type op;
        identifier src, dst;
};

std::vector<log_entry> global_log{};
identifier next_identifier{};

// type, which is used for testing the behavior:
struct tracker
{
        // construct/destroy:
        tracker()
        {
                ++n_default_constructor_calls;
                global_log.emplace_back(log_entry::op_type::default_construct, id, id);
        }

        tracker(int x_) : x{x_}
        {
                ++n_non_default_constructor_calls;
                global_log.emplace_back(log_entry::op_type::non_default_construct, id, id);
        }

        tracker(const tracker& other) : x{other.x}
        {
                ++n_copy_constructor_calls;
                global_log.emplace_back(log_entry::op_type::copy_construct, other.id, id);
        }

        tracker(tracker&& other) : x{other.x}
        {
                ++n_move_constructor_calls;
                global_log.emplace_back(log_entry::op_type::move_construct, other.id, id);
        }

        ~tracker()
        {
                ++n_destructor_calls;
                global_log.emplace_back(log_entry::op_type::destroy, id, id);
        }

        // assign:
        tracker& operator=(const tracker& other)
        {
                global_log.emplace_back(log_entry::op_type::copy, other.id, id);

                ++n_copy_assignments;
                x = other.x;

                return *this;
        }

        tracker& operator=(tracker&& other)
        {
                global_log.emplace_back(log_entry::op_type::move, other.id, id);

                ++n_move_assignments;
                x = other.x;

                return *this;
        }

        // data member
        int x{};
        identifier id{next_identifier++};

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

        //
        count n_construct_calls{}, n_destroy_calls{};

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

//
template <std::size_t N>
using container = ecs::contiguous_container<tracked_storage<tracker, N>>;

template <std::size_t N>
bool check_container(const container<N>& c, std::array<int, N> a)
{
        return std::equal(c.begin(), c.end(), a.begin(), [](auto& x, auto& y) { return x.x == y; });
}

template <std::size_t N>
bool check_log(std::array<log_entry, N> log)
{
        if(log.size() != global_log.size())
                return false;

        return std::equal(log.begin(), log.end(), global_log.begin());
}

TEST_CASE("core container functionality", "[contiguous_container]")
{
        global_log.clear();
        next_identifier = 0;

        container<5> c;

        REQUIRE(c.size() == 0);
        REQUIRE(c.capacity() == 5);

        REQUIRE(c.empty());
        REQUIRE(!c.full());

        SECTION("empty, full, size; emplace_back, clear:")
        {
                // construct(ids): 0
                auto p = c.emplace_back(1);
                REQUIRE(p != c.end());
                REQUIRE(p->x == 1);

                REQUIRE(c.size() == 1);
                REQUIRE(!c.empty());
                REQUIRE(!c.full());

                // construct(ids): 1
                p = c.emplace_back();
                REQUIRE(p != c.end());
                REQUIRE(p->x == 0);

                REQUIRE(c.size() == 2);
                REQUIRE(!c.empty());
                REQUIRE(!c.full());

                // construct(ids): 2
                p = c.emplace_back(3);
                REQUIRE(p != c.end());
                REQUIRE(p->x == 3);

                REQUIRE(c.size() == 3);
                REQUIRE(!c.empty());
                REQUIRE(!c.full());

                // construct(ids): 3
                p = c.emplace_back(4);
                REQUIRE(p != c.end());
                REQUIRE(p->x == 4);

                REQUIRE(c.size() == 4);
                REQUIRE(!c.empty());
                REQUIRE(!c.full());

                // construct(ids): 4
                p = c.emplace_back(5);
                REQUIRE(p != c.end());
                REQUIRE(p->x == 5);

                REQUIRE(c.size() == 5);
                REQUIRE(!c.empty());
                REQUIRE(c.full());

                // construct(ids): none
                p = c.emplace_back(6);
                REQUIRE(p == c.end());

                REQUIRE(c.size() == 5);
                REQUIRE(!c.empty());
                REQUIRE(c.full());

                // identifiers: 0, 1, 2, 3, 4
                REQUIRE(check_container(c, {{1, 0, 3, 4, 5}}));

                // destroy(ids): 0, 1, 2, 3 ,4
                c.clear();
                REQUIRE(c.size() == 0);
                REQUIRE(c.empty());
                REQUIRE(!c.full());

                REQUIRE(check_log(std::array<log_entry, 10>{
                        {log_entry{log_entry::op_type::non_default_construct, 0, 0},
                         log_entry{log_entry::op_type::default_construct, 1, 1},
                         log_entry{log_entry::op_type::non_default_construct, 2, 2},
                         log_entry{log_entry::op_type::non_default_construct, 3, 3},
                         log_entry{log_entry::op_type::non_default_construct, 4, 4},
                         log_entry{log_entry::op_type::destroy, 0, 0},
                         log_entry{log_entry::op_type::destroy, 1, 1},
                         log_entry{log_entry::op_type::destroy, 2, 2},
                         log_entry{log_entry::op_type::destroy, 3, 3},
                         log_entry{log_entry::op_type::destroy, 4, 4}}}));

                REQUIRE(c.n_construct_calls == 5);
                REQUIRE(c.n_destroy_calls == 5);
        }

        SECTION("push_back/pop_back/erase:")
        {
                // construct(ids): 0, 1
                tracker x0, x1{7};

                // construct(ids): 2
                auto p = c.emplace_back(1);
                REQUIRE(p != c.end());
                REQUIRE(p->x == 1);

                // construct(ids): 3
                p = c.emplace_back(3);
                REQUIRE(p != c.end());
                REQUIRE(p->x == 3);

                // construct(ids): 4
                p = c.emplace_back(5);
                REQUIRE(p != c.end());
                REQUIRE(p->x == 5);

                // construct(ids): 5 copy from 0
                p = c.push_back(x0);
                REQUIRE(p != c.end());
                REQUIRE(p->x == 0);

                // construct(ids): 6 move from 1
                p = c.push_back(std::move(x1));
                REQUIRE(p != c.end());
                REQUIRE(p->x == 7);

                // identifiers: 2, 3, 4, 5, 6
                REQUIRE(c.size() == 5);
                REQUIRE(check_container(c, {{1, 3, 5, 0, 7}}));

                // move(ids): 3 -> 2, 4 -> 3, 5 -> 4, 6 -> 5
                // destroy(ids): 6
                p = c.erase(c.begin());
                REQUIRE(p != c.end());
                REQUIRE(p->x == 3);

                // identifiers: 2, 3, 4, 5
                REQUIRE(c.size() == 4);
                REQUIRE(check_container(c, {{3, 5, 0, 7}}));

                // construct(ids): 7
                p = c.emplace_back(11);
                REQUIRE(p != c.end());
                REQUIRE(p->x == 11);

                // identifiers: 2, 3, 4, 5, 7
                REQUIRE(c.size() == 5);
                REQUIRE(check_container(c, {{3, 5, 0, 7, 11}}));

                // move(ids): 5 -> 2, 7 -> 3
                // destroy(ids): 4, 5, 7
                p = c.erase(c.begin(), c.begin() + 3);
                REQUIRE(p != c.end());
                REQUIRE(p->x == 7);

                // identifiers: 2, 3
                REQUIRE(c.size() == 2);
                REQUIRE(check_container(c, {{7, 11}}));

                // destroy(ids): 3
                c.pop_back();

                // identifiers: 2
                REQUIRE(c.size() == 1);
                REQUIRE(check_container(c, {{7}}));

                REQUIRE(check_log(std::array<log_entry, 19>{
                        {log_entry{log_entry::op_type::default_construct, 0, 0},
                         log_entry{log_entry::op_type::non_default_construct, 1, 1},
                         log_entry{log_entry::op_type::non_default_construct, 2, 2},
                         log_entry{log_entry::op_type::non_default_construct, 3, 3},
                         log_entry{log_entry::op_type::non_default_construct, 4, 4},
                         log_entry{log_entry::op_type::copy_construct, 0, 5},
                         log_entry{log_entry::op_type::move_construct, 1, 6},
                         log_entry{log_entry::op_type::move, 3, 2},
                         log_entry{log_entry::op_type::move, 4, 3},
                         log_entry{log_entry::op_type::move, 5, 4},
                         log_entry{log_entry::op_type::move, 6, 5},
                         log_entry{log_entry::op_type::destroy, 6, 6},
                         log_entry{log_entry::op_type::non_default_construct, 7, 7},
                         log_entry{log_entry::op_type::move, 5, 2},
                         log_entry{log_entry::op_type::move, 7, 3},
                         log_entry{log_entry::op_type::destroy, 4, 4},
                         log_entry{log_entry::op_type::destroy, 5, 5},
                         log_entry{log_entry::op_type::destroy, 7, 7},
                         log_entry{log_entry::op_type::destroy, 3, 3}}}));

                REQUIRE(c.n_construct_calls == 6);
                REQUIRE(c.n_destroy_calls == 5);
        }
}

//
} // namespace contiguous_container_testing
