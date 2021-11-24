cmake_minimum_required(VERSION 3.16)
project(roost)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON) # Require C++17

include_directories(include)

# add_subdirectory(src)
# file(GLOB SOURCES "src/*.cpp")
set(SOURCES src/main.cpp src/game/Action.cpp src/game/GameState.cpp include/game/GameState.h)
add_executable(roost ${SOURCES})
# target_link_libraries(roost PRIVATE roost_lib)
# add_executable(roost src/main.cpp src/game/Action.cpp src/game/Action.h src/game/Game.cpp src/game/Game.h src/game/game_defs.h)

# locations to search for clang-format, clang-tidy
set(BUILD_SUPPORT_DIR "${CMAKE_SOURCE_DIR}/build_support")
set(CLANG_SEARCH_PATH "/usr/local/bin" "/usr/bin" "/usr/local/opt/llvm/bin" "/usr/local/opt/llvm@8/bin"
        "/usr/local/Cellar/llvm/8.0.1/bin")

if (NOT DEFINED CLANG_FORMAT_BIN)
    # attempt to find the binary if user did not specify
    find_program(CLANG_FORMAT_BIN
            NAMES clang-format clang-format-8
            HINTS ${BUSTUB_CLANG_SEARCH_PATH})
endif()
if ("${CLANG_FORMAT_BIN}" STREQUAL "CLANG_FORMAT_BIN-NOTFOUND")
    message(WARNING "Couldn't find clang-format.")
else()
    message(STATUS "Found clang-format at ${CLANG_FORMAT_BIN}")
endif()

# add clang format
add_custom_target(format ${BUILD_SUPPORT_DIR}/run_clang_format.py
        ${CLANG_FORMAT_BIN}
        ${BUILD_SUPPORT_DIR}/clang_format_exclusions.txt
        --source_dirs
        "${CMAKE_CURRENT_SOURCE_DIR}/src,"
        --fix
        --quiet
        )