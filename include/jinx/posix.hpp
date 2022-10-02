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
#ifndef __posix_hpp__
#define __posix_hpp__

#include <cerrno>
#include <cstring>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <jinx/slice.hpp>
#include <jinx/macros.hpp>
#include <jinx/result.hpp>
#include <jinx/error.hpp>
#include <jinx/eventengine.hpp>
#include <jinx/async.hpp>

#ifndef in_port_t
#define in_port_t uint16_t
#endif

namespace jinx {
namespace posix {

enum class ErrorPosix {
    ArgumentListTooLong = E2BIG, // Argument list too long.
    PermissionDenied = EACCES, // Permission denied.
    AddressInUse = EADDRINUSE, // Address in use.
    AddressNotAvailable = EADDRNOTAVAIL, // Address not available.
    AddressFamilyNotSupported = EAFNOSUPPORT, // Address family not supported.
    ResourceUnavailableTryAgain = EAGAIN, // Resource unavailable, try again.
    ConnectionAlreadyInProgress = EALREADY, // Connection already in progress.
    BadFileDescriptor = EBADF, // Bad file descriptor.
    BadMessage = EBADMSG, // Bad message.
    DeviceOrResourceBusy = EBUSY, // Device or resource busy.
    OperationCanceled = ECANCELED, // Operation canceled.
    NoChildProcesses = ECHILD, // No child processes.
    ConnectionAborted = ECONNABORTED, // Connection aborted.
    ConnectionRefused = ECONNREFUSED, // Connection refused.
    ConnectionReset = ECONNRESET, // Connection reset.
    ResourceDeadlockWouldOccur = EDEADLK, // Resource deadlock would occur.
    DestinationAddressRequired = EDESTADDRREQ, // Destination address required.
    ArgumentOutOfDomain = EDOM, // Mathematics argument out of domain of function.
    FileExists = EEXIST, // File exists.
    BadAddress = EFAULT, // Bad address.
    FileTooLarge = EFBIG, // File too large.
    HostIsUnreachable = EHOSTUNREACH, // Host is unreachable.
    IdentifierRemoved = EIDRM, // Identifier removed.
    IllegalByteSequence = EILSEQ, // Illegal byte sequence.
    OperationInProgress = EINPROGRESS, // Operation in progress.
    InterruptedFunction = EINTR, // Interrupted function.
    InvalidArgument = EINVAL, // Invalid argument.
    IoError = EIO, // I/O error.
    SocketIsConnected = EISCONN, // Socket is connected.
    IsADirectory = EISDIR, // Is a directory.
    TooManyLevelsOfSymbolicLinks = ELOOP, // Too many levels of symbolic links.
    FileDescriptorValueTooLarge = EMFILE, // File descriptor value too large.
    TooManyLinks = EMLINK, // Too many links.
    MessageTooLarge = EMSGSIZE, // Message too large.
    FilenameTooLong = ENAMETOOLONG, // Filename too long.
    NetworkIsDown = ENETDOWN, // Network is down.
    ConnectionAbortedByNetwork = ENETRESET, // Connection aborted by network.
    NetworkUnreachable = ENETUNREACH, // Network unreachable.
    TooManyFilesOpenInSystem = ENFILE, // Too many files open in system.
    NoBufferSpaceAvailable = ENOBUFS, // No buffer space available.
    NoMessageIsAvailable = ENODATA, // No message is available on the STREAM head read queue. 
    NoSuchDevice = ENODEV, // No such device.
    NoSuchFileOrDirectory = ENOENT, // No such file or directory.
    ExecutableFileFormatError = ENOEXEC, // Executable file format error.
    NoLocksAvailable = ENOLCK, // No locks available.
    NotEnoughMemory = ENOMEM, // Not enough space.
    NoMessageOfTheDesiredType = ENOMSG, // No message of the desired type.
    ProtocolNotAvailable = ENOPROTOOPT, // Protocol not available.
    NoSpaceLeft = ENOSPC, // No space left on device.
    NoStreamResources = ENOSR, // No STREAM resources. 
    NotAStream = ENOSTR, // Not a STREAM. 
    FunctionalityNotSupported = ENOSYS, // Functionality not supported.
    TheSocketIsNotConnected = ENOTCONN, // The socket is not connected.
    NotADirectory = ENOTDIR, // Not a directory or a symbolic link to a directory.
    DirectoryNotEmpty = ENOTEMPTY, // Directory not empty.
    StateNotRecoverable = ENOTRECOVERABLE, // State not recoverable.
    NotASocket = ENOTSOCK, // Not a socket.
    NotSupported = ENOTSUP, // Not supported (may be the same value as [EOPNOTSUPP]).
    InappropriateIOControlOperation = ENOTTY, // Inappropriate I/O control operation.
    NoSuchDeviceOrAddress = ENXIO, // No such device or address.
    OperationNotSupported = EOPNOTSUPP, // Operation not supported on socket
    ValueTooLarge = EOVERFLOW, // Value too large to be stored in data type.
    PreviousOwnerDied = EOWNERDEAD, // Previous owner died.
    OperationNotPermitted = EPERM, // Operation not permitted.
    BrokenPipe = EPIPE, // Broken pipe.
    ProtocolError = EPROTO, // Protocol error.
    ProtocolNotSupported = EPROTONOSUPPORT, // Protocol not supported.
    ProtocolWrongTypeForSocket = EPROTOTYPE, // Protocol wrong type for socket.
    ResultTooLarge = ERANGE, // Result too large.
    ReadOnlyFileSystem = EROFS, // Read-only file system.
    InvalidSeek = ESPIPE, // Invalid seek.
    NoSuchProcess = ESRCH, // No such process.
    Reserved = ESTALE, // Reserved.
    StreamTimeout = ETIME, // Stream timeout. 
    TimedOut = ETIMEDOUT, // Timed out.
    TextFileBusy = ETXTBSY, // Text file busy.
    OperationWouldBlock = EWOULDBLOCK, // Operation would block.
    CrossDeviceLink = EXDEV, // Cross-device link.
};

JINX_ERROR_DEFINE(posix, ErrorPosix)

const error::Category& category_again() noexcept;

enum class AddressStatus {
    Ok,
    InvalidAddress,
    UnsupportedAddressFamily
};

struct IOHandle {
    typedef int NativeHandleType;
protected:
    constexpr static NativeHandleType InvalidNativeHandle = -1;
    NativeHandleType _io_handle{InvalidNativeHandle};
public:
    explicit IOHandle(NativeHandleType io_handle) : _io_handle(io_handle) { }
    JINX_NO_COPY(IOHandle);

