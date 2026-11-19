cmake_minimum_required(VERSION 3.20)
project(Babarchive)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(vendor/zlib)

# --- Archive library: everything needed to create, read and modify .babar archives ---
add_library(BabarchiveLib STATIC
    src/Archive/Structs.h
    src/Archive/Packer.h        src/Archive/Packer.cpp
    src/Archive/FileStream.h    src/Archive/FileStream.cpp
    src/Archive/Encryption.hpp

    src/Core/Error.h            src/Core/Error.cpp
    src/Core/FileHelper.h       src/Core/FileHelper.cpp
    src/Core/Logger.h           src/Core/Logger.cpp
)

target_include_directories(BabarchiveLib PUBLIC src)
target_link_libraries(BabarchiveLib PUBLIC ZLIB::ZLIBSTATIC)

# --- Assets (static lib, depends on the archive)
add_library(BabarsetEngine STATIC
    src/Asset/AssetTypes.h      
    src/Asset/AssetHandler.h    src/Asset/AssetHandler.cpp
    src/Asset/AssetPacker.h     src/Asset/AssetPacker.cpp
    src/Asset/Scene.h           src/Asset/Scene.cpp
    
    src/Asset/Importers/BMPLoader.cpp
    src/Asset/Importers/ObjLoader.cpp
)

target_link_libraries(BabarsetEngine PUBLIC BabarchiveLib)

# --- Console tool: command line interface built on top of the library ---
add_executable(BabarchiveTool
    src/Console/main.cpp

    src/Console/Console.h       src/Console/Console.cpp
    src/Console/Commands.h      src/Console/Commands.cpp

    src/HexViewer/HexViewer.h   src/HexViewer/HexViewer.cpp
)

# --- Console tool: command line interface built on top of the asset engine ---
add_executable(BabarsetTool
        src/BabarsetDemo/main.cpp
)

target_link_libraries(BabarchiveTool PRIVATE BabarchiveLib)
target_link_libraries(BabarsetTool PRIVATE BabarsetEngine)

# Run the program from the folder holding this CMakeLists (paths like "Test/..." are relative to it)
target_compile_definitions(BabarchiveTool PRIVATE BABARCHIVE_WORKING_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(BabarchiveTool PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

target_compile_definitions(BabarsetTool PRIVATE BABARCHIVE_WORKING_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(BabarsetTool PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
