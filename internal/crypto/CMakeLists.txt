add_library(${CRYPTO_LIBRARY} STATIC Sha256.cpp)
target_include_directories(${CRYPTO_LIBRARY} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