    IOHandle(IOHandle&& other) noexcept {
        _io_handle = other._io_handle;
        other._io_handle = InvalidNativeHandle;
    }

    IOHandle& operator=(IOHandle&& other)  noexcept {
        reset();
        _io_handle = other._io_handle;
        other._io_handle = InvalidNativeHandle;
        return *this;
    }

    ~IOHandle() {
        this->reset();
    }

    void reset(int filedesc = InvalidNativeHandle);

    NativeHandleType native_handle() const { return _io_handle; }
    
    NativeHandleType release() {
        auto handle = _io_handle;
        _io_handle = InvalidNativeHandle;
        return handle;
    }

    void set_non_blocking(bool enable) const;

    bool is_valid() const { return _io_handle != -1; }
};

template<typename Family>
class AddressChecker
{
    struct sockaddr* _addr{nullptr};
    socklen_t* _size{nullptr};
    socklen_t _capcity{0};

public:
    AddressChecker() = default;
    explicit AddressChecker(struct sockaddr* addr, socklen_t* size, socklen_t capcity) 
    : _addr(addr), _size(size), _capcity(capcity) 
    { }

    JINX_NO_COPY(AddressChecker);

    AddressChecker(AddressChecker&& other) noexcept {
        _addr = other._addr;
        _size = other._size;
        _capcity = other._capcity;
        other.discard();
    }

    AddressChecker& operator=(AddressChecker&& other) noexcept {
        _addr = other._addr;
        _size = other._size;
        _capcity = other._capcity;
        other.discard();
        return *this;
    }

    ~AddressChecker() {
#if !defined(NDEBUG) && !defined(JINX_RESULT_NO_CHECK)
#if defined(JINX_RESULT_THROW)
        if (_addr != nullptr) {
            throw std::logic_error("Unchecked address");  // NOLINT
        }
#else
        jinx_assert(_addr == nullptr && "Unchecked address");
#endif
#endif
    }

    inline
    struct sockaddr* data() { return _addr; }
    socklen_t* capcity() { return &_capcity; }

    void discard() {
        _addr = nullptr;
        _size = nullptr;
        _capcity = 0;
    }

