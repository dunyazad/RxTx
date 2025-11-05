project(appRxTx LANGUAGES CXX)

# 소스/헤더 수집
file(GLOB APP_SOURCES
    ${CMAKE_SOURCE_DIR}/src/appRxTx/*.cpp
)
file(GLOB APP_HEADERS
    ${CMAKE_SOURCE_DIR}/inc/RxTx/*.h
)

# 실행 파일 생성
add_executable(appRxTx
    ${APP_SOURCES}
    ${APP_HEADERS}
)

# 소스 그룹 (VS 트리 정리용)
source_group("Source Files" FILES ${APP_SOURCES})
source_group("Header Files" FILES ${APP_HEADERS})

# include 경로
target_include_directories(appRxTx PRIVATE
    ${CMAKE_SOURCE_DIR}/inc
)

# DLL import 매크로 정의
target_compile_definitions(appRxTx PRIVATE RXTX_IMPORTS)

# RxTx DLL 링크
target_link_libraries(appRxTx PRIVATE RxTx)

# 실행 파일 출력 경로 지정
set_target_properties(appRxTx PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)

message(STATUS "Building appRxTx EXE → ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
