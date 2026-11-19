# Babarchive

A C++ project using CMake build system.

## Requirements

- CMake 3.20 or higher
- C++ compiler with C++20 support
- JetBrains Rider

## Building and Running

This project uses CMake Presets for configuration. Open the project in Rider and use the IDE's built-in CMake tools to build and run:

1. Open the project in Rider
2. Rider will automatically detect the CMake configuration from `CMakePresets.json`
3. Select the desired build configuration from the toolbar
4. Use the Run/Debug buttons to build and execute the project

## CMake targets

- `BabarchiveLib` - static library to create, read and modify `.babar` archives (`src/Archive`, `src/Core`, zlib). It doesn't depend on the console and can be linked by other projects.
- `Babarchive` - console tool built on top of the library (`src/main.cpp`, `src/Console`, `src/HexViewer`). Run `Babarchive --help` for the list of commands.

## Project Structure

- `src/Archive/` - archive library: `Packer`, `FileStream`, file format structs, encryption
- `src/Core/` - errors, logging and file helpers used by the library
- `src/Console/` - command line commands
- `src/HexViewer/` - hexadecimal viewer used by the `--hex` command
- `doc/` - documentation (archive usage, file format, command line)
- `CMakeLists.txt` - CMake configuration
