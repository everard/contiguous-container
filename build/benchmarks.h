// Copyright Ildus Nezametdinov 2016.
// Distributed under the Boost Software License, Version 1.0.
//(See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "../source/experimental/contiguous_container.h"
#include <benchmark/benchmark.h>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <chrono>

static void opt_escape(void* p)
{
        asm volatile("" : : "g"(p) : "memory");
}

static void opt_clobber()
{
        asm volatile("" : : : "memory");
}

template <typename T, std::size_t N>
struct uninitialized_memory_buffer
{
        using value_type = T;
        using no_exceptions = std::true_type;

        ~uninitialized_memory_buffer()
        {
                if(!std::is_trivially_destructible<T>::value)
                {
                        for(std::size_t i = 0; i < size_; ++i)
                                (begin() + i)->~T();
                }
        }

        template <typename... Args>
        void construct_at(T* location, Args&&... args)
        {
                ::new(location) T{std::forward<Args>(args)...};
        }

        void destroy_at(T* location) noexcept
        {
                if /*constexpr*/ (!std::is_trivially_destructible<T>::value)
                        location->~T();
        }

        static constexpr auto reserve(std::size_t) noexcept
        {
                return false;
        }

        T* begin() noexcept
        {
                return reinterpret_cast<T*>(storage_);
        }

        const T* begin() const noexcept
        {
                return reinterpret_cast<const T*>(storage_);
        }

        constexpr auto& size() noexcept
        {
                return size_;
        }

        constexpr auto& size() const noexcept
        {
                return size_;
        }

        constexpr auto max_size() const noexcept
        {
                return N;
        }

        constexpr auto capacity() const noexcept
        {
                return N;
        }

private:
        alignas(T) unsigned char storage_[N * sizeof(T)];
        std::size_t size_{};
};

template <typename T>
struct dynamic_uninitialized_memory_buffer
{
        using value_type = T;
        using no_exceptions = std::true_type;

        ~dynamic_uninitialized_memory_buffer()
        {
                if(!std::is_trivially_destructible<T>::value)
                {
                        for(std::size_t i = 0; i < size_; ++i)
                                begin()[i].~T();
                }
        }

        template <typename... Args>
        void construct_at(T* location, Args&&... args)
        {
                ::new(location) T{std::forward<Args>(args)...};
        }

        void destroy_at(T* location) noexcept
        {
                if /*constexpr*/ (!std::is_trivially_destructible<T>::value)
                        location->~T();
        }

        auto reserve(std::size_t n)
        {
                if(n >= max_size())
                        throw std::length_error("cc");

                if(n <= capacity())
                        return true;

                auto new_capacity = std::max(size() + size(), n);
                new_capacity = (new_capacity < size() || new_capacity > max_size()) ? max_size()
                                                                                    : new_capacity;

                std::unique_ptr<unsigned char[]> new_storage{
                        new unsigned char[new_capacity * sizeof(T)]};
                auto new_data = reinterpret_cast<T*>(new_storage.get());
                std::size_t n_constructed{};

                try
                {
                        std::experimental::for_each_iter(
                                begin(), begin() + size(), new_data, [&](auto i, auto j) {
                                        this->construct_at(j, std::move_if_noexcept(*i));
                                        ++n_constructed;
                                });
                }
                catch(...)
                {
                        for(std::size_t i = 0; i < n_constructed; ++i)
                                destroy_at(new_data + i);

                        throw;
                }

                for(std::size_t i = 0; i < size(); ++i)
                        destroy_at(begin() + i);

                std::swap(storage_, new_storage);
                capacity_ = new_capacity;
                return true;
        }

        T* begin() noexcept
        {
                return reinterpret_cast<T*>(storage_.get());
        }

        const T* begin() const noexcept
        {
                return reinterpret_cast<const T*>(storage_.get());
        }

        constexpr auto& size() noexcept
        {
                return size_;
        }

        constexpr auto& size() const noexcept
        {
                return size_;
        }

        constexpr auto max_size() const noexcept
        {
                return static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max());
        }

        constexpr auto capacity() const noexcept
        {
                return capacity_;
        }

private:
        std::unique_ptr<unsigned char[]> storage_{};
        std::size_t size_{}, capacity_{};
};

struct non_trivial
{
        non_trivial() = default;
        non_trivial(int x) : v{x}
        {
        }

