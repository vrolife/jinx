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
#ifndef __jinx_openssl_hpp__
#define __jinx_openssl_hpp__

#include <system_error>

#include <openssl/ssl.h>

#include <jinx/macros.hpp>
#include <jinx/stream.hpp>

namespace jinx {
namespace openssl {

using namespace jinx::stream;

#define JINX_OPENSSL_OBJECT(OpenSSLType, OpenSSLObject) \
class OpenSSLObject \
{ \
    typedef OpenSSLType ObjectType; \
    ObjectType* _obj{nullptr}; \
public: \
 \
   OpenSSLObject() = default; \
    explicit OpenSSLObject(ObjectType* obj) : _obj(obj){ } \
 \
    OpenSSLObject(const OpenSSLObject& other) { \
        _obj = other._obj; \
        up_ref(); \
    } \
 \
    OpenSSLObject(OpenSSLObject&& other)  noexcept { \
        _obj = other.release(); \
    } \
 \
    ~OpenSSLObject() { \
        free(); \
    } \
 \
    OpenSSLObject& operator=(const OpenSSLObject& other) { \
        free(); \
        _obj = other._obj; \
        up_ref(); \
        return *this; \
    } \
 \
    OpenSSLObject& operator=(OpenSSLObject&& other) noexcept { \
        free(); \
        _obj = other.release(); \
        return *this; \
    } \
 \
    void up_ref() { \
        if (_obj != nullptr) { \
            OpenSSLType##_up_ref(_obj); \
        } \
    } \
 \
    void free() { \
        if (_obj != nullptr) { \
            OpenSSLType##_free(_obj); \
            _obj = nullptr; \
        } \
    } \
 \
    void reset(ObjectType* ptr=nullptr) noexcept { \
        free(); \
        _obj = ptr; \
    } \
\
    ObjectType* release() { \
        auto* obj = _obj; \
        _obj = nullptr; \
        return obj; \
    } \
 \
    ObjectType* get() const { \
        return _obj; \
    } \
\
    operator bool() { return _obj != nullptr; } \
    operator ObjectType*() { return _obj; } \
}

JINX_OPENSSL_OBJECT(SSL, OpenSSLConnection);
JINX_OPENSSL_OBJECT(SSL_CTX, OpenSSLContext);
JINX_OPENSSL_OBJECT(X509, OpenSSLCertificate);
JINX_OPENSSL_OBJECT(X509_STORE, OpenSSLCertificateStore);
JINX_OPENSSL_OBJECT(EVP_PKEY, OpenSSLPrivateKey);
JINX_OPENSSL_OBJECT(BIO, OpenSSLBIO);

enum class ErrorCodeOpenSSL {
    NoError = 0,
    HandshakeFailed,
    Established,
    EndOfStream,
    SysCall,
    ProtocolError,
    X509Lookup,
    // ClientHelloCallback,
    WantRead,
    WantWrite,
};

JINX_ERROR_DEFINE(openssl, ErrorCodeOpenSSL);

inline error::Error convert_to_streamtls_error_code(int err)
{
    switch(err) {
        case SSL_ERROR_WANT_READ:
            return make_error(ErrorCodeOpenSSL::WantRead);
            break;
        case SSL_ERROR_WANT_WRITE:
            return make_error(ErrorCodeOpenSSL::WantWrite);
            break;
        case SSL_ERROR_WANT_X509_LOOKUP:
            return make_error(ErrorCodeOpenSSL::X509Lookup);
            break;
// TODO check macro SSL_ERROR_WANT_CLIENT_HELLO_CB
//        case SSL_ERROR_WANT_CLIENT_HELLO_CB:
//            return make_error(ErrorCodeOpenSSL::ClientHelloCallback);
//            break;
        case SSL_ERROR_SSL:
            return make_error(ErrorCodeOpenSSL::ProtocolError);
            break;
        case SSL_ERROR_SYSCALL:
            return make_error(ErrorCodeOpenSSL::SysCall);
            break;
        default:
            break;
    }
    return make_error(static_cast<ErrorCodeOpenSSL>(err));
}

template<typename AsyncIOImpl, typename TLSImpl>
class AsyncTLS : public AsyncFunction<typename TLSImpl::TLSResultType> {
    typedef AsyncFunction<typename TLSImpl::TLSResultType> BaseType;

    typedef AsyncIOImpl asyncio;
    typedef typename asyncio::EventEngineType EventEngineType;
    typedef typename EventEngineType::IONativeHandleType IONativeHandleType;