    JINX_NO_DISCARD
    Result<AddressStatus> commit() {
        auto result = Family::check(_addr, _capcity);
        if (result.peek() == AddressStatus::Ok) {
            *_size = _capcity;
        }
        discard();
        return result;
    }
};

enum SocketAddressAbortOnFailure {
    SocketAddressAbortOnFailure
};

class SocketAddress {
    static_assert(sizeof(struct sockaddr_storage) >= sizeof(struct sockaddr_in), "incompatable struct sockaddr_storage");
    static_assert(sizeof(struct sockaddr_storage) >= sizeof(struct sockaddr_in6), "incompatable struct sockaddr_storage");
    static_assert(sizeof(struct sockaddr_storage) >= sizeof(struct sockaddr_un), "incompatable struct sockaddr_storage");

    struct sockaddr_storage _addr{};
    socklen_t _size{0};

public:
    SocketAddress() {
        reset();
    }

    template<size_t N>
    explicit SocketAddress(const char(&buf)[N], in_port_t port, enum SocketAddressAbortOnFailure ignored);

    void reset() noexcept {
        memset(&_addr, 0, sizeof(_addr));
        if (AF_UNSPEC != 0) {
            _addr.ss_family = AF_UNSPEC;
        }
        _size = 0;
    }

    sa_family_t family() const { return _addr.ss_family; }

    socklen_t capacity() const { return sizeof(_addr); }

    socklen_t size() const {
        return _size;
    }
    
    const struct sockaddr* data() const {
        return reinterpret_cast<const struct sockaddr*>(&_addr);
    }

    template<typename Checker>
    Checker buffer() {
        return Checker{reinterpret_cast<struct sockaddr*>(&_addr), &_size, capacity()};
    }
};

struct AddressUnspecified {
    JINX_NO_DISCARD
    static
    Result<AddressStatus> check(struct sockaddr* addr, socklen_t size) {
        return AddressStatus::Ok;
    }

};

class AddressINet {
public:
    JINX_NO_DISCARD
    static
    Result<AddressStatus> set_port(SocketAddress& addr, in_port_t port) {
        auto buffer = addr.buffer<AddressChecker<AddressUnspecified>>();
        switch(addr.family()) {
            case AF_INET:
                *buffer.capcity() = sizeof(struct sockaddr_in);
                reinterpret_cast<struct sockaddr_in*>(buffer.data())->sin_port = htons(port);
                break;
            case AF_INET6:
                *buffer.capcity() = sizeof(struct sockaddr_in6);
                reinterpret_cast<struct sockaddr_in6*>(buffer.data())->sin6_port = htons(port);
                break;
            default:
                buffer.discard();
                return AddressStatus::UnsupportedAddressFamily;
        }
        return buffer.commit();
    }

    JINX_NO_DISCARD
    static
    Result<AddressStatus> set_address(SocketAddress& addr, const char* str) 
    {
        auto buffer = addr.buffer<AddressChecker<AddressUnspecified>>();
        
        auto* addr4 = reinterpret_cast<struct sockaddr_in*>(buffer.data());
        addr4->sin_family = AF_INET;
        *buffer.capcity() = sizeof(struct sockaddr_in);

        int res = inet_pton(AF_INET, str, &addr4->sin_addr);

        if (res == -1) {
            buffer.discard();
            return AddressStatus::InvalidAddress;
        }

        if (res == 0) {
            auto* addr6 = reinterpret_cast<struct sockaddr_in6*>(buffer.data());
            addr6->sin6_family = AF_INET6;
            *buffer.capcity() = sizeof(struct sockaddr_in6);
            res = inet_pton(AF_INET6, str, &reinterpret_cast<struct sockaddr_in6*>(buffer.data())->sin6_addr);
        }

        if (res != 1) {
            buffer.discard();
            return AddressStatus::InvalidAddress;
        }

        return buffer.commit();
    }

    JINX_NO_DISCARD
    static
    Result<AddressStatus> set_address_port(SocketAddress& addr, const char* str, in_port_t port) 
    {
        auto result = set_address(addr, str);
        if (result.is_not(AddressStatus::Ok)) {
            return result;
        }
        return set_port(addr, port);
    }

