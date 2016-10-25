// Copyright Ildus Nezametdinov 2016.
// Distributed under the Boost Software License, Version 1.0.
//(See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "common.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <chrono>

// This function uses bounded_array in constexpr context
constexpr int sum()
{
        ecs::contiguous_container<literal_storage<int, 16>> arr{};
        arr.emplace_back(1);
        arr.emplace_back(2);
        arr.emplace_back(3);
        arr.pop_back();
        arr.push_back(4);

        int a = 5;
        arr.push_back(a);
        arr.push_back(6);

        // arr.erase(arr.begin(), arr.begin() + 3);
        // arr.erase(arr.begin());
        // arr.erase(arr.begin());
        // arr.emplace(arr.begin(), 16);
        // arr.insert(arr.begin() + 1, 3, 5);
        // int b = 25;
        // arr.insert(arr.begin() + 1, b);
        int s = static_cast<int>(arr.size());
        for(auto& v : arr)
                s += v;

        return s;
}
static_assert(sum() == 18 + 5, "");

// Simple type for behavior check
struct some_type
{
        some_type(int x_) : x{x_}
        {
                std::cout << "constructing some type " << x << std::endl;
        }

        some_type(const some_type& other) : x{other.x}
        {
                std::cout << "copy constructing some type from " << x << std::endl;
        }

        some_type(some_type&& other) : x{other.x}
        {
                std::cout << "move constructing some type from " << x << std::endl;
        }

        ~some_type()
        {
                std::cout << "destroying some type " << x << std::endl;
        }

        some_type& operator=(const some_type& rhs)
        {
                std::cout << "copying some type from " << rhs.x << " to " << x << std::endl;
                x = rhs.x;

                return *this;
        }

        some_type& operator=(some_type&& rhs)
        {
                std::cout << "moving some type from " << rhs.x << " to " << x << std::endl;
                x = rhs.x;

                return *this;
        }

        int x;
};

template <typename Container>
void print_container(Container& arr)
{
        std::cout << "container now has: ";
        for(auto& v : arr)
                std::cout << v.x << ' ';
        std::cout << std::endl;
}

template <typename Container>
void reverse_print_container(Container& arr)
{
        std::cout << "container now has: ";
        for(auto i = arr.rbegin(); i != arr.rend(); ++i)
                std::cout << i->x << ' ';
        std::cout << " (reversed) " << std::endl;
}

