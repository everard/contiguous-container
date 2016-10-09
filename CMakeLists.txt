set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")

project(build)
cmake_minimum_required(VERSION 2.8)

aux_source_directory(build SRC_LIST)
file(GLOB_RECURSE SOURCES0 "source/*")
file(GLOB SOURCES1 "build/*.*")

include_directories(/home/everard/Software/benchmark/include)
link_directories(/home/everard/Software/benchmark/src)
add_executable(${PROJECT_NAME} ${SRC_LIST} ${SOURCES0} ${SOURCES1})

set(CMAKE_CXX_FLAGS_DEBUG "-g")
file(STRINGS build/flags ADDITIONAL_FLAGS)
target_compile_options(${PROJECT_NAME} PUBLIC ${ADDITIONAL_FLAGS})
target_link_libraries(${PROJECT_NAME} pthread benchmark)
