// Copyright Ildus Nezametdinov 2016.
// Distributed under the Boost Software License, Version 1.0.
//(See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef ITERATOR_H
#define ITERATOR_H

#include <iterator>

namespace ecs
{
template <typename Iterator>
struct identity_iterator
{
        // types:
        //
        using iterator_type = Iterator;

        //
        using iterator_category = typename std::iterator_traits<iterator_type>::iterator_category;

        using difference_type = typename std::iterator_traits<iterator_type>::difference_type;
        using value_type = typename std::iterator_traits<iterator_type>::value_type;

        using reference = typename std::iterator_traits<iterator_type>::reference;
        using pointer = typename std::iterator_traits<iterator_type>::pointer;

        //
        constexpr identity_iterator() = default;
        constexpr explicit identity_iterator(iterator_type i) : i_{i}
        {
        }

        template <typename U>
        constexpr identity_iterator(const identity_iterator<U>& u) : i_(u.base())
        {
        }

        template <typename U>
        constexpr identity_iterator& operator=(const identity_iterator<U>& u)
        {
                i_ = u.base();
                return *this;
        }

        //
        constexpr iterator_type base() const
        {
                return i_;
        }

        //
        constexpr reference operator*() const
        {
                return static_cast<reference>(*i_);
        }

        constexpr pointer operator->() const
        {
                return i_;
        }

        //
        constexpr identity_iterator& operator++() noexcept
        {
                return *this;
        }

        constexpr identity_iterator& operator--() noexcept
        {
                return *this;
        }

        constexpr identity_iterator operator++(int)
        {
                return *this;
        }

        constexpr identity_iterator operator--(int)
        {
                return *this;
        }

        //
        constexpr identity_iterator operator+(difference_type) const
        {
                return *this;
        }

        constexpr identity_iterator operator-(difference_type) const
        {
                return *this;
        }

        constexpr identity_iterator& operator+=(difference_type) noexcept
        {
                return *this;
        }

        constexpr identity_iterator& operator-=(difference_type) noexcept
        {
                return *this;
        }

        //
        constexpr reference operator[](difference_type) const
        {
                return *(*this);
        }

private:
        iterator_type i_{};
};

//
template <typename Iterator1, typename Iterator2>
constexpr bool operator==(const identity_iterator<Iterator1>& lhs,
                          const identity_iterator<Iterator2>& rhs)
{
        return lhs.base() == rhs.base();
}

template <typename Iterator1, typename Iterator2>
constexpr bool operator!=(const identity_iterator<Iterator1>& lhs,
                          const identity_iterator<Iterator2>& rhs)
{
        return !(lhs == rhs);
}

//
template <typename Iterator1, typename Iterator2>
constexpr bool operator<(const identity_iterator<Iterator1>& lhs,
                         const identity_iterator<Iterator2>& rhs)
{
        return lhs.base() < rhs.base();
}

template <typename Iterator1, typename Iterator2>
constexpr bool operator>(const identity_iterator<Iterator1>& lhs,
                         const identity_iterator<Iterator2>& rhs)
{
        return rhs < lhs;
}

template <typename Iterator1, typename Iterator2>
constexpr bool operator<=(const identity_iterator<Iterator1>& lhs,
                          const identity_iterator<Iterator2>& rhs)
{
        return !(rhs < lhs);
}

template <typename Iterator1, typename Iterator2>
constexpr bool operator>=(const identity_iterator<Iterator1>& lhs,
                          const identity_iterator<Iterator2>& rhs)
{
        return !(lhs < rhs);
}

template <typename Iterator1, typename Iterator2>
constexpr auto operator-(const identity_iterator<Iterator1>& lhs,
                         const identity_iterator<Iterator2>& rhs)
        -> decltype(rhs.base() - lhs.base())
{
        return rhs.base() - lhs.base();
}

template <typename Iterator>
constexpr identity_iterator<Iterator> operator+(
        typename identity_iterator<Iterator>::difference_type, const identity_iterator<Iterator>& x)
{
        return x;
}

template <typename Iterator>
constexpr identity_iterator<Iterator> make_identity_iterator(Iterator i)
{
        return identity_iterator<Iterator>{i};
}

//
} // namespace ecs

#endif // ITERATOR_H
