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
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <cstring>
#include <ostream>

#include <jinx/posix.hpp>

namespace jinx {
namespace posix {

JINX_ERROR_IMPLEMENT(posix, {
    switch(code.as<ErrorPosix>()) {
        case ErrorPosix::ArgumentListTooLong: return "Argument list too long";
        case ErrorPosix::PermissionDenied: return "Permission denied";
        case ErrorPosix::AddressInUse: return "Address in use";
        case ErrorPosix::AddressNotAvailable: return "Address not available";
        case ErrorPosix::AddressFamilyNotSupported: return "Address family not supported";
        case ErrorPosix::ResourceUnavailableTryAgain: return "Resource unavailable, try again";
        case ErrorPosix::ConnectionAlreadyInProgress: return "Connection already in progress";
        case ErrorPosix::BadFileDescriptor: return "Bad file descriptor";
        case ErrorPosix::BadMessage: return "Bad message";
        case ErrorPosix::DeviceOrResourceBusy: return "Device or resource busy";
        case ErrorPosix::OperationCanceled: return "Operation canceled";
        case ErrorPosix::NoChildProcesses: return "No child processes";
        case ErrorPosix::ConnectionAborted: return "Connection aborted";
        case ErrorPosix::ConnectionRefused: return "Connection refused";
        case ErrorPosix::ConnectionReset: return "Connection reset";
        case ErrorPosix::ResourceDeadlockWouldOccur: return "Resource deadlock would occur";
        case ErrorPosix::DestinationAddressRequired: return "Destination address required";
        case ErrorPosix::ArgumentOutOfDomain: return "Mathematics argument out of domain of function";
        case ErrorPosix::FileExists: return "File exists";
        case ErrorPosix::BadAddress: return "Bad address";
        case ErrorPosix::FileTooLarge: return "File too large";
        case ErrorPosix::HostIsUnreachable: return "Host is unreachable";
        case ErrorPosix::IdentifierRemoved: return "Identifier removed";
        case ErrorPosix::IllegalByteSequence: return "Illegal byte sequence";
        case ErrorPosix::OperationInProgress: return "Operation in progress";
        case ErrorPosix::InterruptedFunction: return "Interrupted function";
        case ErrorPosix::InvalidArgument: return "Invalid argument";
        case ErrorPosix::IoError: return "I/O error";
        case ErrorPosix::SocketIsConnected: return "Socket is connected";
        case ErrorPosix::IsADirectory: return "Is a directory";
        case ErrorPosix::TooManyLevelsOfSymbolicLinks: return "Too many levels of symbolic links";
        case ErrorPosix::FileDescriptorValueTooLarge: return "File descriptor value too large";
        case ErrorPosix::TooManyLinks: return "Too many links";
        case ErrorPosix::MessageTooLarge: return "Message too large";
        case ErrorPosix::FilenameTooLong: return "Filename too long";
        case ErrorPosix::NetworkIsDown: return "Network is down";
        case ErrorPosix::ConnectionAbortedByNetwork: return "Connection aborted by network";
        case ErrorPosix::NetworkUnreachable: return "Network unreachable";
        case ErrorPosix::TooManyFilesOpenInSystem: return "Too many files open in system";
        case ErrorPosix::NoBufferSpaceAvailable: return "No buffer space available";
        case ErrorPosix::NoMessageIsAvailable: return "No message is available on the STREAM head read queue";
        case ErrorPosix::NoSuchDevice: return "No such device";
        case ErrorPosix::NoSuchFileOrDirectory: return "No such file or directory";
        case ErrorPosix::ExecutableFileFormatError: return "Executable file format error";
        case ErrorPosix::NoLocksAvailable: return "No locks available";
        case ErrorPosix::NotEnoughMemory: return "Not enough space";
        case ErrorPosix::NoMessageOfTheDesiredType: return "No message of the desired type";
        case ErrorPosix::ProtocolNotAvailable: return "Protocol not available";
        case ErrorPosix::NoSpaceLeft: return "No space left on device";
        case ErrorPosix::NoStreamResources: return "No STREAM resources";
        case ErrorPosix::NotAStream: return "Not a STREAM";
        case ErrorPosix::FunctionalityNotSupported: return "Functionality not supported";
        case ErrorPosix::TheSocketIsNotConnected: return "The socket is not connected";
        case ErrorPosix::NotADirectory: return "Not a directory or a symbolic link to a directory";
        case ErrorPosix::DirectoryNotEmpty: return "Directory not empty";
        case ErrorPosix::StateNotRecoverable: return "State not recoverable";
        case ErrorPosix::NotASocket: return "Not a socket";
        case ErrorPosix::NotSupported: return "Not supported";
        case ErrorPosix::InappropriateIOControlOperation: return "Inappropriate I/O control operation";
        case ErrorPosix::NoSuchDeviceOrAddress: return "No such device or address";
#if EOPNOTSUPP != ENOTSUP
        case ErrorPosix::OperationNotSupported: return "Operation not supported on socket"
#endif
        case ErrorPosix::ValueTooLarge: return "Value too large to be stored in data type";
        case ErrorPosix::PreviousOwnerDied: return "Previous owner died";
        case ErrorPosix::OperationNotPermitted: return "Operation not permitted";
        case ErrorPosix::BrokenPipe: return "Broken pipe";
        case ErrorPosix::ProtocolError: return "Protocol error";
        case ErrorPosix::ProtocolNotSupported: return "Protocol not supported";
        case ErrorPosix::ProtocolWrongTypeForSocket: return "Protocol wrong type for socket";
        case ErrorPosix::ResultTooLarge: return "Result too large";
        case ErrorPosix::ReadOnlyFileSystem: return "Read-only file system";
        case ErrorPosix::InvalidSeek: return "Invalid seek";
        case ErrorPosix::NoSuchProcess: return "No such process";
        case ErrorPosix::Reserved: return "Reserved";
        case ErrorPosix::StreamTimeout: return "Stream timeout";
        case ErrorPosix::TimedOut: return "Timed out";
        case ErrorPosix::TextFileBusy: return "Text file busy";
#if EWOULDBLOCK != EAGAIN
        case ErrorPosix::OperationWouldBlock: return "Operation would block"
#endif
        case ErrorPosix::CrossDeviceLink: return "Cross-device link";
    }
    return "NoError";
});

static const Category_posix __category_again;
const jinx::error::Category& category_again() noexcept {
    return __category_again;
}

void IOHandle::reset(int filedesc) {
    if (_io_handle != InvalidNativeHandle) {
        int err = 0;
        do {
            if (::close(_io_handle) == -1) {
                err = errno;
            }
        } while(err == EINTR);
    }
    _io_handle = filedesc;
}

void IOHandle::set_non_blocking(bool enable) const {
    int flags = ::fcntl(_io_handle, F_GETFL);
    if (enable) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    ::fcntl(_io_handle, F_SETFL, flags); // TODO check
}

Socket::~Socket() {
    ::shutdown(_io_handle, SHUT_RDWR);
}

int Socket::get_receive_buffer_size()
{
    int size = 0;
    socklen_t len = sizeof(size);
    getsockopt(_io_handle, SOL_SOCKET, SO_RCVBUF, &size, &len);
    return size;
}

int Socket::get_send_buffer_size()
{
    int size = 0;
    socklen_t len = sizeof(size);
    getsockopt(_io_handle, SOL_SOCKET, SO_SNDBUF, &size, &len);
    return size;
}

int Socket::set_receive_buffer_size(int size)
{
    return setsockopt(_io_handle, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
}

int Socket::set_send_buffer_size(int size)
{
    return setsockopt(_io_handle, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
}

int Socket::set_reuse_address(bool enable)
{
    int val = int(enable);
    return setsockopt(_io_handle, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
}

int Socket::set_reuse_port(bool enable)
{
#ifdef SO_REUSEPORT
    int val = int(enable);
    return setsockopt(_io_handle, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val));
#endif
}

Result<int> Socket::get_socket_name(SocketAddress& addr)
{
    auto buffer = addr.buffer<AddressChecker<AddressUnspecified>>();
    auto res = ::getsockname(native_handle(), buffer.data(), buffer.capcity());
    if (res == 0) {
        buffer.commit() >> JINX_IGNORE_RESULT;
    }
    return res;
}

Result<int> Socket::get_peer_name(SocketAddress& addr)
{
    auto buffer = addr.buffer<AddressChecker<AddressUnspecified>>();
    auto res = ::getpeername(native_handle(), buffer.data(), buffer.capcity());
    if (res == 0) {
        buffer.commit() >> JINX_IGNORE_RESULT;
    }
    return res;
}

int Socket::set_keep_alive(bool enable)
{
    int enable_bool = int(enable);
    return ::setsockopt(native_handle(), SOL_SOCKET, SO_KEEPALIVE, &enable_bool, sizeof(enable_bool));
}

Result<int> Socket::shutdown(int how)
{
    return ::shutdown(_io_handle, how);
}

Result<int> Socket::bind(const char* address, in_port_t port)
{
    SocketAddress addr{};
    AddressINet::set_address_port(addr, address, port) >> JINX_IGNORE_RESULT;
    return ::bind(_io_handle, addr.data(), addr.size());
}

Result<int> Socket::bind(const SocketAddress& addr)
{
    return ::bind(_io_handle, addr.data(), addr.size());
}

Result<int> Socket::listen(int backlog)
{
    return ::listen(_io_handle, backlog);
}

Result<int> Socket::create(int domain, int type, int protocol)
{
    int filedesc = ::socket(domain, type, protocol);
    this->reset(filedesc);
    return filedesc;
}

std::ostream& operator <<(std::ostream& output_stream, const SocketAddress& addr)
{
    char buffer[INET6_ADDRSTRLEN];
    posix::AddressINet::get_address_string(addr, buffer) >> JINX_IGNORE_RESULT;
    output_stream << &buffer[0];
    return output_stream;
}

} // namespace posix
} // namespace jinx
