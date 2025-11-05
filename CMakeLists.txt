cmake_minimum_required(VERSION 3.16)
project(RxTx LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 모든 빌드 결과를 bin에 통합
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# include 경로 추가
include_directories(${CMAKE_SOURCE_DIR}/inc)

# 하위 프로젝트 추가
add_subdirectory(src/libRxTx)
add_subdirectory(src/appRxTx)
