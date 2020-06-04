///////////////////////////////////////////////////////////////////////////////
//
// Copyright(c) 2015 Microsoft Corporation.All rights reserved.
//
// This code is licensed under the MIT License(MIT).
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this softwareand associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions :
//
// The above copyright noticeand this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef UHOBGOBLIN_COMMON_GSL_POINTERS_HPP
#define UHOBGOBLIN_COMMON_GSL_POINTERS_HPP

#include <algorithm>    // for forward
#include <iosfwd>       // for ptrdiff_t, nullptr_t, ostream, size_t
#include <memory>       // for shared_ptr, unique_ptr
#include <system_error> // for hash
#include <type_traits>  // for enable_if_t, is_convertible, is_assignable

#include <Hobgoblin/Private/Pmacro_define.hpp>

HOBGOBLIN_NAMESPACE_START

//
// GSL.owner: ownership pointers
//
using std::unique_ptr;
using std::shared_ptr;

//
// owner
//
// owner<T> is designed as a bridge for code that must deal directly with owning pointers for some reason
//
// T must be a pointer type
// - disallow construction from any type other than pointer type
//
template <class T, class = std::enable_if_t<std::is_pointer<T>::value>>
using owner = T;

namespace detail {

extern void ThrowBecauseNotNullNotSatisfied();

template <class T>
void RequireNotNull(T pointer) {
    if (pointer == nullptr) {
        ThrowBecauseNotNullNotSatisfied();
    }
}

} // namespace detail

//
// not_null
//
// Restricts a pointer or smart pointer to only hold non-null values.
//
// Has zero size overhead over T.
//
// If T is a pointer (i.e. T == U*) then
// - allow construction from U*
// - disallow construction from nullptr_t
// - disallow default construction
// - ensure construction from null U* fails
// - allow implicit conversion to U*
//
template <class T>
class not_null
{
public:
    static_assert(std::is_assignable<T&, std::nullptr_t>::value, "T cannot be assigned nullptr.");

    template <typename U, typename = std::enable_if_t<std::is_convertible<U, T>::value>>
    constexpr not_null(U&& u) : ptr_(std::forward<U>(u))
    {
        ::jbatnozic::hobgoblin::detail::RequireNotNull(ptr_);
    }

    template <typename = std::enable_if_t<!std::is_same<std::nullptr_t, T>::value>>
    constexpr not_null(T u) : ptr_(u)
    {
        ::jbatnozic::hobgoblin::detail::RequireNotNull(ptr_);
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U, T>::value>>
    constexpr not_null(const not_null<U>& other) : not_null(other.get())
    {
    }

    not_null(const not_null& other) = default;
    not_null& operator=(const not_null& other) = default;

    constexpr T get() const
    {
        ::jbatnozic::hobgoblin::detail::RequireNotNull(ptr_);
        return ptr_;
    }

    constexpr operator T() const { return get(); }
    constexpr T operator->() const { return get(); }
    constexpr decltype(auto) operator*() const { return *get(); }

    // prevents compilation when someone attempts to assign a null pointer constant
    not_null(std::nullptr_t) = delete;
    not_null& operator=(std::nullptr_t) = delete;

    // unwanted operators...pointers only point to single objects!
    not_null& operator++() = delete;
    not_null& operator--() = delete;
    not_null operator++(int) = delete;
    not_null operator--(int) = delete;
    not_null& operator+=(std::ptrdiff_t) = delete;
    not_null& operator-=(std::ptrdiff_t) = delete;
    void operator[](std::ptrdiff_t) const = delete;

private:
    T ptr_;
};

template <class T>
auto make_not_null(T&& t) {
    return not_null<std::remove_cv_t<std::remove_reference_t<T>>>{std::forward<T>(t)};
}

template <class T>
std::ostream& operator<<(std::ostream& os, const not_null<T>& val)
{
    os << val.get();
    return os;
}

template <class T, class U>
auto operator==(const not_null<T>& lhs, const not_null<U>& rhs) -> decltype(lhs.get() == rhs.get())
{
    return lhs.get() == rhs.get();
}

template <class T, class U>
auto operator!=(const not_null<T>& lhs, const not_null<U>& rhs) -> decltype(lhs.get() != rhs.get())
{
    return lhs.get() != rhs.get();
}

template <class T, class U>
auto operator<(const not_null<T>& lhs, const not_null<U>& rhs) -> decltype(lhs.get() < rhs.get())
{
    return lhs.get() < rhs.get();
}

template <class T, class U>
auto operator<=(const not_null<T>& lhs, const not_null<U>& rhs) -> decltype(lhs.get() <= rhs.get())
{
    return lhs.get() <= rhs.get();
}

template <class T, class U>
auto operator>(const not_null<T>& lhs, const not_null<U>& rhs) -> decltype(lhs.get() > rhs.get())
{
    return lhs.get() > rhs.get();
}

template <class T, class U>
auto operator>=(const not_null<T>& lhs, const not_null<U>& rhs) -> decltype(lhs.get() >= rhs.get())
{
    return lhs.get() >= rhs.get();
}

// more unwanted operators
template <class T, class U>
std::ptrdiff_t operator-(const not_null<T>&, const not_null<U>&) = delete;
template <class T>
not_null<T> operator-(const not_null<T>&, std::ptrdiff_t) = delete;
template <class T>
not_null<T> operator+(const not_null<T>&, std::ptrdiff_t) = delete;
template <class T>
not_null<T> operator+(std::ptrdiff_t, const not_null<T>&) = delete;

HOBGOBLIN_NAMESPACE_END

namespace std
{
template <class T>
struct hash<::jbatnozic::hobgoblin::not_null<T>>
{
    std::size_t operator()(const ::jbatnozic::hobgoblin::not_null<T>& value) const {
        return hash<T>{}(value);
    }
};

} // namespace std

HOBGOBLIN_NAMESPACE_START

//
// strict_not_null
//
// Restricts a pointer or smart pointer to only hold non-null values,
//
// - provides a strict (i.e. explicit constructor from T) wrapper of not_null
// - to be used for new code that wishes the design to be cleaner and make not_null
//   checks intentional, or in old code that would like to make the transition.
//
//   To make the transition from not_null, incrementally replace not_null
//   by strict_not_null and fix compilation errors
//
//   Expect to
//   - remove all unneeded conversions from raw pointer to not_null and back
//   - make API clear by specifying not_null in parameters where needed
//   - remove unnecessary asserts
//
template <class T>
class strict_not_null : public not_null<T>
{
public:

