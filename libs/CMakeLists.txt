if(JINX_BUILD_HTTP)
add_subdirectory(http)
endif()

if(JINX_BUILD_OPENSSL)
add_subdirectory(openssl)
endif()

if(JINX_BUILD_CJSON)
add_subdirectory(cjson)
endif()

if(JINX_BUILD_EVDNS)
add_subdirectory(evdns)
endif()
