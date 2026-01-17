cmake_minimum_required(VERSION 3.11)
project(RASPBERRYPROJECT)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# --- CURL CONFIGURATION ---
if(WIN32)
    set(CURL_INCLUDE_DIR "C:/curl/include")
    set(CURL_LIBRARY "C:/curl/lib/libcurl.dll.a") 
    message(STATUS "Windows: Using manual CURL paths")
else()
    find_package(CURL REQUIRED)
    set(CURL_INCLUDE_DIR ${CURL_INCLUDE_DIRS})
    set(CURL_LIBRARY ${CURL_LIBRARIES})
    message(STATUS "Linux: Using find_package for CURL")
endif()

# --- RAYLIB FETCHING ---
include(FetchContent)
FetchContent_Declare(
  raylib
  URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP NEW
)
FetchContent_MakeAvailable(raylib)

# --- EXECUTABLE ---
# Updated: Added 'src/' prefix so CMake knows where the code lives
add_executable(RASPBERRYPROJECT 
    src/main.cpp 
    src/cryptoAPI.cpp
)

# --- LINKING & INCLUDES ---

# This tells the compiler to look inside your 'include' folder for .h files
target_include_directories(RASPBERRYPROJECT PRIVATE include)

# This includes the CURL headers
target_include_directories(RASPBERRYPROJECT PRIVATE ${CURL_INCLUDE_DIR})

# Link libraries
target_link_libraries(RASPBERRYPROJECT raylib ${CURL_LIBRARY})

# Automatically copy config.json to the build directory
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/config.json" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")