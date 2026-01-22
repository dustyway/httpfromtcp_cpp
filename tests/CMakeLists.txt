if(ENABLE_TESTING)

set(TEST_MAIN "unit_test")
set(TESTS_SOURCES
        request_test.cpp
)

add_executable(${TEST_MAIN} ${TESTS_SOURCES})
target_link_libraries(${TEST_MAIN} PUBLIC ${REQUEST_LIBRARY} Catch2::Catch2WithMain)
endif ()