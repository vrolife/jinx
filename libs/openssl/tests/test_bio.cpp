#include <jinx/libevent.hpp>
#include <jinx/async.hpp>
#include <jinx/openssl/openssl.hpp>
#include <jinx/openssl/bio.hpp>

using namespace jinx;
using namespace jinx::openssl;

class AsyncServer : public AsyncRoutine {
    typedef AsyncRoutine BaseType;

    buffer::BufferView _view{};
    std::array<char, 128> _buffer{};

    StreamOpenSSL<AsyncIOBIO>* _ssl{};

public:
    AsyncServer& operator ()(StreamOpenSSL<AsyncIOBIO>* ssl) {
        _ssl = ssl;
        _view = {_buffer.data(), _buffer.size(), 0, 0};
        async_start(&AsyncServer::handshake);
        return *this;
    }

    Async handshake() {
        return *this / _ssl->accept() / &AsyncServer::read;
    }

    Async read() {
        return *this / _ssl->read(&_view) / &AsyncServer::write;
    }

    Async write() {
        return *this / _ssl->write(&_view) / &AsyncServer::async_return;
    }
};

class AsyncClient : public AsyncRoutine {
    typedef AsyncRoutine BaseType;

    buffer::BufferView _view{};
    std::array<char, 128> _buffer{};
    
    StreamOpenSSL<AsyncIOBIO>* _ssl{};

public:
    AsyncClient& operator ()(StreamOpenSSL<AsyncIOBIO>* ssl) {
        _ssl = ssl;
        _view = {_buffer.data(), _buffer.size(), 0, 0};
        async_start(&AsyncClient::yield);
        return *this;
    }

    Async yield() {
        return this->async_yield(&AsyncClient::connect);
    }

    Async connect() {
        _view = {const_cast<char*>("hello"), 5, 0, 5};
        return *this / _ssl->connect() / &AsyncClient::write;
    }

    Async write() {
        return *this / _ssl->write(&_view) / &AsyncClient::read;
    }

    Async read() {
        _view = {_buffer.data(), _buffer.size(), 0, 0};
        memset(_view.begin(), 0, _view.capacity());
        return *this / _ssl->read(&_view) / &AsyncClient::check;
    }

    Async check() {
        jinx_assert(_view.size() == 5);
        jinx_assert(memcmp("hello", _view.begin(), 5) == 0);
        return async_throw(posix::ErrorPosix::InvalidArgument);
    }
};

class AsyncTransport : public AsyncRoutine {
    typedef AsyncRoutine BaseType;
    stream::Stream* _read{};
    stream::Stream* _write{};

    buffer::BufferView _view{};
    std::array<char, 1024 * 128> _buffer{};

public:
    AsyncTransport& operator ()(stream::Stream* read, stream::Stream* write) {
        _read = read;
        _write = write;
        _view = {_buffer.data(), _buffer.size(), 0, 0};
        async_start(&AsyncTransport::read);
        return *this;
    }

    Async read() {
        if (_view.capacity() == 0) {
            _view.reset_empty();
        }
        return *this / _read->read(&_view) / &AsyncTransport::write;
    }

    Async write() {
        return *this / _write->write(&_view) / &AsyncTransport::read;
    }
};

static unsigned int psk_server_cb(SSL* ssl, const char* identity, unsigned char *psk, unsigned int max_psk_len)
{
    std::array<char, 32> password{};
    std::fill(password.begin(), password.end(), 'K');
    memcpy(psk, password.data(), password.size());
    return password.size();
}

static unsigned int psk_client_cb(
    SSL *ssl, 
    const char *hint,
    char *identity,
    unsigned int max_identity_len,
    unsigned char *psk,
    unsigned int max_psk_len)
{
    strcpy(identity, "hello");
    return psk_server_cb(ssl, identity, psk, max_psk_len);
}

int main(int argc, char *argv[])
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);

    OpenSSLContext context_server{SSL_CTX_new(TLS_server_method())};    
    SSL_CTX_set_options(context_server, SSL_OP_NO_COMPRESSION);
    SSL_CTX_use_psk_identity_hint(context_server, nullptr);
    SSL_CTX_set_psk_server_callback(context_server, psk_server_cb);

    StreamBIO bio_server{};
    bio_server.initialize().abort_on(Failed_, "faield to create BIO");

    OpenSSLConnection connection_server{SSL_new(context_server)};
    bio_server.get_bio().up_ref();
    SSL_set_bio(connection_server, bio_server.get_bio(), bio_server.get_bio());
    SSL_set_app_data(connection_server, &bio_server);

    StreamOpenSSL<AsyncIOBIO> ssl_server{};
    ssl_server.initialize(connection_server);

    // client
    OpenSSLContext context_client{SSL_CTX_new(TLS_client_method())};    
    SSL_CTX_set_options(context_client, SSL_OP_NO_COMPRESSION);
    SSL_CTX_use_psk_identity_hint(context_client, nullptr);
    SSL_CTX_set_psk_client_callback(context_client, psk_client_cb);

    StreamBIO bio_client{};
    bio_client.initialize().abort_on(Failed_, "faield to create BIO");

    OpenSSLConnection connection_client{SSL_new(context_client)};
    bio_client.get_bio().up_ref();
    SSL_set_bio(connection_client, bio_client.get_bio(), bio_client.get_bio());
    SSL_set_app_data(connection_client, &bio_client);

    StreamOpenSSL<AsyncIOBIO> ssl_client{};
    ssl_client.initialize(connection_client);

    loop.task_new<AsyncTransport>(&bio_server, &bio_client);
    loop.task_new<AsyncTransport>(&bio_client, &bio_server);

    loop.task_new<AsyncServer>(&ssl_server);
    loop.task_new<AsyncClient>(&ssl_client);

    try {
        loop.run();
    } catch(...) {
        loop.exit();
    }
    return 0;
}
