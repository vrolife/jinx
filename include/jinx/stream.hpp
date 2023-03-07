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
#ifndef __jinx_stream_hpp__
#define __jinx_stream_hpp__

#include <jinx/buffer.hpp>
#include <jinx/macros.hpp>
#include <jinx/error.hpp>

namespace jinx {
namespace stream {

enum class ErrorStream {
    NoError,
    EndOfStream,
    BrokenStream
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
