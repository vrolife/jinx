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
#ifndef __overload_hpp__
#define __overload_hpp__

#include <type_traits>
#include <utility>

namespace jinx {
namespace detail {

template<typename T, typename... Ts>
struct __overload_impl;

template<typename T>
struct __overload_impl<T> : T {
   template<typename _T>
   explicit __overload_impl(_T&& fun) : T{std::forward<_T>(fun)} { }
};

template<typename T, typename... Ts>
struct __overload_impl : T,  __overload_impl<Ts...>
{
    template<typename _T, typename... _Ts>
    explicit __overload_impl(_T&& fun, _Ts&&... args)
    : _T{std::forward<_T>(fun)}, 
      __overload_impl<Ts...>{std::forward<_Ts>(args)...}
    { }

    using T::operator();
    using __overload_impl<Ts...>::operator();
};

template<typename... Ts>
__overload_impl<Ts...> overload(Ts&&... args) {
   return __overload_impl<Ts...>{std::forward<Ts>(args)...};
}

} // namespace detail

using ::jinx::detail::overload;

} // namespace jinx

#endif
