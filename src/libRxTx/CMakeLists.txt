project(RxTx LANGUAGES CXX)

# 소스/헤더 수집
file(GLOB LIB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)
file(GLOB LIB_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../inc/RxTx/*.h
)

# DLL 생성
add_library(RxTx SHARED
    ${LIB_SOURCES}
    ${LIB_HEADERS}
)

# include 경로
target_include_directories(RxTx PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../inc
)

# DLL export 매크로 정의
target_compile_definitions(RxTx PRIVATE RXTX_EXPORTS)

# 출력 경로
set_target_properties(RxTx PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../bin
)

message(STATUS "Building RxTx DLL → ${CMAKE_CURRENT_SOURCE_DIR}/../../bin")
