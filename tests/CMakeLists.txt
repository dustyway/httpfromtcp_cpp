if(ENABLE_TESTING)

set(TEST_MAIN "unit_tests")
set(TESTS_SOURCES
        headers_test.cpp
        request_test.cpp
        response_test.cpp
#        sanitizer_test.cpp
)

add_executable(${TEST_MAIN} ${TESTS_SOURCES})
target_link_libraries(${TEST_MAIN} PUBLIC ${REQUEST_LIBRARY} ${RESPONSE_LIBRARY} Catch2::Catch2WithMain)

#set(RESPONSE_TEST "response_test")
#add_executable(${RESPONSE_TEST} response_test.cpp)
#target_link_libraries(${RESPONSE_TEST} PUBLIC ${RESPONSE_LIBRARY} Catch2::Catch2WithMain)

set(SERVER_TEST "server_test")
add_executable(${SERVER_TEST} server_test.cpp)
target_link_libraries(${SERVER_TEST} PUBLIC ${SERVER_LIBRARY} Catch2::Catch2WithMain pthread)
endif ()