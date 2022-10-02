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
#ifndef __jinx_streamsocket_hpp__
#define __jinx_streamsocket_hpp__

#include <sys/socket.h>

#include <jinx/buffer.hpp>
#include <jinx/macros.hpp>
#include <jinx/stream.hpp>

namespace jinx {
namespace stream {
namespace detail {

template<typename Send>
class StreamSend : public Send
{
    typedef typename Send::EventEngineType EventEngineType;

    buffer::BufferView* _view{nullptr};

public:
    StreamSend& operator()(
        typename EventEngineType::IOHandleNativeType handle, 
        buffer::BufferView* view) 
    {
#if __linux__
        const int SendFlags = MSG_NOSIGNAL;
#else
        const int SendFlags = 0;
#endif
        Send::operator()(handle, view->slice_for_consumer(), SendFlags);
        _view = view;
        return *this;
    }

protected:
    void async_finalize() noexcept override {
        _view = nullptr;
        Send::async_finalize();
    }

    Async async_poll() override {
        auto state = Send::async_poll();
        if (state == ControlState::Ready) {
            switch(_view->consume(Send::get_result()).unwrap()) {
                case buffer::BufferViewStatus::Completed:
                    if (_view->size() > 0) {
#if __linux__
                        const int SendFlags = MSG_NOSIGNAL;
#else
                        const int SendFlags = 0;
#endif
                        Send::operator()(this->native_handle(), _view->slice_for_consumer(), SendFlags);
                        return this->async_poll();
                    }
                    break;
                case buffer::BufferViewStatus::OutOfRange:
                    error::fatal("stream buffer out of range");
            }
        }
        return state;
    }
};

template<typename Recv>
class StreamRecv : public Recv
{
    typedef typename Recv::EventEngineType EventEngineType;

    buffer::BufferView* _view{};

public:
    StreamRecv& operator()(
        typename EventEngineType::IOHandleNativeType handle, 
        buffer::BufferView* view) 
    {
        Recv::operator()(handle, view->slice_for_producer(), 0);
        _view = view;
        return *this;
    }

protected:
    void async_finalize() noexcept override {
        _view = nullptr;
        Recv::async_finalize();
    }

    Async async_poll() override {
        auto state = Recv::async_poll();
        if (state == ControlState::Ready) {
            auto size = Recv::get_result();
            if (size == 0) {
                return this->async_throw(ErrorStream::EndOfStream);
            }

            switch(_view->commit(size).unwrap()) {
                case buffer::BufferViewStatus::Completed:
                    break;
                case buffer::BufferViewStatus::OutOfRange:
                    error::fatal("stream buffer out of range");
            }
        }
        return state;
    }
};

template<typename AsyncIOImpl>
class StreamSocket : public Stream
{
public:
    typedef AsyncIOImpl asyncio;
    typedef typename asyncio::EventEngineType EventEngineType;
    typedef typename asyncio::IOHandleType IOHandleType;
    typedef typename asyncio::IOHandleNativeType IOHandleNativeType;

private:
    IOHandleType _io_handle{-1};
    char _buf{0};

    StreamSend<typename asyncio::Send> _send{};
    StreamRecv<typename asyncio::Recv> _recv{};

    typename asyncio::Recv _socket_recv{};

public:
    StreamSocket() = default;
    ~StreamSocket() override = default;
    explicit StreamSocket(IOHandleType&& io_handle) : _io_handle(std::move(io_handle)) { }
    explicit StreamSocket(IOHandleNativeType native_handle) : _io_handle(native_handle) { }
    JINX_NO_COPY_NO_MOVE(StreamSocket);

    void initialize(IOHandleType&& io_handle) 
    {
        _io_handle = std::move(io_handle);
#ifdef __APPLE__
        int b = 1;
        ::setsockopt(_sock.native_handle(), SOL_SOCKET, SO_NOSIGPIPE, &b, sizeof(b));
#endif
    }

    IOHandleType& io_handle() {
        return _io_handle;
    }

    Awaitable& shutdown() override {
        EventEngineType::shutdown(_io_handle.native_handle(), SHUT_WR);
        return _socket_recv(_io_handle.native_handle(), SliceMutable{&_buf, 1}, MSG_PEEK);
    }

    void reset() noexcept override {
        _io_handle.reset();
    }

    Awaitable& read(buffer::BufferView* view) override {
        return _recv(_io_handle.native_handle(), view);
    }

    Awaitable& write(buffer::BufferView* view) override {
        return _send(_io_handle.native_handle(), view);
    }
};

} // namespace detail

template<typename AsyncIOImpl>
using StreamSocket = detail::StreamSocket<AsyncIOImpl>;

} // namespace stream
} // namespace jinx

#endif
