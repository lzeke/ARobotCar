cmake_minimum_required(VERSION 3.10.0)
project(robotcar VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

set(OPENSSL_ROOT_DIR "~/vcpkg/packages/openssl_arm64-linux")
find_package(OpenSSL REQUIRED)
find_package(google_cloud_cpp_speech REQUIRED)

file(GLOB SOURCES "src/*.cpp")

add_executable(robotcar ${SOURCES})

target_include_directories(robotcar PUBLIC ./include /usr/local/include/PiPCA9685
                           ~/vcpkg/packages/google-cloud-cpp_arm64-linux/include
                           ~/vcpkg/packages/abseil_arm64-linux/include                          
                           ~/vcpkg/packages/grpc_arm64-linux/include
                           ~/vcpkg/packages/protobuf_arm64-linux/include
                           ~/Projects/GoogleSpeechToText/include)


target_link_directories(robotcar PRIVATE ~/Projects/GoogleSpeechToText/lib)

target_link_libraries(robotcar PUBLIC gpiod PiPCA9685 i2c tof sndfile fvad espeak-ng PRIVATE libgooglespeechtotext.so)
