#ifndef __jinx_openssl_bio_hpp__
#define __jinx_openssl_bio_hpp__

#include <openssl/ssl.h>

#include <jinx/stream.hpp>
#include <jinx/linkedlist.hpp>

#include <jinx/openssl/openssl.hpp>

namespace jinx {
namespace openssl {

class StreamBIO;

namespace detail {

constexpr static const int IOFlagRead = 1; 
constexpr static const int IOFlagWrite = 2; 
constexpr static const int IOFlagReadWrite = IOFlagRead | IOFlagWrite; 
constexpr static const int IOFlagPersist = 4;
constexpr static const int IOFlagEdgeTriggered = 8;

typedef void (*EventCallbackIO)(unsigned int flags, const error::Error& error, void* data);

class BIOEventCallback {
    StreamBIO* _stream;
    EventCallbackIO _callback;
    void* _data;
    unsigned int _flags;

public:
    BIOEventCallback(StreamBIO* stream, unsigned int flags, EventCallbackIO callback, void* data) 
    : _stream(stream),
      _flags(flags),
      _callback(callback),
      _data(data)
    { }

    void invoke() const {
        _callback(_flags, {}, _data);
    }

    void cancel() const {
        _callback(_flags, make_error(stream::ErrorStream::BrokenStream), _data);
    }

    unsigned int flags() const {
        return _flags;
    }

    ResultGeneric remove();
};

class BIORead : public Awaitable {
    friend class BIOHandle;
    friend StreamBIO;

    BIOEventCallback* _pending_write{nullptr};

    buffer::BufferView* _view{nullptr};
    size_t _capacity{0};

public:
    BIORead& operator()(buffer::BufferView* view) {
        _view = view;
        _capacity = _view->capacity();
        return *this;
    }
    
    ResultGeneric add_event(BIOEventCallback* event) {
        if (_pending_write != nullptr) {
            return Failed_;
        }
        _pending_write = event;
        return Successful_;
    }

    ResultGeneric remove_event(BIOEventCallback* event) {
        if (event != _pending_write and _pending_write != nullptr) {
            return Failed_;
        }
        _pending_write = nullptr;
        return Successful_;
    }

    void reset() {
        if (_pending_write != nullptr) {
            _pending_write->cancel();
            _pending_write = nullptr;
        }
        _view = nullptr;
    }

protected:
    void async_finalize() noexcept override {
        _view = nullptr;
        Awaitable::async_finalize();
    }

    Async async_poll() override 
    {
        if (_capacity > _view->capacity()) {
            return async_return();
        }
        // TODO EOS
        if (_pending_write != nullptr) {
            _pending_write->invoke();
            if ((_pending_write->flags() & detail::IOFlagPersist) == 0) {
                _pending_write = nullptr;
            }
        }
        return async_suspend();
    }
};

class BIOWrite : public Awaitable {
    friend class BIOHandle;
    friend StreamBIO;

    BIOEventCallback* _pending_read{};

    buffer::BufferView* _view{nullptr};


public:
    BIOWrite& operator()(buffer::BufferView* view) {
        _view = view;
        return *this;
    }

    ResultGeneric add_event(BIOEventCallback* event) {
        if (_pending_read != nullptr) {
            return Failed_;
        }
        _pending_read = event;
        return Successful_;
    }

    ResultGeneric remove_event(BIOEventCallback* event) {
        if (event != _pending_read and _pending_read != nullptr) {
            return Failed_;
        }
        _pending_read = nullptr;
        return Successful_;
    }

    void reset() {
        if (_pending_read != nullptr) {
            _pending_read->cancel();
            _pending_read = nullptr;
        }
        _view = nullptr;
    }

protected:
    void async_finalize() noexcept override {
        _view = nullptr;
        Awaitable::async_finalize();
    }

    Async async_poll() override {
        if (_view->size() == 0) {
            return async_return();
        }

        if (_pending_read != nullptr) {
            _pending_read->invoke();
            if ((_pending_read->flags() & detail::IOFlagPersist) == 0) {
                _pending_read = nullptr;
            }
        }
        return async_suspend();
    }
};

} // namespace detail

class StreamBIO : public stream::Stream {
    friend class EventEngineBIO;
    friend detail::BIOEventCallback;

    OpenSSLBIO _bio{};
    
    AsyncDoNothing _nop{};
    detail::BIORead _read{};
    detail::BIOWrite _write{};

public:
    StreamBIO() = default;
    ~StreamBIO() override = default;

    JINX_NO_COPY_NO_MOVE(StreamBIO);

    JINX_NO_DISCARD
    ResultGeneric initialize();

    OpenSSLBIO& get_bio() { return _bio; }

    Awaitable& shutdown() override {
        return _nop;
    }

    void reset() noexcept override {
        _read.reset();
        _write.reset();
        _bio.reset();
    }

    Awaitable& read(buffer::BufferView* view) override {
        return _read(view);
    }

    Awaitable& write(buffer::BufferView* view) override {
        return _write(view);
    }
private:
    ResultGeneric add_io(detail::BIOEventCallback* event) {
        if ((event->flags() & detail::IOFlagRead) != 0) {
            return _write.add_event(event);
        }
        return _read.add_event(event);
    }

    ResultGeneric remove_io(detail::BIOEventCallback* event) {
        if ((event->flags() & detail::IOFlagRead) != 0) {
            return _write.remove_event(event);
        }
        return _read.remove_event(event);
    }