    JINX_NO_DISCARD
    static
    Result<AddressStatus> set_raw(SocketAddress& addr, int family, ::jinx::SliceConst raw, in_port_t port) 
    {
        auto buffer = addr.buffer<AddressChecker<AddressUnspecified>>();
        buffer.data()->sa_family = family;

        if (addr.family() == AF_INET) {
            if (raw.size() != 4) {
                buffer.discard();
                return AddressStatus::InvalidAddress;
            }
            auto* addr4 = reinterpret_cast<struct sockaddr_in*>(buffer.data());
            addr4->sin_port = htons(port);
            *buffer.capcity() = sizeof(struct sockaddr_in);
            memcpy(&addr4->sin_addr, raw.data(), 4);

        } else if (addr.family() == AF_INET6) {
            if (raw.size() != 16) {
                buffer.discard();
                return AddressStatus::InvalidAddress;
            }
            auto* addr6 = reinterpret_cast<struct sockaddr_in6*>(buffer.data());
            addr6->sin6_port = htons(port);
            *buffer.capcity() = sizeof(struct sockaddr_in6);
            memcpy(&addr6->sin6_addr, raw.data(), 16);
        } else {
            buffer.discard();
            return AddressStatus::UnsupportedAddressFamily;
        }
        return buffer.commit();
    }

    JINX_NO_DISCARD
    static
    Result<AddressStatus> check(struct sockaddr* addr, socklen_t size) {
        if (addr->sa_family == AF_INET) {
            if (size != sizeof(struct sockaddr_in)) {
                return AddressStatus::InvalidAddress;
            }
        } else if (addr->sa_family == AF_INET6) {
            if (size != sizeof(struct sockaddr_in6)) {
                return AddressStatus::InvalidAddress;
            }
        } else {
            return AddressStatus::UnsupportedAddressFamily;
        }
        return AddressStatus::Ok;
    }

    template<size_t N>
    JINX_NO_DISCARD
    static
    Result<AddressStatus> get_address_string(const SocketAddress& addr, char (&output)[N]) {
        static_assert(N >= INET6_ADDRSTRLEN, "output buffer too small");
        return get_address_string(addr, SliceMutable{&output[0], N});
    }

    JINX_NO_DISCARD
    static
    Result<AddressStatus> get_address_string(const SocketAddress& addr, SliceMutable buffer) {
        if (addr.family() == AF_INET) {
            const auto* addr4 = reinterpret_cast<const struct sockaddr_in*>(addr.data());
            inet_ntop(AF_INET, &addr4->sin_addr, reinterpret_cast<char*>(buffer.data()), buffer.size());

        } else if (addr.family() == AF_INET6) {
            const auto* addr6 = reinterpret_cast<const struct sockaddr_in6*>(addr.data());
            inet_ntop(AF_INET6, &addr6->sin6_addr, reinterpret_cast<char*>(buffer.data()), buffer.size());
        } else {
            return AddressStatus::UnsupportedAddressFamily;
        }
        return AddressStatus::Ok;
    }

    JINX_NO_DISCARD
    static
    ResultBool get_port(const SocketAddress& addr, in_port_t &port) {
        if (addr.family() == AF_INET) {
            const auto* addr4 = reinterpret_cast<const struct sockaddr_in*>(addr.data());
            port = ntohs(addr4->sin_port);
            return true;
        }
        
        if (addr.family() == AF_INET6) {
            const auto* addr6 = reinterpret_cast<const struct sockaddr_in6*>(addr.data());
            port = ntohs(addr6->sin6_port);
            return true;
        }
        return false;
    }

    static bool compare(const SocketAddress& addr, const SocketAddress& other) {
        if (addr.family() != other.family()) {
            return false;
        }

        if (addr.family() == AF_INET) {
            const auto* addr4 = reinterpret_cast<const struct sockaddr_in*>(addr.data());
            const auto* other4 = reinterpret_cast<const struct sockaddr_in*>(other.data());
            return addr4->sin_addr.s_addr == other4->sin_addr.s_addr 
                and addr4->sin_port == other4->sin_port;
        }

        if (addr.family() == AF_INET) {
            const auto* addr6 = reinterpret_cast<const struct sockaddr_in6*>(addr.data());
            const auto* other6 = reinterpret_cast<const struct sockaddr_in6*>(other.data());
            return memcmp(&addr6->sin6_addr, &other6->sin6_addr, sizeof(addr6->sin6_addr)) == 0 
                and addr6->sin6_port == other6->sin6_port;
        }

        return false;
    }
};

template<size_t N>
SocketAddress::SocketAddress(const char(&buf)[N], in_port_t port, enum SocketAddressAbortOnFailure ignored)
{
    reset();
    if (AddressINet::set_address_port(*this, buf, port).is_not(AddressStatus::Ok)) {
        error::error("invalid address");
    }
}

class Socket : public IOHandle {
public:
    explicit Socket(NativeHandleType filedesc = InvalidNativeHandle) : IOHandle(filedesc) { }
    Socket(Socket&&) = default;
    Socket& operator=(Socket&&) = default;
    JINX_NO_COPY(Socket);
    ~Socket();

