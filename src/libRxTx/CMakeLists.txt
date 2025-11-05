project(libRxTx LANGUAGES CXX)

# 소스/헤더 수집
file(GLOB LIB_SOURCES
    ${CMAKE_SOURCE_DIR}/src/libRxTx/*.cpp
)
file(GLOB LIB_HEADERS
    ${CMAKE_SOURCE_DIR}/inc/RxTx/*.h
)

# DLL 생성
add_library(RxTx SHARED
    ${LIB_SOURCES}
    ${LIB_HEADERS}
)

# 소스 그룹 (VS 트리 정리용)
source_group("Source Files" FILES ${LIB_SOURCES})
source_group("Header Files" FILES ${LIB_HEADERS})

# include 경로
target_include_directories(RxTx PUBLIC
    ${CMAKE_SOURCE_DIR}/inc
)

# DLL export 매크로 정의
target_compile_definitions(RxTx PRIVATE RXTX_EXPORTS)

# Windows에서는 WinSock2 연결
if (WIN32)
    target_link_libraries(RxTx ws2_32)
endif()

# 출력 경로 확인용 메시지
message(STATUS "Building RxTx DLL → ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
