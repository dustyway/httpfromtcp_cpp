add_executable(httpserver main.cpp handlers.cpp)
target_link_libraries(httpserver ${SERVER_LIBRARY} ${CRYPTO_LIBRARY})
