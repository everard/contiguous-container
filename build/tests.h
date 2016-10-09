// Copyright Ildus Nezametdinov 2016.
// Distributed under the Boost Software License, Version 1.0.
//(See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "../source/experimental/contiguous_container.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <chrono>

template <typename T, std::size_t N>
struct uninitialized_memory_buffer
{
        using value_type = T;
        using no_exceptions = std::false_type;

        ~uninitialized_memory_buffer()
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

        bool reserve(std::size_t n)
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
                                        construct_at(j, std::move_if_noexcept(*i));
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

template <typename T, std::size_t N>
struct literal_storage
{
        using value_type = T;
        using no_exceptions = std::false_type;

        template <typename... Args>
        constexpr void construct_at(T* location, Args&&... args)
        {
                *location = T{std::forward<Args>(args)...};
        }

        constexpr void destroy_at(T*) noexcept
        {
        }

        static constexpr auto reserve(std::size_t = 1) noexcept
        {
                return false;
        }

        constexpr T* begin() noexcept
        {
                return storage_;
        }

        constexpr const T* begin() const noexcept
        {
                return storage_;
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

protected:
        T storage_[N]{};
        std::size_t size_{};
};

// This function uses bounded_array in constexpr context
constexpr int sum()
{
        std::experimental::contiguous_container<literal_storage<int, 16>> arr{};
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
        std::cout << '\n';
}

int main()
{
        std::cout
                << "\n--------------------------------------------------\nTESTING BOUNDED_ARRAY\n";
        {
                std::experimental::contiguous_container<uninitialized_memory_buffer<some_type, 32>>
                        arr;
                test_container(arr);
        }

        std::cout << "\n--------------------------------------------------\nTESTING STD::VECTOR\n";
        {
                std::vector<some_type> arr;
                arr.reserve(32);
                test_container(arr);
        }
        return 0;
}
