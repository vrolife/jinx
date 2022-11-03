
add_library(jinx_evdns STATIC src/evdns.cpp)
add_library(jinx::evdns ALIAS jinx_evdns)
target_include_directories(jinx_evdns PUBLIC include)

if(NOT LIBEVENT_FIND_PACKAGE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(libevent_extra REQUIRED IMPORTED_TARGET libevent_extra)
    target_link_libraries(jinx_evdns PUBLIC jinx PkgConfig::libevent_extra)

else(NOT LIBEVENT_FIND_PACKAGE)
    find_package(Libevent REQUIRED)
    target_link_libraries(jinx_evdns PUBLIC jinx libevent::extra)
endif(NOT LIBEVENT_FIND_PACKAGE)


add_subdirectory(tests)
