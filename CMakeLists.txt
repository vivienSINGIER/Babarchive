cmake_minimum_required(VERSION 3.20)
project(Babarchive)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(vendor/zlib)

add_executable(Babarchive src/main.cpp
    src/Archive/Structs.h
    src/Archive/Packer.h        src/Archive/Packer.cpp
    src/Archive/FileStream.h    src/Archive/FileStream.cpp
    
    src/Core/Error.h            src/Core/Error.cpp
    src/Core/FileHelper.h       src/Core/FileHelper.cpp
    src/Core/Logger.h           src/Core/Logger.cpp
        
    src/HexViewer/HexViewer.h   src/HexViewer/HexViewer.cpp
)

target_link_libraries(Babarchive PRIVATE ZLIB::ZLIBSTATIC)

# Run the program from the folder holding this CMakeLists (paths like "Test/..." are relative to it)
target_compile_definitions(Babarchive PRIVATE BABARCHIVE_WORKING_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(Babarchive PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
