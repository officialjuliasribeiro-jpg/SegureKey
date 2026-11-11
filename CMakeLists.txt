cmake_minimum_required(VERSION 3.16)

project(SecureKey VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

find_path(HTTPLIB_INCLUDE_DIR httplib.h REQUIRED)

find_library(HTTPLIB_LIBRARY
    NAMES cpp-httplib
    REQUIRED
)

add_executable(SecureKey
    src/main.cpp
)

add_executable(SecureKeyServer
    src/server.cpp
    src/vault.cpp
    src/hill.cpp
    src/document_crypto.cpp
    src/document_store.cpp
)

target_include_directories(SecureKeyServer PRIVATE
    ${HTTPLIB_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(SecureKeyServer PRIVATE
    ${HTTPLIB_LIBRARY}
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
)

foreach(TARGET SecureKey SecureKeyServer)
    target_compile_options(${TARGET} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endforeach()