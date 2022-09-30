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
#include <cstdlib>

#include <jinx/error.hpp>

#include <system_error>

namespace jinx {
namespace error {

[[noreturn]]
void error(const char* message)
{
#ifdef JINX_ERROR_THROW
    throw JinxErrorAbort();
#else
    ::abort();
#endif
}

[[noreturn]]
void fatal(const char* message)
{
    ::abort();
}

JINX_ERROR_IMPLEMENT(uninitialized, {
    return "Uninitialized error code";
});

} // namespace error
} // namespace jinx