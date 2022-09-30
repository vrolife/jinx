
if (APPLE AND NOT OPENSSL_ROOT_DIR)
    set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
endif(APPLE AND NOT OPENSSL_ROOT_DIR)

find_package(OpenSSL REQUIRED)

add_library(jinx_openssl STATIC openssl.cpp)
add_library(jinx::openssl ALIAS jinx_openssl)
target_include_directories(jinx_openssl PUBLIC include)
target_link_libraries(jinx_openssl PUBLIC jinx OpenSSL::SSL)

if(NOT JINX_NO_TESTS)
    add_subdirectory(tests)
endif(NOT JINX_NO_TESTS)
