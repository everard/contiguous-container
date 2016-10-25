// Copyright Ildus Nezametdinov 2016.
// Distributed under the Boost Software License, Version 1.0.
//(See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "common.h"
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

using ttype = non_trivial;

//
template <typename Container>
void test_container_performance_baseline(Container& arr)
{
        arr.assign({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18});
        opt_clobber();
}

template <typename Container>
void test_container_performance_emplace_back(Container& arr)
{
        test_container_performance_baseline(arr);

        arr.emplace_back(1);
        arr.emplace_back(1);
        arr.emplace_back(1);
        arr.emplace_back(1);
        arr.emplace_back(1);
        arr.emplace_back(1);
        arr.emplace_back(1);
        arr.emplace_back(1);
        arr.emplace_back(1);
        arr.emplace_back(1);
        arr.emplace_back(1);
        arr.emplace_back(1);
        opt_clobber();
}

template <typename Container>
void test_container_performance_erase_one(Container& arr)
{
        test_container_performance_baseline(arr);

        arr.erase(arr.begin());
        opt_clobber();
}

template <typename Container>
void test_container_performance_erase_range(Container& arr)
{
        test_container_performance_baseline(arr);

        arr.erase(arr.begin(), arr.begin() + 4);
        opt_clobber();
}

template <typename Container>
void test_container_performance_erase_empty_range(Container& arr)
{
        test_container_performance_baseline(arr);

        arr.erase(arr.begin(), arr.begin());
        opt_clobber();
}

template <typename Container>
void test_container_performance_emplace(Container& arr)
{
        test_container_performance_baseline(arr);

        arr.emplace(arr.begin() + 1, 15);
        opt_clobber();
}

template <typename Container>
void test_container_performance_insert_initlist(Container& arr)
{
        test_container_performance_baseline(arr);

        arr.insert(arr.begin() + 1, {101, 102, 103, 104, 105});
        opt_clobber();
}

template <typename Container>
void test_container_performance_insert_range(Container& arr)
{
        test_container_performance_baseline(arr);

        // 8 + 5
        int a[] = {201, 202, 203, 204, 205};
        arr.insert(arr.begin() + 1, std::begin(a), std::end(a));
        opt_clobber();
}

template <typename Container>
void test_container_performance_insert_n_at_end(Container& arr)
{
        test_container_performance_baseline(arr);

        arr.insert(arr.end(), 5, 777);
        opt_clobber();
}

template <typename Container>
void test_container_performance_insert_n_at_begin(Container& arr)
{
        test_container_performance_baseline(arr);

        arr.insert(arr.begin(), 5, 777);
        opt_clobber();
}

template <typename Container>
void test_container_performance_insert_range_input_iter(Container& arr)
{
        test_container_performance_baseline(arr);

        int a[] = {301, 302, 303, 304, 305};
        arr.insert(arr.begin() + 1, make_input_iterator(std::begin(a)),
                   make_input_iterator(std::end(a)));
        opt_clobber();
}

template <typename Container>
void test_container_performance_insert_range_input_iter_at_end(Container& arr)
{
        test_container_performance_baseline(arr);

        // 24 + 6
        int a[] = {401, 402, 403, 404, 405};
        arr.insert(arr.end(), make_input_iterator(std::begin(a)), make_input_iterator(std::end(a)));
        opt_clobber();
}

template <typename Container>
void test_container_performance_assign_less(Container& arr)
{
        test_container_performance_baseline(arr);
        arr.assign({1001, 1002, 1003, 1004, 1005});
        opt_clobber();
}

template <typename Container>
void test_container_performance_assign_more(Container& arr)
{
        test_container_performance_baseline(arr);
        arr.assign({2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011,
                    2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022});
        opt_clobber();
}

////////////////////////// Benchmarks
#define BM_M_Container(C, test)         \
        while(state.KeepRunning())      \
        {                               \
                C arr;                  \
                arr.reserve(64);        \
                opt_escape(arr.data()); \
                test(arr);              \
                opt_clobber();          \
        }