    static int read(BIO *bio, char *buf, size_t size, size_t *readbytes) 
    {
        BIO_clear_flags(bio, BIO_FLAGS_READ);

        auto* self = reinterpret_cast<StreamBIO*>(BIO_get_app_data(bio));
        auto* view = self->_write._view;

        if (view == nullptr or view->size() == 0) {
            BIO_set_retry_read(bio);
            return -1;
        }

        auto wsz = std::min(size, view->size());
        memcpy(buf, view->begin(), wsz);
        *readbytes = wsz;
        view->consume(wsz) >> JINX_IGNORE_RESULT;
        self->_write.async_resume() >> JINX_IGNORE_RESULT;
        return 1;
    }

    static int write(BIO *bio, const char *buf, size_t size, size_t *writen) 
    {
        BIO_clear_flags(bio, BIO_FLAGS_WRITE);

        if (size == 0) {
            *writen = 0;
            return 1;
        }

        auto* self = reinterpret_cast<StreamBIO*>(BIO_get_app_data(bio));
        auto* view = self->_read._view;

        if (view == nullptr or view->capacity() == 0) {
            BIO_set_retry_write(bio);
            return -1;
        }

        auto data = view->slice_for_producer();
        auto rsz = std::min(size, view->capacity());
        memcpy(data.data(), buf, rsz);
        *writen = rsz;
        view->commit(rsz) >> JINX_IGNORE_RESULT;
        self->_read.async_resume() >> JINX_IGNORE_RESULT;
        return 1;
    }

    static long ctrl(BIO * bio, int cmd, long larg, void * parg)
    {
        switch(cmd) {
#ifdef BIO_CTRL_GET_KTLS_RECV
            case BIO_CTRL_GET_KTLS_RECV:
            case BIO_CTRL_GET_KTLS_SEND:
                return 0;
#endif
            case BIO_CTRL_FLUSH:
                return 1;
            case BIO_CTRL_PUSH:
            case BIO_CTRL_POP:
            case BIO_CTRL_DUP:
                return -1;
            default:
                break;
        }
        return 0;
    }
};

class EventEngineBIO : public EventEngineAbstract {
public:
    typedef StreamBIO* IOHandleNativeType;
    typedef OpenSSLConnection IOHandleType;
    typedef Optional<detail::BIOEventCallback> EventHandleIO;

    constexpr static const IOHandleNativeType IOHandleNativeInvalidValue = nullptr;

    constexpr static const int IOFlagRead = detail::IOFlagRead; 
    constexpr static const int IOFlagWrite = detail::IOFlagWrite; 
    constexpr static const int IOFlagReadWrite = detail::IOFlagReadWrite; 
    constexpr static const int IOFlagPersist = detail::IOFlagPersist;
    constexpr static const int IOFlagEdgeTriggered = detail::IOFlagEdgeTriggered;

    struct IOTypeRead {
        constexpr static unsigned int Flags = IOFlagRead;
    };

    struct IOTypeWrite {
        constexpr static unsigned int Flags = IOFlagWrite;
    };

    struct IOTypeReadWrite {
        constexpr static unsigned int Flags = IOFlagReadWrite;
    };

    static
    IOHandleNativeType get_native_handle(IOHandleType& handle) {
        return reinterpret_cast<IOHandleNativeType>(SSL_get_app_data(handle.get()));
    }

    static
    IOHandleNativeType get_native_handle(SSL* handle) {
        return reinterpret_cast<IOHandleNativeType>(SSL_get_app_data(handle));
    }

    static
    IOHandleNativeType get_native_handle(IOHandleNativeType handle) {
        return handle;
    }

    JINX_NO_DISCARD
    static
    inline ResultGeneric add_io(
        void* awaitable,
        const IOTypeRead& /*unused*/,
        EventHandleIO& event_handle,
        IOHandleNativeType io_handle,
        EventCallbackIO callback, void* arg) 
    { 
        return add_io(awaitable, IOTypeRead::Flags, event_handle, io_handle, callback, arg); 
    }

    JINX_NO_DISCARD
    static
    inline ResultGeneric add_io(
        void* awaitable,
        const IOTypeWrite& /*unused*/,
        EventHandleIO& event_handle,
        IOHandleNativeType io_handle,
        EventCallbackIO callback, void* arg) 
    {
        return add_io(awaitable, IOTypeWrite::Flags, event_handle, io_handle, callback, arg); 
    }

    JINX_NO_DISCARD
    static
    inline ResultGeneric add_io(
        void* awaitable,
        const IOTypeReadWrite& /*unused*/,
        EventHandleIO& event_handle,
        IOHandleNativeType io_handle,
        EventCallbackIO callback, void* arg) 
    { 
        return add_io(awaitable, IOTypeReadWrite::Flags, event_handle, io_handle, callback, arg); 
    }

    JINX_NO_DISCARD
    static
    ResultGeneric add_io(
        void* awaitable,
        unsigned int flags,
        EventHandleIO& handle,
        IOHandleNativeType io_handle,
        EventCallbackIO callback, void* arg) 
    {
        auto flags_rw = flags & IOFlagReadWrite;
        if (flags_rw == IOFlagReadWrite or flags_rw == 0) {
            return Failed_;
        }

        handle.emplace(io_handle, flags, callback, arg);
        return io_handle->add_io(&handle.get());
    }

    static
    ResultGeneric remove_io(void* awaitable, EventHandleIO& handle) {
        return handle.get().remove();
    }

    const void * get_type_tag() override;
};

struct AsyncIOBIO
{
    typedef EventEngineBIO EventEngineType;
    typedef typename EventEngineType::IOHandleType IOHandleType;
    typedef typename EventEngineType::IOHandleNativeType IOHandleNativeType;
};

} // namespace openssl
} // namespace jinx

#endif
