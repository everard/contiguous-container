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
        constexpr bool operator==(const input_iterator_adaptor& other) const
        {
                return base_ == other.base_;
        }

        constexpr bool operator!=(const input_iterator_adaptor& other) const
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
                global_log.emplace_back(log_entry::op_type::default_construct, id, id);
        }

        tracker(int x_) : x{x_}
        {
                global_log.emplace_back(log_entry::op_type::non_default_construct, id, id);
        }

        tracker(const tracker& other) : x{other.x}
        {
                global_log.emplace_back(log_entry::op_type::copy_construct, other.id, id);
        }

        tracker(tracker&& other) : x{other.x}
        {
                global_log.emplace_back(log_entry::op_type::move_construct, other.id, id);
        }

        ~tracker()
        {
                global_log.emplace_back(log_entry::op_type::destroy, id, id);
        }

        // assign:
        tracker& operator=(const tracker& other)
        {
                global_log.emplace_back(log_entry::op_type::copy, other.id, id);

                x = other.x;
                return *this;
        }

        tracker& operator=(tracker&& other)
        {
                global_log.emplace_back(log_entry::op_type::move, other.id, id);

                x = other.x;
                return *this;
        }

        // data members:
        int x{};
        identifier id{next_identifier++};
};

// simple storage type, which tracks calls to construct/destroy
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

        SECTION("push_back, pop_back, erase:")
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

        SECTION("emplace, insert single element:")
        {
                // construct(ids): 0, 1, 2, 3
                c.emplace_back(1);
                c.emplace_back(2);
                c.emplace_back(3);
                c.emplace_back(4);

                // identifiers: 0, 1, 2, 3
                REQUIRE(c.size() == 4);
                REQUIRE(check_container(c, {{1, 2, 3, 4}}));

                // construct(ids): 4, 5 move from 3
                // move(ids): 2 -> 3, 1 -> 2, 0 -> 1, 4 -> 0
                // destroy(ids): 4
                auto p = c.emplace(c.begin(), 10);
                REQUIRE(p->x == 10);

                // identifiers: 0, 1, 2, 3, 5
                REQUIRE(c.size() == 5);
                REQUIRE(check_container(c, {{10, 1, 2, 3, 4}}));

                // destroy(ids): 0, 1, 2, 3, 5
                c.erase(c.begin(), c.end());
                REQUIRE(c.size() == 0);
                REQUIRE(c.empty());

                // construct(ids): 6, 7
                c.emplace_back(11);
                c.emplace_back(12);

                // identifiers: 6, 7
                REQUIRE(c.size() == 2);
                REQUIRE(check_container(c, {{11, 12}}));

                // construct(ids): 8
                tracker x0;

                // construct(ids): 9 copy from 8, 10 move from 7
                // move(ids): 6 -> 7, 9 -> 6
                // destroy(ids): 9
                c.insert(c.begin(), x0);

                // identifiers: 6, 7, 10
                REQUIRE(c.size() == 3);
                REQUIRE(check_container(c, {{0, 11, 12}}));

                // construct(ids): 11
                tracker x1{17};

                // construct(ids): 12 move from 11, 13 move from 10
                // move(ids): 7 -> 10, 12 -> 7
                // destroy(ids): 12
                c.insert(c.begin() + 1, std::move(x1));

                // identifiers: 6, 7, 10, 13
                REQUIRE(c.size() == 4);
                REQUIRE(check_container(c, {{0, 17, 11, 12}}));

                // construct(ids): 14
                c.emplace_back(29);

                // identifiers: 6, 7, 10, 13, 14
                REQUIRE(c.size() == 5);
                REQUIRE(check_container(c, {{0, 17, 11, 12, 29}}));

                // construct(ids): 15
                // destroy(ids): 15
                p = c.emplace(c.begin(), 33);
                REQUIRE(p == c.end());

                // identifiers: 6, 7, 10, 13, 14
                REQUIRE(c.size() == 5);
                REQUIRE(check_container(c, {{0, 17, 11, 12, 29}}));

                //
                REQUIRE(check_log(std::array<log_entry, 33>{
                        {log_entry{log_entry::op_type::non_default_construct, 0, 0},
                         log_entry{log_entry::op_type::non_default_construct, 1, 1},
                         log_entry{log_entry::op_type::non_default_construct, 2, 2},
                         log_entry{log_entry::op_type::non_default_construct, 3, 3},
                         log_entry{log_entry::op_type::non_default_construct, 4, 4},
                         log_entry{log_entry::op_type::move_construct, 3, 5},
                         log_entry{log_entry::op_type::move, 2, 3},
                         log_entry{log_entry::op_type::move, 1, 2},
                         log_entry{log_entry::op_type::move, 0, 1},
                         log_entry{log_entry::op_type::move, 4, 0},
                         log_entry{log_entry::op_type::destroy, 4, 4},
                         log_entry{log_entry::op_type::destroy, 0, 0},
                         log_entry{log_entry::op_type::destroy, 1, 1},
                         log_entry{log_entry::op_type::destroy, 2, 2},
                         log_entry{log_entry::op_type::destroy, 3, 3},
                         log_entry{log_entry::op_type::destroy, 5, 5},
                         log_entry{log_entry::op_type::non_default_construct, 6, 6},
                         log_entry{log_entry::op_type::non_default_construct, 7, 7},
                         log_entry{log_entry::op_type::default_construct, 8, 8},
                         log_entry{log_entry::op_type::copy_construct, 8, 9},
                         log_entry{log_entry::op_type::move_construct, 7, 10},
                         log_entry{log_entry::op_type::move, 6, 7},
                         log_entry{log_entry::op_type::move, 9, 6},
                         log_entry{log_entry::op_type::destroy, 9, 9},
                         log_entry{log_entry::op_type::non_default_construct, 11, 11},
                         log_entry{log_entry::op_type::move_construct, 11, 12},
                         log_entry{log_entry::op_type::move_construct, 10, 13},
                         log_entry{log_entry::op_type::move, 7, 10},
                         log_entry{log_entry::op_type::move, 12, 7},
                         log_entry{log_entry::op_type::destroy, 12, 12},
                         log_entry{log_entry::op_type::non_default_construct, 14, 14},
                         log_entry{log_entry::op_type::non_default_construct, 15, 15},
                         log_entry{log_entry::op_type::destroy, 15, 15}}}));

                REQUIRE(c.n_construct_calls == 10);
                REQUIRE(c.n_destroy_calls == 5);
        }

        SECTION("insert multiple elements using forward iterators:")
        {
                // construct(ids): 0, 1, 2
                c.emplace_back(1);
                c.emplace_back(2);
                c.emplace_back(3);

                // identifiers: 0, 1, 2
                REQUIRE(c.size() == 3);
                REQUIRE(check_container(c, {{1, 2, 3}}));

                // construct(ids): 3, 4 move from 2, 5
                // move(ids): 5 -> 2
                // destroy(ids): 5
                int v[] = {11, 12};

                auto p = c.insert(c.begin() + 2, std::begin(v), std::end(v));
                REQUIRE(p->x == 11);

                // identifiers: 0, 1, 2, 3, 4
                REQUIRE(c.size() == 5);
                REQUIRE(check_container(c, {{1, 2, 11, 12, 3}}));

                // destroy(ids): 0, 1, 2, 3, 4
                c.clear();

                // construct(ids): 6, 7, 8
                c.emplace_back(1);
                c.emplace_back(2);
                c.emplace_back(3);

                // identifiers: 6, 7, 8
                REQUIRE(c.size() == 3);
                REQUIRE(check_container(c, {{1, 2, 3}}));

                // construct(ids): 9 move from 7, 10 move from 8
                // move(ids): 6 -> 8
                // construct(ids): 11
                // move(ids): 11 -> 6
                // destroy(ids): 11
                // construct(ids): 12
                // move(ids): 12 -> 7
                // destroy(ids): 12
                p = c.insert(c.begin(), std::begin(v), std::end(v));
                REQUIRE(p->x == 11);

                // identifiers: 6, 7, 8, 9, 10
                REQUIRE(c.size() == 5);
                REQUIRE(check_container(c, {{11, 12, 1, 2, 3}}));

                //
                REQUIRE(check_log(std::array<log_entry, 25>{
                        {log_entry{log_entry::op_type::non_default_construct, 0, 0},
                         log_entry{log_entry::op_type::non_default_construct, 1, 1},
                         log_entry{log_entry::op_type::non_default_construct, 2, 2},
                         log_entry{log_entry::op_type::non_default_construct, 3, 3},
                         log_entry{log_entry::op_type::move_construct, 2, 4},
                         log_entry{log_entry::op_type::non_default_construct, 5, 5},
                         log_entry{log_entry::op_type::move, 5, 2},
                         log_entry{log_entry::op_type::destroy, 5, 5},
                         log_entry{log_entry::op_type::destroy, 0, 0},
                         log_entry{log_entry::op_type::destroy, 1, 1},
                         log_entry{log_entry::op_type::destroy, 2, 2},
                         log_entry{log_entry::op_type::destroy, 3, 3},
                         log_entry{log_entry::op_type::destroy, 4, 4},
                         log_entry{log_entry::op_type::non_default_construct, 6, 6},
                         log_entry{log_entry::op_type::non_default_construct, 7, 7},
                         log_entry{log_entry::op_type::non_default_construct, 8, 8},
                         log_entry{log_entry::op_type::move_construct, 7, 9},
                         log_entry{log_entry::op_type::move_construct, 8, 10},
                         log_entry{log_entry::op_type::move, 6, 8},
                         log_entry{log_entry::op_type::non_default_construct, 11, 11},
                         log_entry{log_entry::op_type::move, 11, 6},
                         log_entry{log_entry::op_type::destroy, 11, 11},
                         log_entry{log_entry::op_type::non_default_construct, 12, 12},
                         log_entry{log_entry::op_type::move, 12, 7},
                         log_entry{log_entry::op_type::destroy, 12, 12}}}));

                REQUIRE(c.n_construct_calls == 10);
                REQUIRE(c.n_destroy_calls == 5);
        }
}

//
} // namespace contiguous_container_testing
