cmake_minimum_required (VERSION 3.19)

set(PROJECT_NAME "InceptionEngine")

project (${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(INCEPTION_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(ENGINE_SOURCE_CODE_DIR "${INCEPTION_ROOT_DIR}/src")
set(THIRD_PARTY_DIR "${INCEPTION_ROOT_DIR}/thirdParty")

add_subdirectory(thirdParty)
add_subdirectory(src)






