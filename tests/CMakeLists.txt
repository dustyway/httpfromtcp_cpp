if(ENABLE_TESTING)

set(TEST_MAIN "unit_test")
set(TESTS_SOURCES
        request_test.cpp
        headers_test.cpp
#        sanitizer_test.cpp
)

add_executable(${TEST_MAIN} ${TESTS_SOURCES})
target_link_libraries(${TEST_MAIN} PUBLIC ${REQUEST_LIBRARY} Catch2::Catch2WithMain)

set(SERVER_TEST "server_test")
add_executable(${SERVER_TEST} server_test.cpp)
target_link_libraries(${SERVER_TEST} PUBLIC ${SERVER_LIBRARY} Catch2::Catch2WithMain pthread)
endif ()