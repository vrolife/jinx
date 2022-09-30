
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/app.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/app.js
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND xxd -i -C app.js >${CMAKE_CURRENT_BINARY_DIR}/app.h
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/index.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/index.html
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND xxd -i -C index.html >${CMAKE_CURRENT_BINARY_DIR}/index.h
)

add_executable(http_server 
    server.cpp 
    static.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/app.h 
    ${CMAKE_CURRENT_BINARY_DIR}/index.h)
target_include_directories(http_server PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(http_server PRIVATE jinx jinx::http)

# if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16.0")
#     target_precompile_headers(http_server PUBLIC pch.hpp)
# endif()
