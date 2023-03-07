/*
MIT License

Copyright (c) 2023 pom@vro.life

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef __jinx_meta_hpp__
#define __jinx_meta_hpp__

#include <cstddef>
#include <type_traits>

namespace jinx {
namespace meta {
namespace detail {

template<typename T1, int Index, typename T2, typename... Types>
struct __index_of
{
    constexpr static int value = std::is_same<T1, T2>::value ? Index : __index_of<T1, Index + 1, Types...>::value;
};

template<typename T1, int Index, typename T2>
struct __index_of<T1, Index, T2>
{
    constexpr static int value = std::is_same<T1, T2>::value ? Index : -1;
};

template<typename T, typename... Types>
using index_of = __index_of<T, 0, Types...>;

template<typename T, typename... Types>
struct union_size
{
    constexpr static size_t size = (sizeof(T) > union_size<Types...>::size) ? sizeof(T) : union_size<Types...>::size;
};

template<typename T>
struct union_size<T>
{
    constexpr static size_t size = sizeof(T);
};

} // namespace detail

template<typename T, typename... Types>
using index_of = detail::index_of<T, Types...>;

template<typename... Types>
using union_size = detail::union_size<Types...>;

} // namespace meta
} // namespace jinx

#endif
