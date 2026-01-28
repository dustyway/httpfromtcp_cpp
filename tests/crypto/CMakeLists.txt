set(CRYPTO_TEST "crypto_tests")
set(CRYPTO_TESTS_SOURCES
        sha256_test.cpp
        hmac_test.cpp
        aes_test.cpp
        gcm_test.cpp
        bigint_test.cpp
        rsa_test.cpp
        prf_test.cpp
)

add_executable(${CRYPTO_TEST} ${CRYPTO_TESTS_SOURCES})
target_link_libraries(${CRYPTO_TEST} PUBLIC ${CRYPTO_LIBRARY} OpenSSL::Crypto Catch2::Catch2WithMain)