    int get_receive_buffer_size();
    int get_send_buffer_size();

    int set_receive_buffer_size(int size);
    int set_send_buffer_size(int size);

    int set_reuse_address(bool enable);
    int set_reuse_port(bool enable);

    int set_keep_alive(bool enable);

    Result<int> get_socket_name(SocketAddress& addr);
    Result<int> get_peer_name(SocketAddress& addr);

    JINX_NO_DISCARD
    Result<int> shutdown(int how);

    JINX_NO_DISCARD
    Result<int> bind(const char* address, in_port_t port);
    
    JINX_NO_DISCARD
    Result<int> bind(const SocketAddress& addr);

    JINX_NO_DISCARD
    Result<int> listen(int backlog);
    
    JINX_NO_DISCARD
    Result<int> create(int domain, int type, int protocol);
};

namespace detail {

template<typename EventEngine>
class PosixConnect
{
public:
    typedef EventEngine EventEngineType;
    typedef typename EventEngine::IOHandleNativeType IOHandleNativeType;

private:
    IOHandleNativeType _io_handle{-1};
    const struct sockaddr* _address{nullptr};
    socklen_t _len{0};
    bool _connected{false};

public:
    typedef int IOResultType;
    typedef typename EventEngine::IOTypeWrite IOType;
    constexpr static const char* IOTypeName = "connect";

    PosixConnect() = default;
    PosixConnect(IOHandleNativeType io_handle, const struct sockaddr* address, socklen_t address_len)
    : _io_handle(io_handle),
      _address(address),
      _len(address_len)
    { }

    IOHandleNativeType native_handle() const { return _io_handle; }

    void reset() noexcept {
        *this = PosixConnect{};
    }

    inline IOResult do_io() {
        if (_connected) {
            int err = 0;
            socklen_t len = sizeof(err);
            int err2 = ::getsockopt(_io_handle, SOL_SOCKET, SO_ERROR, &err, &len);
            if (err2 != 0) {
                err = errno;
            }

            IOResult result;
            result._int = err;
            result._error = error::Error(err, category_posix());
            return result;
        }
    
        _connected = true;
        return EventEngine::connect(_io_handle, _address, _len);
    }
};

template<typename EventEngine>
class PosixAccept
{
public:
    typedef EventEngine EventEngineType;
    typedef typename EventEngine::IOHandleNativeType IOHandleNativeType;

private:
    IOHandleNativeType _io_handle{-1};
    struct sockaddr* _address{nullptr};
    socklen_t* _address_len{nullptr};
    bool _connected{false};

public:
    typedef int IOResultType;
    typedef typename EventEngine::IOTypeRead IOType;
    constexpr static const char* IOTypeName = "accept";

    PosixAccept() = default;
    PosixAccept(IOHandleNativeType io_handle, struct sockaddr* address, socklen_t* address_len)
    : _io_handle(io_handle),
      _address(address),
      _address_len(address_len)
    { }

    IOHandleNativeType native_handle() const { return _io_handle; }

    void reset() noexcept {
        *this = PosixAccept{};
    }

    inline IOResult do_io() {
        return EventEngine::accept(_io_handle, _address, _address_len);
    }
};

template<typename EventEngine>
class PosixSendTo
{
public:
    typedef EventEngine EventEngineType;
    typedef typename EventEngine::IOHandleNativeType IOHandleNativeType;

private:
    IOHandleNativeType _io_handle{-1};
    SliceConst _buffer{};
    int _flags{0};
    const struct sockaddr* _address{nullptr};
    socklen_t _len{0};

public:
    typedef long IOResultType;
    typedef typename EventEngine::IOTypeWrite IOType;
    constexpr static const char* IOTypeName = "sendto";

    PosixSendTo() = default;
    PosixSendTo(IOHandleNativeType io_handle, SliceConst buffer, int flags, const struct sockaddr* address, socklen_t address_len)
    : _io_handle(io_handle),
      _buffer(buffer),
      _flags(flags),
      _address(address),
      _len(address_len)
    { }

    IOHandleNativeType native_handle() const { return _io_handle; }

    void reset() noexcept {
        *this = PosixSendTo{};
    }

    inline IOResult do_io() {
        return EventEngine::sendto(_io_handle, _buffer, _flags, _address, _len);
    }
};

template<typename EventEngine>
class PosixRecvFrom
{
public:
    typedef EventEngine EventEngineType;
    typedef typename EventEngine::IOHandleNativeType IOHandleNativeType;

private:
    IOHandleNativeType _io_handle{-1};
    SliceMutable _buffer{};
    int _flags{0};
    AddressChecker<AddressUnspecified> _address{};

public:
    typedef long IOResultType;
    typedef typename EventEngine::IOTypeRead IOType;
    constexpr static const char* IOTypeName = "recvfrom";