        non_trivial(const non_trivial& o) : v{o.v}
        {
        }
        non_trivial(non_trivial&& o) : v{o.v}
        {
        }

        non_trivial& operator=(const non_trivial& rhs)
        {
                v = rhs.v;
                return *this;
        }
        non_trivial& operator=(non_trivial&& rhs)
        {
                v = rhs.v;
                return *this;
        }

        ~non_trivial()
        {
                v = 0;
        }

        int v{};
};

//
template <typename Container>
void test_container_performance_0(Container& arr)
{
        arr.emplace_back(1);
        arr.emplace_back(2);
        arr.emplace_back(3);
        arr.emplace_back(4);

        non_trivial s0{5}, s1{8};
        arr.push_back(s0);
        arr.push_back(std::move(s1));
        opt_clobber();
}

template <typename Container>
void test_container_performance_1(Container& arr)
{
        // 6 - 3
        arr.erase(arr.begin() + 1, arr.begin() + 3);
        arr.erase(arr.begin());
        opt_clobber();
}

template <typename Container>
void test_container_performance_2(Container& arr)
{
        // 3 + 1
        arr.emplace(arr.begin() + 1, 15);
        opt_clobber();
}

template <typename Container>
void test_container_performance_3(Container& arr)
{
        // 4 + 4
        arr.insert(arr.begin() + 1, {101, 102, 103, 104});
        opt_clobber();
}

template <typename Container>
void test_container_performance_4(Container& arr)
{
        // 8 + 5
        int a1[] = {201, 202, 203, 204, 205};
        arr.insert(arr.begin() + 4, std::begin(a1), std::end(a1));
        opt_clobber();
}

template <typename Container>
void test_container_performance_5(Container& arr)
{
        // 13 + 7
        arr.insert(arr.end(), 7, 777);
        opt_clobber();
}

template <typename Container>
void test_container_performance_6(Container& arr)
{
        // 20 - 0
        arr.erase(arr.begin(), arr.begin());
        opt_clobber();
}

template <typename Container>
void test_container_performance_7(Container& arr)
{
        arr.assign({1001, 1002, 1003, 1004, 1005});
        opt_clobber();
}

template <typename Container>
void test_container_performance_8(Container& arr)
{
        arr.assign({2001, 2002, 2003, 2004, 2005, 2006, 2007});
        opt_clobber();
}

////////////////////////// Benchmarks
#define BM_M_ContainerBaseline(C)       \
        while(state.KeepRunning())      \
        {                               \
                C arr;                  \
                arr.reserve(32);        \
                opt_escape(arr.data()); \
        }

#define BM_M_ContainerEmplaceBack(C)               \
        while(state.KeepRunning())                 \
        {                                          \
                C arr;                             \
                arr.reserve(32);                   \
                opt_escape(arr.data());            \
                test_container_performance_0(arr); \
                opt_clobber();                     \
        }

#define BM_M_ContainerErase(C)                     \
        while(state.KeepRunning())                 \
        {                                          \
                C arr;                             \
                arr.reserve(32);                   \
                opt_escape(arr.data());            \
                test_container_performance_0(arr); \
                test_container_performance_1(arr); \
                opt_clobber();                     \
        }

#define BM_M_ContainerEmplace(C)                   \
        while(state.KeepRunning())                 \
        {                                          \
                C arr;                             \
                arr.reserve(32);                   \
                opt_escape(arr.data());            \
                test_container_performance_0(arr); \
                test_container_performance_1(arr); \
                test_container_performance_2(arr); \
                opt_clobber();                     \
        }

#define BM_M_ContainerInsert0(C)                   \
        while(state.KeepRunning())                 \
        {                                          \
                C arr;                             \
                arr.reserve(32);                   \
                opt_escape(arr.data());            \
                test_container_performance_0(arr); \
                test_container_performance_1(arr); \
                test_container_performance_2(arr); \
                test_container_performance_3(arr); \
                opt_clobber();                     \
        }

#define BM_M_ContainerInsert1(C)                   \
        while(state.KeepRunning())                 \
        {                                          \
                C arr;                             \
                arr.reserve(32);                   \
                opt_escape(arr.data());            \
                test_container_performance_0(arr); \
                test_container_performance_1(arr); \
                test_container_performance_2(arr); \
                test_container_performance_3(arr); \
                test_container_performance_4(arr); \
                opt_clobber();                     \
        }

