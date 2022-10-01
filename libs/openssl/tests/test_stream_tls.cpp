#include <openssl/bio.h>

#include <jinx/async.hpp>
#include <jinx/openssl/openssl.hpp>
#include <jinx/posix.hpp>
#include <jinx/libevent.hpp>

#include "tls_server.crt.h"
#include "tls_server.pem.h"

using namespace jinx;
using namespace jinx::stream;
using namespace jinx::buffer;
using namespace jinx::openssl;

typedef AsyncImplement<libevent::EventEngineLibevent> async;
typedef posix::AsyncIOPosix<libevent::EventEngineLibevent> asyncio;

class AsyncEcho : public AsyncRoutine {
    Stream* _stream{nullptr};
    char _buffer[0x1000]{};
    BufferView _view{};

public:
    AsyncEcho& operator ()(Stream* stream) {
        _stream = stream;
        _view = {_buffer, 100, 0, 0};
        async_start(&AsyncEcho::read_more);
        return *this;
    }

    Async read_message() {
        if (_view.size() != 5) {
            return read_more();
        }

        std::string str{_view.begin(), _view.end()};
        jinx_assert(str == "hello");
        return write_message();
    }

    Async read_more() {
        return *this / _stream->read(&_view) / &AsyncEcho::read_message;
    }

    Async write_message() {
        return *this / _stream->write(&_view) / &AsyncEcho::shutdown;
    }

    Async shutdown() {
        return *this / _stream->shutdown() / &AsyncEcho::finish;
    }

    Async finish() { return this-> async_return(); }
};

class HandshakeTLS : public AsyncRoutine {
    StreamOpenSSL<asyncio> _stream{};
    posix::Socket _sock{-1};
    OpenSSLConnection _ssl{nullptr};
    AsyncEcho _echo{};

public:
    HandshakeTLS& operator ()(OpenSSLContext& ctx, posix::Socket&& sock) {
        _sock = std::move(sock);
        _sock.set_non_blocking(true);

        _ssl = OpenSSLConnection{SSL_new(ctx)};
        SSL_set_fd(_ssl, _sock.native_handle());

        _stream.initialize(_ssl);

        async_start(&HandshakeTLS::handshake);
        return *this;
    }

protected:
    void print_backtrace(std::ostream& output_stream, int indent) override {
        output_stream << (indent > 0 ? '-' : '|') 
            << "HandshakeTLS{ stream: " << &_stream << " }" << std::endl;
        AsyncRoutine::print_backtrace(output_stream, indent + 1);
    }

    Async handle_error(const error::Error& error) override {
        abort();
    }

    Async handshake() {
        return *this / _stream.accept() / &HandshakeTLS::run;
    }

    Async run() {
        return async_await(_echo(&_stream), &HandshakeTLS::finish);
    }

    Async finish() {
        return this->async_return();
    }
};

class AsyncClient : public AsyncRoutine {
    posix::Socket _sock{-1};
    asyncio::Connect _connect{};
    StreamOpenSSL<asyncio> _stream{};
    OpenSSLConnection _ssl{nullptr};

    char _buffer[100];
    BufferView _view{};

    char _read_buffer[100];
    BufferView _read_view{};

public:
    AsyncClient& operator ()(posix::Socket&& sock) {
        _sock = std::move(sock);
        _view = {_buffer, 100, 0, 0};
        async_start(&AsyncClient::_yield);
        return *this;
    }

protected:
    Async handle_error(const error::Error& error) override {
        auto state = AsyncRoutine::handle_error(error);
        if (state != ControlState::Raise) {
            return state;
        }

        std::cerr << error.message() << std::endl;
        return state;
    }

    Async _yield() {
        return this->async_yield(&AsyncClient::handshake);
    }

    static int verify_callback(int preverify_ok, X509_STORE_CTX *ctx)
    {
        int depth = X509_STORE_CTX_get_error_depth(ctx);
        int err = X509_STORE_CTX_get_error(ctx);
        jinx_assert(preverify_ok);
        return preverify_ok;
    }

    Async handshake() {
        _sock.set_non_blocking(true);
        OpenSSLContext ctx{SSL_CTX_new(TLS_client_method())};
        SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, verify_callback);
        SSL_CTX_set_verify_depth(ctx, 4);
        SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);

        OpenSSLBIO bio(BIO_new_mem_buf(TLS_SERVER_CRT, TLS_SERVER_CRT_LEN));
        OpenSSLCertificate cert(PEM_read_bio_X509(bio, nullptr, nullptr, nullptr));
        OpenSSLCertificateStore store{X509_STORE_new()};
        X509_STORE_add_cert(store, cert.get());
        store.up_ref(); // ???
        SSL_CTX_set_cert_store(ctx, store);
        // SSL_CTX_load_verify_locations(ctx, "/path/to/tls_server.crt", nullptr);

        _ssl = OpenSSLConnection{SSL_new(ctx)};
        SSL_set_fd(_ssl, _sock.native_handle());
        SSL_set_tlsext_host_name(_ssl, "jinx.local.lan");
        _stream.initialize(_ssl);
        return *this / _stream.connect() / &AsyncClient::write_message;
    }

    Async write_message() {
        auto buf = _view.slice_for_producer();
        jinx_assert(buf.size() >= 5);
        memcpy(buf.begin(), "hello", 5);
        jinx_assert(_view.commit(5).unwrap() == BufferViewStatus::Completed);

        _read_view = {_read_buffer, 100, 0, 0};
        memset(_read_buffer, 0, 100);
        return *this / _stream.write(&_view) / &AsyncClient::read_more;
    }

    Async read_message() {
        auto buf = _read_view.slice_for_consumer();
        if (buf.size() != 5) {
            return read_more();
        }

        std::string str{buf.begin(), buf.end()};
        jinx_assert(str == "hello");
        return async_throw(posix::ErrorPosix::IoError);
    }

    Async read_more() {
        return this->async_await(_stream.read(&_read_view), &AsyncClient::read_message);
    }
};

void setup_server_context(OpenSSLContext& ctx)
{
    OpenSSLBIO bio{BIO_new_mem_buf(TLS_SERVER_CRT, TLS_SERVER_CRT_LEN)};
    OpenSSLCertificate cert{PEM_read_bio_X509(bio, nullptr, nullptr, nullptr)};
    jinx_assert(!!cert);
    SSL_CTX_use_certificate(ctx, cert);

    bio = OpenSSLBIO{BIO_new_mem_buf(TLS_SERVER_PEM, TLS_SERVER_PEM_LEN)};

    OpenSSLPrivateKey key{PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr)};
    jinx_assert(!!key);
    SSL_CTX_use_PrivateKey(ctx, key);
}

error::Error error_handler(Loop& loop, TaskPtr& task, const error::Error& error) {
    if (error != make_error(posix::ErrorPosix::IoError)) {
        abort();
    }
    loop.exit();
    return error;
}

int main(int argc, const char **argv)
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);

    loop.set_error_handler(error_handler);

    int fds[2]{-1, -1};
    jinx_assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);

    posix::Socket server{fds[0]};
    server.set_non_blocking(true);

    posix::Socket client{fds[1]};
    client.set_non_blocking(true);

    const auto* method = TLS_server_method();
    OpenSSLContext ctx{SSL_CTX_new(method)};
    setup_server_context(ctx);

    loop.task_new<AsyncClient>(std::move(client));
    loop.task_new<HandshakeTLS>(ctx, std::move(server));
    loop.run();
    return 0;
}
