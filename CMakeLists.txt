cmake_minimum_required(VERSION 3.13)
project(DisneyStreaming)

set (CMAKE_CXX_STANDARD 20)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(MSVC)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /WX")
add_compile_options(/wd4251)
endif()

#Grab VKL as our first dependency - this will pull all of the vkl dependencies as well
message(STATUS "Pulling VKL (Eric Stancliff) from GitHub")
include(FetchContent)
FetchContent_Declare(
  VKL
  GIT_REPOSITORY https://github.com/EricStancliff/VKL.git
  GIT_TAG        origin/main
)
FetchContent_MakeAvailable(VKL)

get_property(EXTERNAL_DIR GLOBAL PROPERTY EXTERNAL_DIR)

cmake_policy(SET CMP0074 NEW)

InstallExternalNoFind(zlib)
set(ZLIB_ROOT ${EXTERNAL_DIR})
find_package(ZLIB REQUIRED)

InstallExternalNoFind(curl)
set(CURL_ROOT ${EXTERNAL_DIR})
find_package(CURL CONFIG REQUIRED)

InstallExternal_Ext(nlohmann-json nlohmann_json)

add_subdirectory(src)