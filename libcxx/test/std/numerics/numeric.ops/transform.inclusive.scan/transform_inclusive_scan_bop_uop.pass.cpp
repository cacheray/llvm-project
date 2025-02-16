
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <numeric>
// UNSUPPORTED: c++03, c++11, c++14

// Became constexpr in C++20
// template<class InputIterator, class OutputIterator, class T,
//          class BinaryOperation, class UnaryOperation>
//   OutputIterator transform_inclusive_scan(InputIterator first, InputIterator last,
//                                           OutputIterator result,
//                                           BinaryOperation binary_op,
//                                           UnaryOperation unary_op);


#include <numeric>
#include <algorithm>
#include <array>
#include <cassert>
#include <functional>
#include <iterator>
#include <vector>

#include "test_macros.h"
#include "test_iterators.h"
// FIXME Remove constexpr vector workaround introduced in D90569
#if TEST_STD_VER > 17
#include <span>
#endif

struct add_one {
    template <typename T>
    constexpr auto operator()(T x) const noexcept {
        return static_cast<T>(x + 1);
    }
};

template <class Iter1, class BOp, class UOp, class Iter2>
TEST_CONSTEXPR_CXX20 void
test(Iter1 first, Iter1 last, BOp bop, UOp uop, Iter2 rFirst, Iter2 rLast)
{
    // C++17 doesn't test constexpr so can use a vector.
    // C++20 can use vector in constexpr evaluation, but both libc++ and MSVC
    // don't have the support yet. In these cases use a std::span for the test.
    // FIXME Remove constexpr vector workaround introduced in D90569
    size_t size = std::distance(first, last);
#if TEST_STD_VER < 20 || \
    (defined(__cpp_lib_constexpr_vector) && __cpp_lib_constexpr_vector >= 201907L)

    std::vector<typename std::iterator_traits<Iter1>::value_type> v(size);
#else
    assert((size <= 5) && "Increment the size of the array");
    typename std::iterator_traits<Iter1>::value_type b[5];
    std::span<typename std::iterator_traits<Iter1>::value_type> v{b, size};
#endif

//  Test not in-place
    std::transform_inclusive_scan(first, last, v.begin(), bop, uop);
    assert(std::equal(v.begin(), v.end(), rFirst, rLast));

//  Test in-place
    std::copy(first, last, v.begin());
    std::transform_inclusive_scan(v.begin(), v.end(), v.begin(), bop, uop);
    assert(std::equal(v.begin(), v.end(), rFirst, rLast));
}


template <class Iter>
TEST_CONSTEXPR_CXX20 void
test()
{
          int ia[]     = {  1,  3,   5,   7,    9 };
    const int pResI0[] = {  2,  6,  12,  20,   30 };        // with add_one
    const int mResI0[] = {  2,  8, 48,  384, 3840 };
    const int pResN0[] = { -1, -4,  -9, -16,  -25 };        // with negate
    const int mResN0[] = { -1,  3, -15, 105, -945 };
    const unsigned sa = sizeof(ia) / sizeof(ia[0] );
    static_assert(sa == sizeof(pResI0) / sizeof(pResI0[0]));       // just to be sure
    static_assert(sa == sizeof(mResI0) / sizeof(mResI0[0]));       // just to be sure
    static_assert(sa == sizeof(pResN0) / sizeof(pResN0[0]));       // just to be sure
    static_assert(sa == sizeof(mResN0) / sizeof(mResN0[0]));       // just to be sure

    for (unsigned int i = 0; i < sa; ++i ) {
        test(Iter(ia), Iter(ia + i), std::plus<>(),       add_one{},       pResI0, pResI0 + i);
        test(Iter(ia), Iter(ia + i), std::multiplies<>(), add_one{},       mResI0, mResI0 + i);
        test(Iter(ia), Iter(ia + i), std::plus<>(),       std::negate<>(), pResN0, pResN0 + i);
        test(Iter(ia), Iter(ia + i), std::multiplies<>(), std::negate<>(), mResN0, mResN0 + i);
        }
}

constexpr size_t triangle(size_t n) { return n*(n+1)/2; }

//  Basic sanity
TEST_CONSTEXPR_CXX20 void
basic_tests()
{
    {
    std::array<size_t, 10> v;
    std::fill(v.begin(), v.end(), 3);
    std::transform_inclusive_scan(v.begin(), v.end(), v.begin(), std::plus<>(), add_one{});
    for (size_t i = 0; i < v.size(); ++i)
        assert(v[i] == (i+1) * 4);
    }

    {
    std::array<size_t, 10> v;
    std::iota(v.begin(), v.end(), 0);
    std::transform_inclusive_scan(v.begin(), v.end(), v.begin(), std::plus<>(), add_one{});
    for (size_t i = 0; i < v.size(); ++i)
        assert(v[i] == triangle(i) + i + 1);
    }

    {
    std::array<size_t, 10> v;
    std::iota(v.begin(), v.end(), 1);
    std::transform_inclusive_scan(v.begin(), v.end(), v.begin(), std::plus<>(), add_one{});
    for (size_t i = 0; i < v.size(); ++i)
        assert(v[i] == triangle(i + 1) + i + 1);
    }

    {
    // C++17 doesn't test constexpr so can use a vector.
    // C++20 can use vector in constexpr evaluation, but both libc++ and MSVC
    // don't have the support yet. In these cases use a std::span for the test.
    // FIXME Remove constexpr vector workaround introduced in D90569
#if TEST_STD_VER < 20 || \
    (defined(__cpp_lib_constexpr_vector) && __cpp_lib_constexpr_vector >= 201907L)
    std::vector<size_t> v, res;
    std::transform_inclusive_scan(v.begin(), v.end(), std::back_inserter(res), std::plus<>(), add_one{});
#else
    std::array<size_t, 0> v, res;
    std::transform_inclusive_scan(v.begin(), v.end(), res.begin(), std::plus<>(), add_one{});
#endif
    assert(res.empty());
    }
}

TEST_CONSTEXPR_CXX20 bool
test()
{
    basic_tests();

//  All the iterator categories
    test<input_iterator        <const int*> >();
    test<forward_iterator      <const int*> >();
    test<bidirectional_iterator<const int*> >();
    test<random_access_iterator<const int*> >();
    test<const int*>();
    test<      int*>();

    return true;
}

int main(int, char**)
{
    test();
#if TEST_STD_VER > 17
    static_assert(test());
#endif
    return 0;
}