    PosixRecvFrom() = default;
    PosixRecvFrom(IOHandleNativeType io_handle, SliceMutable buffer, int flags, struct sockaddr* address, socklen_t* address_len)
    : _io_handle(io_handle),
      _buffer(buffer),
      _flags(flags),
      _address(address, address_len, *address_len)
    { }

    PosixRecvFrom(IOHandleNativeType io_handle, SliceMutable buffer, int flags, SocketAddress& address)
    : _io_handle(io_handle),
      _buffer(buffer),
      _flags(flags),
      _address(address.buffer<AddressChecker<AddressUnspecified>>())
    { }

    IOHandleNativeType native_handle() const { return _io_handle; }

    void reset() noexcept {
        *this = PosixRecvFrom{};
    }

    inline IOResult do_io() {
        auto result = EventEngine::recvfrom(_io_handle, _buffer, _flags, _address.data(), _address.capcity());
        if (not result._error) {
            _address.commit() >> JINX_IGNORE_RESULT;
        }
        return result;
    }
};

template<typename EventEngine>
class PosixRecv
{
public:
    typedef EventEngine EventEngineType;
    typedef typename EventEngine::IOHandleNativeType IOHandleNativeType;

private:
    IOHandleNativeType _io_handle{-1};
    SliceMutable _buffer{};
    int _flags{0};

public:
    typedef long IOResultType;
    typedef typename EventEngine::IOTypeRead IOType;
    constexpr static const char* IOTypeName = "recv";

    PosixRecv() = default;
    PosixRecv(IOHandleNativeType io_handle, SliceMutable buffer, int flags)
    : _io_handle(io_handle),
      _buffer(buffer),
      _flags(flags)
    { }

    IOHandleNativeType native_handle() const { return _io_handle; }

    void reset() noexcept {
        *this = PosixRecv{};
    }

    inline IOResult do_io() {
        return EventEngine::recv(_io_handle, _buffer, _flags);
    }
};

template<typename EventEngine>
class PosixSend
{
public:
    typedef EventEngine EventEngineType;
    typedef typename EventEngine::IOHandleNativeType IOHandleNativeType;

private:
    IOHandleNativeType _io_handle{-1};
    SliceConst _buffer{};
    int _flags{0};

public:
    typedef long IOResultType;
    typedef typename EventEngine::IOTypeWrite IOType;
    constexpr static const char* IOTypeName = "send";

    PosixSend() = default;
    PosixSend(IOHandleNativeType io_handle, SliceConst buffer, int flags)
    : _io_handle(io_handle),
      _buffer(buffer),
      _flags(flags)
    { }

    IOHandleNativeType native_handle() const { return _io_handle; }

    void reset() noexcept {
        *this = PosixSend{};
    }

    inline IOResult do_io() {
        return EventEngine::send(_io_handle, _buffer, _flags);
    }
};

template<typename EventEngine>
class PosixRead
{
public:
    typedef EventEngine EventEngineType;
    typedef typename EventEngine::IOHandleNativeType IOHandleNativeType;

private:
    IOHandleNativeType _io_handle{-1};
    SliceMutable _buffer{};

public:
    typedef long IOResultType;
    typedef typename EventEngine::IOTypeRead IOType;
    constexpr static const char* IOTypeName = "read";

    PosixRead() = default;
    PosixRead(IOHandleNativeType io_handle, SliceMutable buffer)
    : _io_handle(io_handle),
      _buffer(buffer)
    { }

    IOHandleNativeType native_handle() const { return _io_handle; }

    void reset() noexcept {
        *this = PosixRead{};
    }

    inline IOResult do_io() {
        return EventEngine::read(_io_handle, _buffer);
    }
};

template<typename EventEngine>
class PosixWrite
{
public:
    typedef EventEngine EventEngineType;
    typedef typename EventEngine::IOHandleNativeType IOHandleNativeType;

private:
    IOHandleNativeType _io_handle{-1};
    SliceConst _buffer{};

public:
    typedef long IOResultType;
    typedef typename EventEngine::IOTypeWrite IOType;
    constexpr static const char* IOTypeName = "write";

    PosixWrite() = default;
    PosixWrite(IOHandleNativeType io_handle, SliceConst buffer)
    : _io_handle(io_handle),
      _buffer(buffer)
    { }