#define BM_M_ContainerInsert2(C)                   \
        while(state.KeepRunning())                 \
        {                                          \
                C arr;                             \
                arr.reserve(32);                   \
                opt_escape(arr.data());            \
                test_container_performance_0(arr); \
                test_container_performance_1(arr); \
                test_container_performance_2(arr); \
                test_container_performance_3(arr); \
                test_container_performance_4(arr); \
                test_container_performance_5(arr); \
                opt_clobber();                     \
        }

#define BM_M_ContainerEraseEmpty(C)                \
        while(state.KeepRunning())                 \
        {                                          \
                C arr;                             \
                arr.reserve(32);                   \
                opt_escape(arr.data());            \
                test_container_performance_0(arr); \
                test_container_performance_1(arr); \
                test_container_performance_2(arr); \
                test_container_performance_3(arr); \
                test_container_performance_4(arr); \
                test_container_performance_5(arr); \
                test_container_performance_6(arr); \
                opt_clobber();                     \
        }

// Vector
using c_vector = std::vector<non_trivial>;
static void BM_VectorBaseline(benchmark::State& state)
{
        BM_M_ContainerBaseline(c_vector)
}
static void BM_VectorEmplaceBack(benchmark::State& state)
{
        BM_M_ContainerEmplaceBack(c_vector)
}
static void BM_VectorErase(benchmark::State& state)
{
        BM_M_ContainerErase(c_vector)
}
static void BM_VectorEmplace(benchmark::State& state)
{
        BM_M_ContainerEmplace(c_vector)
}
static void BM_VectorInsert0(benchmark::State& state)
{
        BM_M_ContainerInsert0(c_vector)
}
static void BM_VectorInsert1(benchmark::State& state)
{
        BM_M_ContainerInsert1(c_vector)
}
static void BM_VectorInsert2(benchmark::State& state)
{
        BM_M_ContainerInsert2(c_vector)
}
static void BM_VectorEraseEmpty(benchmark::State& state)
{
        BM_M_ContainerEraseEmpty(c_vector)
}

// Contiguous container
using c_container =
        std::experimental::contiguous_container<dynamic_uninitialized_memory_buffer<non_trivial>>;
static void BM_ContiguousContainerBaseline(benchmark::State& state)
{
        BM_M_ContainerBaseline(c_container)
}
static void BM_ContiguousContainerEmplaceBack(benchmark::State& state)
{
        BM_M_ContainerEmplaceBack(c_container)
}
static void BM_ContiguousContainerErase(benchmark::State& state)
{
        BM_M_ContainerErase(c_container)
}
static void BM_ContiguousContainerEmplace(benchmark::State& state)
{
        BM_M_ContainerEmplace(c_container)
}
static void BM_ContiguousContainerInsert0(benchmark::State& state)
{
        BM_M_ContainerInsert0(c_container)
}
static void BM_ContiguousContainerInsert1(benchmark::State& state)
{
        BM_M_ContainerInsert1(c_container)
}
static void BM_ContiguousContainerInsert2(benchmark::State& state)
{
        BM_M_ContainerInsert2(c_container)
}
static void BM_ContiguousContainerEraseEmpty(benchmark::State& state)
{
        BM_M_ContainerEraseEmpty(c_container)
}

////////////////
BENCHMARK(BM_VectorBaseline);
BENCHMARK(BM_VectorEmplaceBack);
BENCHMARK(BM_VectorErase);
BENCHMARK(BM_VectorEmplace);
BENCHMARK(BM_VectorInsert0);
BENCHMARK(BM_VectorInsert1);
BENCHMARK(BM_VectorInsert2);
BENCHMARK(BM_VectorEraseEmpty);

BENCHMARK(BM_ContiguousContainerBaseline);
BENCHMARK(BM_ContiguousContainerEmplaceBack);
BENCHMARK(BM_ContiguousContainerErase);
BENCHMARK(BM_ContiguousContainerEmplace);
BENCHMARK(BM_ContiguousContainerInsert0);
BENCHMARK(BM_ContiguousContainerInsert1);
BENCHMARK(BM_ContiguousContainerInsert2);
BENCHMARK(BM_ContiguousContainerEraseEmpty);

BENCHMARK_MAIN()