template <typename Container>
void test_container(Container& arr)
{
        std::cout << "emplace back 1, 2, 3, 4:" << std::endl;
        arr.emplace_back(1);
        arr.emplace_back(2);
        arr.emplace_back(3);
        arr.emplace_back(4);
        print_container(arr);
        reverse_print_container(arr);

        std::cout << "\nconstruct s0{5} and s1{8} on stack:" << std::endl;
        some_type s0{5}, s1{8};

        std::cout << "\npush_back s0:" << std::endl;
        arr.push_back(s0);
        print_container(arr);

        std::cout << "\npush_back s1 by moving it" << std::endl;
        arr.push_back(std::move(s1));
        print_container(arr);

        std::cout << "\nerase elements 1 and 2:" << std::endl;
        auto next = arr.erase(arr.begin() + 1, arr.begin() + 3);
        print_container(arr);
        std::cout << "next element: " << next->x << std::endl;

        std::cout << "\nerase element 0:" << std::endl;
        next = arr.erase(arr.begin());
        print_container(arr);
        std::cout << "next element: " << next->x << std::endl;

        std::cout << "\nemplace element before second" << std::endl;
        next = arr.emplace(arr.begin() + 1, 15);
        print_container(arr);
        std::cout << "new element: " << next->x << std::endl;

        std::cout << "\nemplace element before end()" << std::endl;
        next = arr.emplace(arr.end(), 25);
        print_container(arr);
        std::cout << "new element: " << next->x << std::endl;

        std::cout << "\nerase 2 last elements:" << std::endl;
        next = arr.erase(arr.end() - 2, arr.end());
        print_container(arr);
        if(next == arr.end())
                std::cout << "next element is arr.end()" << std::endl;

        std::cout << "\npopping the last element" << std::endl;
        arr.pop_back();
        print_container(arr);

        std::cout << "\nconstruct s2{33}, s3{44}, s4{55} and s5{66} on stack:" << std::endl;
        some_type s2{33}, s3{44}, s4{55}, s5{66};

        std::cout << "\ninsert s2 before the first element" << std::endl;
        next = arr.insert(arr.begin(), s2);
        print_container(arr);

        std::cout << "\ninsert s3 before the first element by moving it" << std::endl;
        next = arr.insert(arr.begin(), std::move(s3));
        print_container(arr);
        std::cout << "next element: " << next->x << '\n' << std::endl;

        std::cout << "\ninsert 10 copies of s4 before third element\n";
        next = arr.insert(arr.begin() + 2, 10, s4);
        print_container(arr);
        std::cout << "next element: " << next->x << '\n' << std::endl;

        std::cout << "\ninsert 2 copies of s5 before second element\n";
        next = arr.insert(arr.begin() + 1, 2, s5);
        print_container(arr);
        if(next == arr.end())
                std::cout << "next element is arr.end()" << std::endl;
        else
                std::cout << "next element: " << next->x << '\n' << std::endl;

        std::cout << "\ninsert {101, 102, 103, 104} before fifth element\n";
        next = arr.insert(arr.begin() + 4, {101, 102, 103, 104});
        print_container(arr);
        if(next == arr.end())
                std::cout << "next element is arr.end()" << std::endl;
        else
                std::cout << "next element: " << next->x << '\n' << std::endl;

        std::cout << "\ncreate array a1={201, 202, 203, 204, 205}\n";
        int a1[] = {201, 202, 203, 204, 205};

        std::cout << "\ninsert elements of a1 before seventh element\n";
        next = arr.insert(arr.begin() + 6, std::begin(a1), std::end(a1));
        print_container(arr);
        if(next == arr.end())
                std::cout << "next element is arr.end()" << std::endl;
        else
                std::cout << "next element: " << next->x << '\n' << std::endl;

        std::cout << "\ninsert 7 elements at the end\n";
        next = arr.insert(arr.end(), 7, some_type{777});
        print_container(arr);
        if(next == arr.end())
                std::cout << "next element is arr.end()" << std::endl;
        else
                std::cout << "next element: " << next->x << '\n' << std::endl;

        std::cout << "\ninsert 3 elements at begin + 5 from a1 using input iterators\n";
        next = arr.insert(arr.begin() + 5, make_input_iterator(a1), make_input_iterator(a1 + 3));
        print_container(arr);
        if(next == arr.end())
                std::cout << "next element is arr.end()" << std::endl;
        else
                std::cout << "next element: " << next->x << '\n' << std::endl;


        std::cout << "\ncreate array a2={301, 302, 303, 304}\n";
        int a2[] = {301, 302, 303, 304};

        std::cout << "\ninsert elements of a2 before the last element\n";
        next = arr.insert(arr.end() - 1, make_input_iterator(std::begin(a2)), make_input_iterator(std::end(a2)));
        print_container(arr);
        if(next == arr.end())
                std::cout << "next element is arr.end()" << std::endl;
        else
                std::cout << "next element: " << next->x << '\n' << std::endl;

        std::cout << "\nerase empty interval" << std::endl;
        next = arr.erase(arr.begin(), arr.begin());
        print_container(arr);
        std::cout << "next element: " << next->x << '\n' << std::endl;

        std::cout << "\nassign {1001, 1002, 1003, 1004, 1005}" << std::endl;
        arr.assign({1001, 1002, 1003, 1004, 1005});
        print_container(arr);
        std::cout << '\n';

        std::cout << "\nassign {2001, 2002, 2003, 2004, 2005, 2006, 2007}" << std::endl;
        arr.assign({2001, 2002, 2003, 2004, 2005, 2006, 2007});
        print_container(arr);

        std::cout << "\ncreate array a3={4001, 4002, 4003, 4004}\n";
        int a3[] = {4001, 4002, 4003, 4004};
        std::cout << "\nassign a3 with input iterator" << std::endl;
        arr.assign(make_input_iterator(std::begin(a3)), make_input_iterator(std::end(a3)));
        print_container(arr);

        std::cout << "\ncreate array a4={5001, 5002, 5003, 5004, 5005, 5006, 5007, 5008}\n";
        int a4[] = {5001, 5002, 5003, 5004, 5005, 5006, 5007, 5008};
        std::cout << "\nassign a4 with input iterator" << std::endl;
        arr.assign(make_input_iterator(std::begin(a4)), make_input_iterator(std::end(a4)));
        print_container(arr);

        std::cout << '\n';
}

int main()
{
        std::cout
                << "\n--------------------------------------------------\nTESTING BOUNDED_ARRAY\n";
        {
                ecs::contiguous_container<uninitialized_memory_buffer<some_type, 64>> arr;
                test_container(arr);
        }

        std::cout << "\n--------------------------------------------------\nTESTING STD::VECTOR\n";
        {
                std::vector<some_type> arr;
                arr.reserve(64);
                test_container(arr);
        }
        return 0;
}