    template <typename U, typename = std::enable_if_t<std::is_convertible<U, T>::value>>
    constexpr explicit strict_not_null(U&& u) :
        not_null<T>(std::forward<U>(u))
    {}

    template <typename = std::enable_if_t<!std::is_same<std::nullptr_t, T>::value>>
    constexpr explicit strict_not_null(T u) :
        not_null<T>(u)
    {}

    template <typename U, typename = std::enable_if_t<std::is_convertible<U, T>::value>>
    constexpr strict_not_null(const not_null<U>& other) :
        not_null<T>(other)
    {}

    template <typename U, typename = std::enable_if_t<std::is_convertible<U, T>::value>>
    constexpr strict_not_null(const strict_not_null<U>& other) :
        not_null<T>(other)
    {}

    strict_not_null(strict_not_null&& other) = default;
    strict_not_null(const strict_not_null& other) = default;
    strict_not_null& operator=(const strict_not_null& other) = default;
    strict_not_null& operator=(const not_null<T>& other)
    {
        not_null<T>::operator=(other);
        return *this;
    }

    // prevents compilation when someone attempts to assign a null pointer constant
    strict_not_null(std::nullptr_t) = delete;
    strict_not_null& operator=(std::nullptr_t) = delete;

    // unwanted operators...pointers only point to single objects!
    strict_not_null& operator++() = delete;
    strict_not_null& operator--() = delete;
    strict_not_null operator++(int) = delete;
    strict_not_null operator--(int) = delete;
    strict_not_null& operator+=(std::ptrdiff_t) = delete;
    strict_not_null& operator-=(std::ptrdiff_t) = delete;
    void operator[](std::ptrdiff_t) const = delete;
};

// more unwanted operators
template <class T, class U>
std::ptrdiff_t operator-(const strict_not_null<T>&, const strict_not_null<U>&) = delete;
template <class T>
strict_not_null<T> operator-(const strict_not_null<T>&, std::ptrdiff_t) = delete;
template <class T>
strict_not_null<T> operator+(const strict_not_null<T>&, std::ptrdiff_t) = delete;
template <class T>
strict_not_null<T> operator+(std::ptrdiff_t, const strict_not_null<T>&) = delete;

template <class T>
auto make_strict_not_null(T&& t) {
    return strict_not_null<std::remove_cv_t<std::remove_reference_t<T>>>{std::forward<T>(t)};
}

#if ( defined(__cpp_deduction_guides) && (__cpp_deduction_guides >= 201611L) )

// deduction guides to prevent the ctad-maybe-unsupported warning
template <class T> not_null(T)->not_null<T>;
template <class T> strict_not_null(T)->strict_not_null<T>;

#endif // ( defined(__cpp_deduction_guides) && (__cpp_deduction_guides >= 201611L) )

HOBGOBLIN_NAMESPACE_END

namespace std
{
template <class T>
struct hash<::jbatnozic::hobgoblin::strict_not_null<T>>
{
    std::size_t operator()(const ::jbatnozic::hobgoblin::strict_not_null<T>& value) const {
        return hash<T>{}(value);
    }
};

} // namespace std

#include <Hobgoblin/Private/Pmacro_undef.hpp>
#include <Hobgoblin/Private/Short_namespace.hpp>

#endif // !UHOBGOBLIN_COMMON_GSL_POINTERS_HPP