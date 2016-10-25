#ifndef COMMON_H
#define COMMON_H

#include "../source/ecs/contiguous_container.h"

template <typename T, std::size_t N>
struct literal_storage
{
        using value_type = T;

        template <typename... Args>
        constexpr void construct(T* location, Args&&... args)
        {
                *location = T{std::forward<Args>(args)...};
        }

        constexpr void destroy(T*) noexcept
        {
        }

        static constexpr auto reallocate(std::size_t) noexcept
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

template <typename T, std::size_t N>
struct uninitialized_memory_buffer
{
        using value_type = T;

        ~uninitialized_memory_buffer()
        {
                if(!std::is_trivially_destructible<T>::value)
                {
                        for(std::size_t i = 0; i < size_; ++i)
                                (begin() + i)->~T();
                }
        }

        template <typename... Args>
        void construct(T* location, Args&&... args)
        {
                ::new(location) T{std::forward<Args>(args)...};
        }

        void destroy(T* location) noexcept
        {
                if /*constexpr*/ (!std::is_trivially_destructible<T>::value)
                        location->~T();
        }

        static constexpr auto reallocate(std::size_t) noexcept
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

        ~dynamic_uninitialized_memory_buffer()
        {
                if(!std::is_trivially_destructible<T>::value)
                {
                        for(std::size_t i = 0; i < size_; ++i)
                                (begin() + i)->~T();
                }
        }

        template <typename... Args>
        void construct(T* location, Args&&... args)
        {
                ::new(location) T{std::forward<Args>(args)...};
        }

        void destroy(T* location) noexcept
        {
                if /*constexpr*/ (!std::is_trivially_destructible<T>::value)
                        location->~T();
        }

        bool reallocate(std::size_t n)
        {
                if(n > max_size() || n < capacity())
                        return false;

                if(size_ == 0)
                {
                        storage_.reset(::new unsigned char[n * sizeof(T)]);
                        capacity_ = n;

                        return true;
                }

                auto capacity = std::max(size_ + size_, n);
                capacity = (capacity < size_ || capacity > max_size()) ? max_size() : capacity;

                auto ptr = std::make_unique<unsigned char[]>(capacity * sizeof(T));
                auto first = reinterpret_cast<T *>(ptr.get()), last = first;

                try
                {
                        ecs::for_each_iter(begin(), begin() + size(), first, [&](auto i, auto j) {
                                this->construct(j, std::move_if_noexcept(*i)), ++last;
                        });
                }
                catch(...)
                {
                        ecs::for_each_iter(first, last, [&](auto i) { this->destroy(i); });
                }

                ecs::for_each_iter(begin(), begin() + size(), [&](auto i) { this->destroy(i); });
                std::swap(storage_, ptr);
                capacity_ = capacity;

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

template <typename Iterator>
struct input_iterator_adaptor
{
        // types:
        //
        using iterator_type = Iterator;
        using iterator_category = std::input_iterator_tag;

        using difference_type = typename std::iterator_traits<iterator_type>::difference_type;
        using value_type = typename std::iterator_traits<iterator_type>::value_type;

        using reference = typename std::iterator_traits<iterator_type>::reference;
        using pointer = iterator_type;

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
                return base_;
        }

        //
        constexpr input_iterator_adaptor& operator++() noexcept
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

#endif // COMMON_H