    IOHandleNativeType native_handle() const { return _io_handle; }

    void reset() noexcept {
        *this = PosixWrite{};
    }

    inline IOResult do_io() {
        return EventEngine::write(_io_handle, _buffer);
    }
};

template<typename EventEngine>
class PosixReadV
{
public:
    typedef EventEngine EventEngineType;
    typedef typename EventEngine::IOHandleNativeType IOHandleNativeType;

private:
    IOHandleNativeType io_handle{-1};
    SliceMutable* _buffers{nullptr};
    int _count{0};

public:
    typedef long IOResultType;
    typedef typename EventEngine::IOTypeRead IOType;
    constexpr static const char* IOTypeName = "readv";

    PosixReadV() = default;
    PosixReadV(IOHandleNativeType io_hanlde, SliceMutable* buffers, int count)
    : io_handle(io_hanlde),
      _buffers(buffers),
      _count(count)
    {
        static_assert(sizeof(SliceMutable) == sizeof(struct iovec), "incompatible buffer type");
        static_assert(offsetof(SliceMutable, _data) == offsetof(struct iovec, iov_base), "incompatible buffer type");
        static_assert(offsetof(SliceMutable, _size) == offsetof(struct iovec, iov_len), "incompatible buffer type");
    }

    IOHandleNativeType native_handle() const { return io_handle; }

    void reset() noexcept {
        *this = PosixReadV{};
    }

    inline IOResult do_io() {
        return EventEngine::readv(io_handle, _buffers, _count);
    }
};

template<typename EventEngine>
class PosixWriteV
{
public:
    typedef EventEngine EventEngineType;
    typedef typename EventEngine::IOHandleNativeType IOHandleNativeType;

private:
    IOHandleNativeType _io_handle{-1};
    SliceConst* _buffers{};
    int _count{0};

public:
    typedef long IOResultType;
    typedef typename EventEngine::IOTypeWrite IOType;
    constexpr static const char* IOTypeName = "writev";

    PosixWriteV() = default;
    PosixWriteV(IOHandleNativeType io_handle, SliceConst* buffers, int count)
    : _io_handle(io_handle),
      _buffers(buffers),
      _count(count)
    {
        static_assert(sizeof(SliceConst) == sizeof(struct iovec), "incompatible buffer type");
        static_assert(offsetof(SliceConst, _data) == offsetof(struct iovec, iov_base), "incompatible buffer type");
        static_assert(offsetof(SliceConst, _size) == offsetof(struct iovec, iov_len), "incompatible buffer type");
    }

    IOHandleNativeType native_handle() const { return _io_handle; }

    void reset() noexcept {
        *this = PosixWriteV{};
    }

    inline IOResult do_io() {
        return EventEngine::writev(_io_handle, _buffers, _count);
    }
};

template<typename EventEngine>
class PosixRecvMsg
{
public:
    typedef EventEngine EventEngineType;
    typedef typename EventEngine::IOHandleNativeType IOHandleNativeType;

private:
    IOHandleNativeType _io_handle{-1};
    struct msghdr *_msg{nullptr};
    int _flags{0};

public:
    typedef long IOResultType;
    typedef typename EventEngine::IOTypeRead IOType;
    constexpr static const char* IOTypeName = "recvmsg";

    PosixRecvMsg() = default;
    PosixRecvMsg(IOHandleNativeType io_handle, struct msghdr *msg, int flags)
    : _io_handle(io_handle),
      _msg(msg),
      _flags(flags)
    { }

    IOHandleNativeType native_handle() const { return _io_handle; }

    void reset() noexcept {
        *this = PosixRecvMsg{};
    }

    inline IOResult do_io() {
        return EventEngine::recvmsg(_io_handle, _msg, _flags);
    }
};

template<typename EventEngine>
class PosixSendMsg
{
public:
    typedef EventEngine EventEngineType;
    typedef typename EventEngine::IOHandleNativeType IOHandleNativeType;

private:
    IOHandleNativeType _io_handle{-1};
    struct msghdr *_msg{nullptr};
    int _flags{0};

public:
    typedef long IOResultType;
    typedef typename EventEngine::IOTypeWrite IOType;
    constexpr static const char* IOTypeName = "writev";

    PosixSendMsg() = default;
    PosixSendMsg(IOHandleNativeType io_handle, struct msghdr *msg, int flags)
    : _io_handle(io_handle),
      _msg(msg),
      _flags(flags)
    { }

    IOHandleNativeType native_handle() const { return _io_handle; }

    void reset() noexcept {
        *this = PosixSendMsg{};
    }

