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
#include <openssl/err.h>

#include <jinx/openssl/openssl.hpp>

namespace jinx {
namespace openssl {

JINX_ERROR_IMPLEMENT(openssl, {
    switch(code.as<ErrorCodeOpenSSL>()) {
        case ErrorCodeOpenSSL::NoError:
            return "No error";
        case ErrorCodeOpenSSL::HandshakeFailed:
            return "Handshake failed";
        case ErrorCodeOpenSSL::Established:
            return "Connection established";
        case ErrorCodeOpenSSL::EndOfStream:
            return "End of stream";
        case ErrorCodeOpenSSL::SysCall:
            return ERR_error_string(SSL_ERROR_SYSCALL, nullptr);
        case ErrorCodeOpenSSL::ProtocolError:
            return ERR_error_string(SSL_ERROR_SSL, nullptr);
        case ErrorCodeOpenSSL::X509Lookup:
            return ERR_error_string(SSL_ERROR_WANT_X509_LOOKUP, nullptr);
        case ErrorCodeOpenSSL::WantRead:
            return ERR_error_string(SSL_ERROR_WANT_READ, nullptr);
        case ErrorCodeOpenSSL::WantWrite:
            return ERR_error_string(SSL_ERROR_WANT_WRITE, nullptr);
    }
});

} // namespace openssl
} // namespace jinx
