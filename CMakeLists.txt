cmake_minimum_required(VERSION 3.16)
project(TVWC VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# OpenSSL Paketi
find_package(OpenSSL REQUIRED)

# Include Dizinleri
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OPENSSL_INCLUDE_DIR}
)

# Header ve Kaynak Dosyaları
file(GLOB_RECURSE HEADERS "include/*.hpp")
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Executable Derleme
add_executable(tvwc ${SOURCES} ${HEADERS})

# Kütüphane Bağlantıları (OpenSSL, Sockets & Threads)
target_link_libraries(tvwc PRIVATE 
    OpenSSL::SSL 
    OpenSSL::Crypto
)

if(UNIX)
    target_link_libraries(tvwc PRIVATE pthread)
endif()