cmake_minimum_required(VERSION 3.16)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Network REQUIRED)

add_library(qodeassist-mcp STATIC
    MCPClientManager.cpp
    MCPToolAdapter.cpp
)

target_link_libraries(qodeassist-mcp
    Qt::Core
    Qt::Network
    Qt::Concurrent
    mcp
)

target_include_directories(qodeassist-mcp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/_deps/cpp-mcp-src/include
    ${CMAKE_BINARY_DIR}/_deps/cpp-mcp-src/common
    ${CMAKE_SOURCE_DIR}
)