cmake_minimum_required(VERSION 3.20)
project(Babarchive)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(vendor/zlib)

# --- Archive + Core (static lib)
add_library(Babarchive STATIC
    src/Archive/Structs.h
    src/Archive/Packer.h        src/Archive/Packer.cpp
    src/Archive/FileStream.h    src/Archive/FileStream.cpp
    src/Archive/Encryption.hpp

    src/Core/Error.h            src/Core/Error.cpp
    src/Core/FileHelper.h       src/Core/FileHelper.cpp
    src/Core/Logger.h           src/Core/Logger.cpp
)
target_include_directories(Babarchive PUBLIC src)
# PUBLIC: FileStream.h includes zlib.h, so users of the lib need it too
target_link_libraries(Babarchive PUBLIC ZLIB::ZLIBSTATIC)

# --- Assets (static lib, depends on the archive)
add_library(BabarsetEngine STATIC
    src/Asset/AssetTypes.h      
    src/Asset/AssetHandler.h    src/Asset/AssetHandler.cpp
    src/Asset/AssetPacker.h     src/Asset/AssetPacker.cpp
    src/Asset/Scene.h           src/Asset/Scene.cpp
    
    src/Asset/Importers/BMPLoader.cpp
    src/Asset/Importers/ObjLoader.cpp
)
target_link_libraries(BabarsetEngine PUBLIC Babarchive)

# --- Tool
add_executable(BabarTool src/main.cpp
    src/HexViewer/HexViewer.h   src/HexViewer/HexViewer.cpp
)
target_link_libraries(BabarTool PRIVATE BabarsetEngine)

# Run the program from the folder holding this CMakeLists (paths like "Test/..." are relative to it)
target_compile_definitions(BabarTool PRIVATE BABARCHIVE_WORKING_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(BabarTool PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
