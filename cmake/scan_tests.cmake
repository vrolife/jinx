
function(_add_jinx_test)
    cmake_parse_arguments(TST "" "_SOURCE" "SOURCES;LIBRARIES" ${ARGN})
    
    string(REPLACE .cpp "" TARGET_NAME "${TST__SOURCE}")

    add_executable(${TARGET_NAME} ${TST__SOURCE} ${TST_SOURCES})
    target_link_libraries(${TARGET_NAME} PRIVATE jinx ${TST_LIBRARIES})
    target_compile_definitions(${TARGET_NAME} PRIVATE -DJINX_FORCE_ASSERT=1)
    
    if (EXISTS "${TARGET_NAME}.cmake")
        include("${TARGET_NAME}.cmake")
    endif()

    add_test(NAME ${TARGET_NAME} COMMAND $<TARGET_FILE:${TARGET_NAME}>)
endfunction(_add_jinx_test)

function(jinx_scan_tests)
    cmake_parse_arguments(TST "" "_SOURCE" "SOURCES;LIBRARIES" ${ARGN})

    file(GLOB_RECURSE SOURCES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/ test_*.cpp)

    foreach(SOURCE ${SOURCES})
        if (JINX_TEST_WHITE_LIST)
            if (SOURCE IN_LIST JINX_TEST_WHITE_LIST)
                _add_jinx_test(${ARGN} _SOURCE ${SOURCE} LIBRARIES ${TST_LIBRARIES})
            endif()
        else()
            _add_jinx_test(${ARGN} _SOURCE ${SOURCE} LIBRARIES ${TST_LIBRARIES})
        endif()
    endforeach()
endfunction()