    inline IOResult do_io() {
        return EventEngine::sendmsg(_io_handle, _msg, _flags);
    }
};

template<typename IOImpl>
class AsyncIOCommon
: public AsyncFunction<typename IOImpl::IOResultType>
{
public:
    typedef AsyncFunction<typename IOImpl::IOResultType> BaseType;
    typedef typename IOImpl::EventEngineType EventEngineType;

private:
    IOImpl _io_impl{};
    typename EventEngineType::EventHandleIO _handle{};

public:
    AsyncIOCommon() = default;
    JINX_NO_COPY_NO_MOVE(AsyncIOCommon);
    ~AsyncIOCommon() override = default;

    template<typename... Args>
    AsyncIOCommon& operator()(Args&&... args) 
    {
        this->reset();
        _io_impl = IOImpl{std::forward<Args>(args)...};
        this->async_start(&AsyncIOCommon::async_io);
        return *this;
    }

    typename IOImpl::IOHandleNativeType native_handle() const { return _io_impl.native_handle(); }

protected:
    void async_finalize() noexcept override {
        if (not _handle.empty()) {
            EventEngineType::remove_io(this, _handle) >> JINX_IGNORE_RESULT;
            _handle.reset();
        }
        _io_impl.reset();
        BaseType::async_finalize();
    }

    void print_backtrace(std::ostream& output_stream, int indent) override {
        output_stream 
            << (indent > 0 ? '-' : '|') 
            << "AsyncIOCommon{ type: " 
            << IOImpl::IOTypeName 
            << ", fd: " 
            << _io_impl.native_handle() 
            << " }" << std::endl;
        BaseType::print_backtrace(output_stream, indent + 1);
    }

    Async async_io() {
        if (not this->empty()) {
            return this->async_return();
        }

        auto result = _io_impl.do_io();
        if (not result._error) {
            this->emplace_result((typename IOImpl::IOResultType)result);
            return this->async_return();
        }
        
        if (result._error.category() == category_again()) {
            if (EventEngineType::add_io(
                    this,
                    typename IOImpl::IOType{}, 
                    _handle, 
                    _io_impl.native_handle(), 
                    &AsyncIOCommon<IOImpl>::io_callback, 
                    this).is(Failed_))
            {
                return this->async_throw(ErrorEventEngine::FailedToRegisterIOEvent);
            }
            return this->async_suspend();
        }
        return BaseType::async_throw(result._error);
    }

    static void io_callback(unsigned int flags, const error::Error& error, void* data) {
        auto* self = reinterpret_cast<AsyncIOCommon<IOImpl>*>(data);
        self->async_resume(error) >> JINX_IGNORE_RESULT;
    }
};

} // namespace detail

template<typename EventEngine>
struct AsyncIOPosix
{
    typedef EventEngine EventEngineType;
    typedef typename EventEngine::IOHandleType IOHandleType;
    typedef typename EventEngine::IOHandleNativeType IOHandleNativeType;

    using Write = detail::AsyncIOCommon<detail::PosixWrite<EventEngine>>;
    using Read = detail::AsyncIOCommon<detail::PosixRead<EventEngine>>;
    using ReadV = detail::AsyncIOCommon<detail::PosixReadV<EventEngine>>;
    using WriteV = detail::AsyncIOCommon<detail::PosixWriteV<EventEngine>>;
    using SendMsg = detail::AsyncIOCommon<detail::PosixSendMsg<EventEngine>>;
    using RecvMsg = detail::AsyncIOCommon<detail::PosixRecvMsg<EventEngine>>;

    using Send = detail::AsyncIOCommon<detail::PosixSend<EventEngine>>;
    using Recv = detail::AsyncIOCommon<detail::PosixRecv<EventEngine>>;
    using SendTo = detail::AsyncIOCommon<detail::PosixSendTo<EventEngine>>;
    using RecvFrom = detail::AsyncIOCommon<detail::PosixRecvFrom<EventEngine>>;
    using Accept = detail::AsyncIOCommon<detail::PosixAccept<EventEngine>>;
    using Connect = detail::AsyncIOCommon<detail::PosixConnect<EventEngine>>;
};

std::ostream& operator <<(std::ostream& output_stream, const SocketAddress& addr);

struct MemoryProvider {
    virtual void* alloc(size_t size) noexcept {
        return ::malloc(size); // NOLINT
    }

    virtual void free(void* mem) noexcept {
        return ::free(mem); // NOLINT
    }
};

} // namespace posix
} // namespace jinx

#endif