    typename EventEngineType::EventHandleIO _handle{};

    TLSImpl _tls_impl{};

public:
    template<typename... Args>
    AsyncTLS& operator() (Args&&... args) 
    {
        _tls_impl = TLSImpl{std::forward<Args>(args)...};
        this->async_start(&AsyncTLS::do_tls_io);
        return *this;
    }

protected:
    void async_finalize() noexcept override {
        if (not _handle.empty()) {
            auto* eve = this->template get_event_engine<EventEngineType>();
            eve->remove_io(_handle) >> JINX_IGNORE_RESULT;
            _handle.reset();
        }
        BaseType::async_finalize();
    }

    void print_backtrace(std::ostream& error, int indent) override {
        error << (indent > 0 ? '-' : '|') 
            << "AsyncTLS{ type: " << TLSImpl::TLSTypeName << " }" << std::endl;
        BaseType::print_backtrace(error, indent + 1);
    }

    Async do_tls_io() {
        auto result = _tls_impl.io();
        this->emplace_result((typename TLSImpl::TLSResultType)result);
        if (result._error.category() == category_openssl()) {
            switch (static_cast<ErrorCodeOpenSSL>(result._error.value())) {
                case ErrorCodeOpenSSL::WantRead:
                {
                    auto* eve = this->template get_event_engine<EventEngineType>();
                    if (eve->add_io(
                        typename EventEngineType::IOTypeRead{},
                        _handle,
                        SSL_get_fd(_tls_impl._connection),
                        &AsyncTLS::callback, this).is(Failed_))
                    {
                        return this->async_throw(ErrorEventEngine::FailedToRegisterIOEvent);
                    }
                    return this->async_suspend();
                }
                case ErrorCodeOpenSSL::WantWrite:
                {
                    auto* eve = this->template get_event_engine<EventEngineType>();
                    if (eve->add_io(
                        typename EventEngineType::IOTypeWrite{},
                        _handle,
                        SSL_get_fd(_tls_impl._connection),
                        &AsyncTLS::callback, this).is(Failed_))
                    {
                        return this->async_throw(ErrorEventEngine::FailedToRegisterIOEvent);
                    }
                    return this->async_suspend();
                }
                case ErrorCodeOpenSSL::Established:
                    return this->async_return();
                default:
                    break;
            }
        }
        if (!result._error) {
            return this->async_return();
        }
        return BaseType::async_throw(result._error);
    }

    static void callback(unsigned int flags, const error::Error& error, void* data) {
        auto* self = reinterpret_cast<AsyncTLS*>(data);
        auto result = self->async_resume(error);
        jinx_assert(result.is(Successful_));
    }
};

struct TLSImplOpenSSLAccept
{
    typedef int TLSResultType;
    constexpr static const char* TLSTypeName = "SSL_accept";

    SSL* _connection{};

    TLSImplOpenSSLAccept() = default;
    explicit TLSImplOpenSSLAccept(SSL* connection) : _connection(connection) { }

    IOResult io() const {
        IOResult result{};

        result._int = SSL_accept(_connection);
        if (result._int == 0) {
            result._error = make_error(ErrorCodeOpenSSL::HandshakeFailed);
            return result;
        }

        if (JINX_LIKELY(result._int == 1)) {
            result._error = make_error(ErrorCodeOpenSSL::Established);
            return result;
        }
        result._error = convert_to_streamtls_error_code(
            SSL_get_error(_connection, result._int));
        return result;
    }
};

struct TLSImplOpenSSLConnect
{
    typedef int TLSResultType;
    constexpr static const char* TLSTypeName = "SSL_connect";

    SSL* _connection{};

    TLSImplOpenSSLConnect() = default;
    explicit TLSImplOpenSSLConnect(SSL* connection) : _connection(connection) { }

    IOResult io() const {
        IOResult result{};

        result._int = SSL_connect(_connection);
        if (result._int == 0) {
            result._error = make_error(ErrorCodeOpenSSL::HandshakeFailed);
            return result;
        }
        if (JINX_LIKELY(result._int == 1)) {
            result._error = make_error(ErrorCodeOpenSSL::Established);
            return result;
        }
        result._error = convert_to_streamtls_error_code(
            SSL_get_error(_connection, result._int));
        return result;
    }
};

struct TLSImplOpenSSLShutdown
{
    typedef int TLSResultType;
    constexpr static const char* TLSTypeName = "SSL_shutdown";

    SSL* _connection{};

