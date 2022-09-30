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
#ifndef __jinx_stream_hpp__
#define __jinx_stream_hpp__

#include <jinx/buffer.hpp>
#include <jinx/macros.hpp>
#include <jinx/error.hpp>

namespace jinx {
namespace stream {

enum class ErrorStream {
    NoError,
    EndOfStream
};

JINX_ERROR_DEFINE(stream, ErrorStream);

class Stream
{    
public:
    Stream() = default;
    JINX_NO_COPY_NO_MOVE(Stream);
    virtual ~Stream() = default;

    virtual Awaitable& shutdown() = 0;
    virtual void reset() noexcept = 0;
    virtual Awaitable& write(buffer::BufferView* view) = 0;
    virtual Awaitable& read(buffer::BufferView* view) = 0;
};

} // namespace stream
} // namespace jinx

#endif
