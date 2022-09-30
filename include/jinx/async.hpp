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
#ifndef __jinx_async_hpp__
#define __jinx_async_hpp__

#include <jinx/async/awaitable.hpp>
#include <jinx/async/task.hpp>
#include <jinx/async/pausable.hpp>
#include <jinx/async/awaitable.ipp>

namespace jinx {

template<typename EventEngine>
struct AsyncEngine {
    template<typename T>
    using Wait = jinx::Wait<T, EventEngine>;

    typedef AsyncSleep<EventEngine> Sleep;
};

} // namespace jinx

#endif