    bool _wait_close_notify{false};
    char _buf{0};

    TLSImplOpenSSLShutdown() = default;
    explicit TLSImplOpenSSLShutdown(SSL* connection) : _connection(connection) { }

    IOResult io() {
        if (_wait_close_notify) {
            return do_peek();
        }

        IOResult result = do_shutdown();
        if (result._int == 0) {
            _wait_close_notify = true;
            return do_peek();
        }
        return result;
    }

private:
    IOResult do_shutdown() {
        IOResult result{};
        result._int = SSL_shutdown(_connection);
        if (result._int == 1) {
            return result;
        } 
        if (result._int == 0) {
            return do_peek();
        }
        result._error = convert_to_streamtls_error_code(
            SSL_get_error(_connection, result._int));
        return result;
    }

    IOResult do_peek() {
        IOResult result{};
        result._int = SSL_peek(_connection, &_buf, 1);
        if (result._int < 0) {
            result._error = convert_to_streamtls_error_code(
                SSL_get_error(_connection, result._int));
        }
        return result;
    }
};

struct TLSImplOpenSSLRead
{
    typedef int TLSResultType;
    constexpr static const char* TLSTypeName = "SSL_read";

    SSL* _connection{};

    buffer::BufferView* _buffer{};

    TLSImplOpenSSLRead() = default;
    TLSImplOpenSSLRead(SSL* connection, buffer::BufferView* buffer) 
    : _connection(connection), _buffer(buffer) { }

    IOResult io() const {
        IOResult result{};
        auto buf = _buffer->slice_for_producer();
        result._int = SSL_read(_connection, buf.data(), buf.size());
        if (result._int == 0) {
            result._error = make_error(ErrorCodeOpenSSL::EndOfStream);
        } else if (result._int > 0) {
            _buffer->commit(result._int).abort_on(buffer::BufferViewStatus::OutOfRange, "commit out of range");
        } else {
            result._error = convert_to_streamtls_error_code(
                SSL_get_error(_connection, result._int));
        }
        return result;
    }
};

struct TLSImplOpenSSLWrite
{
    typedef int TLSResultType;
    constexpr static const char* TLSTypeName = "SSL_write";

    SSL* _connection{};

    buffer::BufferView* _buffer{};
    
    TLSImplOpenSSLWrite() = default;
    TLSImplOpenSSLWrite(SSL* connection, buffer::BufferView* buffer) 
    : _connection(connection), _buffer(buffer) { }

    IOResult io() const {
        IOResult result{};
        const auto buf = _buffer->slice_for_consumer();
        result._int = SSL_write(_connection, buf.data(), buf.size());
        if (result._int > 0) {
            _buffer->consume(result._size).abort_on(buffer::BufferViewStatus::OutOfRange, "commit out of range");
            // write all
            if (_buffer->size() > 0) {
                return  io();
            }
        } else {
            result._error = convert_to_streamtls_error_code(
                SSL_get_error(_connection, result._int));
        }
        return result;
    }
};

template<typename AsyncIOImpl>
class StreamOpenSSL : public Stream
{
private:
    OpenSSLConnection _connection{nullptr};

    AsyncTLS<AsyncIOImpl, TLSImplOpenSSLAccept> _accept{};
    AsyncTLS<AsyncIOImpl, TLSImplOpenSSLConnect> _connect{};
    AsyncTLS<AsyncIOImpl, TLSImplOpenSSLShutdown> _shutdown{};

    AsyncTLS<AsyncIOImpl, TLSImplOpenSSLRead> _recv{};
    AsyncTLS<AsyncIOImpl, TLSImplOpenSSLWrite> _send{};

public:
    StreamOpenSSL() = default;
    ~StreamOpenSSL() override = default;
    JINX_NO_COPY_NO_MOVE(StreamOpenSSL);

    void initialize(OpenSSLConnection& connection) 
    {
        _connection = connection;
    }

    Awaitable& shutdown() override {
        return _shutdown(_connection.get());
    }

    Awaitable& accept() {
        return _accept(_connection.get());
    }

    Awaitable& connect() {
        return _connect(_connection.get());
    }

    void reset() noexcept override
    {
        _connection.reset();
    }

    Awaitable& read(buffer::BufferView* buffer) override {
        return _recv(_connection.get(), buffer);
    }

    Awaitable& write(buffer::BufferView* buffer) override {
        return _send(_connection.get(), buffer);
    }
};
    
} // namespace openssl
} // namespace jinx

#endif
