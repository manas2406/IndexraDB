cmake_minimum_required(VERSION 3.10)
project(VultureDB)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include include/nlq_ml include/nlq)

file(GLOB SOURCES 
    "src/*.cpp" 
    "src/nlq/*.cpp"
    "src/nlq_ml/*.cpp"
)

# Filter out standalone demo files from the main engine build targets
# to prevent multiple main() definition conflicts.
list(FILTER SOURCES EXCLUDE REGEX "src/demo.*\\.cpp")
list(FILTER SOURCES EXCLUDE REGEX "src/repro.*\\.cpp")

list(FILTER SOURCES EXCLUDE REGEX "src/main.cpp")

add_executable(vulturedb src/main.cpp ${SOURCES})
add_executable(verify_delete src/testers/verify_delete.cpp ${SOURCES})
