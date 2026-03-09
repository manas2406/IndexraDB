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

# Exclude demo files from the main engine build if they contain main()
# But wait, main.cpp has main(). 
# We should separate library and executable or just build the engine.
# Existing structure has everything in src.
# Let's filter out demo files for the main engine target
list(FILTER SOURCES EXCLUDE REGEX "src/demo.*\\.cpp")
list(FILTER SOURCES EXCLUDE REGEX "src/repro.*\\.cpp")

list(FILTER SOURCES EXCLUDE REGEX "src/main.cpp")

add_executable(vulturedb src/main.cpp ${SOURCES})
add_executable(verify_delete src/testers/verify_delete.cpp ${SOURCES})
