cmake_minimum_required(VERSION 3.16)
project(roost)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON) # Require C++20

find_package(PythonInterp) # suppresses some warning while building; not required though
find_package(Torch REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -fPIC -Wall -Wextra -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls")
include_directories(src/include)

# add_subdirectory(src)
# file(GLOB SOURCES "src/*.cpp")
set(SOURCES src/main.cpp
        src/game/Action.cpp
        src/game/GameState.cpp
        src/play/Match.cpp
        src/player/RandomPlayer.cpp
        src/player/Evaluator.cpp
        src/player/MCTSPlayer.cpp
        src/player/NNEvaluator.cpp)
add_executable(roost ${SOURCES})
target_link_libraries(roost "${TORCH_LIBRARIES}")
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

# add gtest
# enable_testing()
# include(FetchContent)
# FetchContent_Declare(
#         googletest
#         # Specify the commit you depend on and update it regularly.
#         URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
# )
# # For Windows: Prevent overriding the parent project's compiler/linker settings
# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# FetchContent_MakeAvailable(googletest)
#
# add_executable(
#         hello_test
#         src/game/Action.cpp
#         src/game/GameState.cpp
#         src/play/Match.cpp
#         src/player/RandomPlayer.cpp
#         src/player/Evaluator.cpp
#         src/player/MCTSPlayer.cpp
#         src/player/NNEvaluator.cpp
#         test/game_test.cpp
#         test/player_test.cpp)
# target_link_libraries(
#         hello_test
#         gtest_main
# )
# target_link_libraries(hello_test "${TORCH_LIBRARIES}")
#
# add_test(NAME sample_test COMMAND hello_test)