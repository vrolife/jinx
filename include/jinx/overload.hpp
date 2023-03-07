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
