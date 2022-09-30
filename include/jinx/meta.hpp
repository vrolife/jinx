/*
Copyright (C) 2022  pom@vro.life

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