// Vector
using c_vector = std::vector<ttype>;
static void BM_VectorBaseline(benchmark::State& state)
{
        BM_M_Container(c_vector, test_container_performance_baseline);
}
static void BM_VectorEmplaceBack(benchmark::State& state)
{
        BM_M_Container(c_vector, test_container_performance_emplace_back);
}
static void BM_VectorEraseOne(benchmark::State& state)
{
        BM_M_Container(c_vector, test_container_performance_erase_one);
}
static void BM_VectorEraseRange(benchmark::State& state)
{
        BM_M_Container(c_vector, test_container_performance_erase_range);
}
static void BM_VectorEraseEmptyRange(benchmark::State& state)
{
        BM_M_Container(c_vector, test_container_performance_erase_empty_range);
}
static void BM_VectorEmplace(benchmark::State& state)
{
        BM_M_Container(c_vector, test_container_performance_emplace);
}
static void BM_VectorInsertInitList(benchmark::State& state)
{
        BM_M_Container(c_vector, test_container_performance_insert_initlist);
}
static void BM_VectorInsertRange(benchmark::State& state)
{
        BM_M_Container(c_vector, test_container_performance_insert_range);
}
static void BM_VectorInsertNAtEnd(benchmark::State& state)
{
        BM_M_Container(c_vector, test_container_performance_insert_n_at_end);
}
static void BM_VectorInsertNAtBegin(benchmark::State& state)
{
        BM_M_Container(c_vector, test_container_performance_insert_n_at_begin);
}
static void BM_VectorInsertRangeInputIter(benchmark::State& state)
{
        BM_M_Container(c_vector, test_container_performance_insert_range_input_iter);
}
static void BM_VectorInsertRangeInputIterAtEnd(benchmark::State& state)
{
        BM_M_Container(c_vector, test_container_performance_insert_range_input_iter_at_end);
}
static void BM_VectorAssignLess(benchmark::State& state)
{
        BM_M_Container(c_vector, test_container_performance_assign_less);
}
static void BM_VectorAssignMore(benchmark::State& state)
{
        BM_M_Container(c_vector, test_container_performance_assign_more);
}

// Contiguous container
using c_container = ecs::contiguous_container<dynamic_uninitialized_memory_buffer<ttype>>;
static void BM_CContBaseline(benchmark::State& state)
{
        BM_M_Container(c_container, test_container_performance_baseline);
}
static void BM_CContEmplaceBack(benchmark::State& state)
{
        BM_M_Container(c_container, test_container_performance_emplace_back);
}
static void BM_CContEraseOne(benchmark::State& state)
{
        BM_M_Container(c_container, test_container_performance_erase_one);
}
static void BM_CContEraseRange(benchmark::State& state)
{
        BM_M_Container(c_container, test_container_performance_erase_range);
}
static void BM_CContEraseEmptyRange(benchmark::State& state)
{
        BM_M_Container(c_container, test_container_performance_erase_empty_range);
}
static void BM_CContEmplace(benchmark::State& state)
{
        BM_M_Container(c_container, test_container_performance_emplace);
}
static void BM_CContInsertInitList(benchmark::State& state)
{
        BM_M_Container(c_container, test_container_performance_insert_initlist);
}
static void BM_CContInsertRange(benchmark::State& state)
{
        BM_M_Container(c_container, test_container_performance_insert_range);
}
static void BM_CContInsertNAtEnd(benchmark::State& state)
{
        BM_M_Container(c_container, test_container_performance_insert_n_at_end);
}
static void BM_CContInsertNAtBegin(benchmark::State& state)
{
        BM_M_Container(c_container, test_container_performance_insert_n_at_begin);
}
static void BM_CContInsertRangeInputIter(benchmark::State& state)
{
        BM_M_Container(c_container, test_container_performance_insert_range_input_iter);
}
static void BM_CContInsertRangeInputIterAtEnd(benchmark::State& state)
{
        BM_M_Container(c_container, test_container_performance_insert_range_input_iter_at_end);
}
static void BM_CContAssignLess(benchmark::State& state)
{
        BM_M_Container(c_container, test_container_performance_assign_less);
}
static void BM_CContAssignMore(benchmark::State& state)
{
        BM_M_Container(c_container, test_container_performance_assign_more);
}

////////////////
BENCHMARK(BM_VectorBaseline);
BENCHMARK(BM_VectorEmplaceBack);
BENCHMARK(BM_VectorEraseOne);
BENCHMARK(BM_VectorEraseRange);
BENCHMARK(BM_VectorEraseEmptyRange);
BENCHMARK(BM_VectorEmplace);
BENCHMARK(BM_VectorInsertInitList);
BENCHMARK(BM_VectorInsertRange);
BENCHMARK(BM_VectorInsertNAtEnd);
BENCHMARK(BM_VectorInsertNAtBegin);
BENCHMARK(BM_VectorInsertRangeInputIter);
BENCHMARK(BM_VectorInsertRangeInputIterAtEnd);
BENCHMARK(BM_VectorAssignLess);
BENCHMARK(BM_VectorAssignMore);

BENCHMARK(BM_CContBaseline);
BENCHMARK(BM_CContEmplaceBack);
BENCHMARK(BM_CContEraseOne);
BENCHMARK(BM_CContEraseRange);
BENCHMARK(BM_CContEraseEmptyRange);
BENCHMARK(BM_CContEmplace);
BENCHMARK(BM_CContInsertInitList);
BENCHMARK(BM_CContInsertRange);
BENCHMARK(BM_CContInsertNAtEnd);
BENCHMARK(BM_CContInsertNAtBegin);
BENCHMARK(BM_CContInsertRangeInputIter);
BENCHMARK(BM_CContInsertRangeInputIterAtEnd);
BENCHMARK(BM_CContAssignLess);
BENCHMARK(BM_CContAssignMore);

BENCHMARK_MAIN()
