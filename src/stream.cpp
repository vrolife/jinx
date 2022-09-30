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
#include <jinx/stream.hpp>

namespace jinx {
namespace stream {

JINX_ERROR_IMPLEMENT(stream, {
    switch(code.as<ErrorStream>()) {
        case ErrorStream::NoError:
            return "no error";
        case ErrorStream::EndOfStream:
            return "end of stream";
    }
    return "unknown error value";
});

} // namespace stream
} // namespace jinx
