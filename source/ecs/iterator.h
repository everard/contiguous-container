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
        using iterator_category = std::random_access_iterator_tag;

        using difference_type = typename std::iterator_traits<iterator_type>::difference_type;
        using value_type = typename std::iterator_traits<iterator_type>::value_type;

        using reference = typename std::iterator_traits<iterator_type>::reference;
        using pointer = iterator_type;

        //
        constexpr identity_iterator() = default;
        constexpr explicit identity_iterator(iterator_type i) : base_{i}
        {
        }

        template <typename U>
        constexpr identity_iterator(const identity_iterator<U>& u) : base_(u.base())
        {
        }

        template <typename U>
        constexpr identity_iterator& operator=(const identity_iterator<U>& u)
        {
                base_ = u.base();
                return *this;
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
        constexpr identity_iterator& operator++() noexcept
        {
                return *this;
        }
        constexpr identity_iterator operator++(int)
        {
                return *this;
        }

        //
        constexpr identity_iterator& operator--() noexcept
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
        constexpr identity_iterator& operator+=(difference_type) noexcept
        {
                return *this;
        }

        //
        constexpr identity_iterator operator-(difference_type) const
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
        iterator_type base_{};
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
constexpr bool operator<(const identity_iterator<Iterator1>&, const identity_iterator<Iterator2>&)
{
        return false;
}

template <typename Iterator1, typename Iterator2>
constexpr bool operator>(const identity_iterator<Iterator1>&, const identity_iterator<Iterator2>&)
{
        return false;
}

template <typename Iterator1, typename Iterator2>
constexpr bool operator<=(const identity_iterator<Iterator1>& lhs,
                          const identity_iterator<Iterator2>& rhs)
{
        return lhs == rhs;
}

template <typename Iterator1, typename Iterator2>
constexpr bool operator>=(const identity_iterator<Iterator1>& lhs,
                          const identity_iterator<Iterator2>& rhs)
{
        return lhs == rhs;
}

template <typename Iterator1, typename Iterator2>
constexpr auto operator-(const identity_iterator<Iterator1>& lhs,
                         const identity_iterator<Iterator2>& rhs)
        -> decltype(lhs.base() - rhs.base())
{
        return 0;
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
