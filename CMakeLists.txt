cmake_minimum_required(VERSION 2.8)

# 版本号
set(PASE_VERSION_MAJOR 0)
set(PASE_VERSION_MINOR 1)

message("")
message("project source dir: ${PROJECT_SOURCE_DIR}")
message("project binary dir: ${PROJECT_BINARY_DIR}")
message("")

# HYPRE 路径
#set(HYPRE /home/ycg/Software/hypre-2.11.2/src/hypre)
set(HYPRE /home/hongqichen/software/hypre-2.11.2/src/hypre)
include_directories(${HYPRE}/include)
set(LIB_HYPRE "${HYPRE}/lib/libHYPRE.a")

# c 编译器
set(CMAKE_C_COMPILER "mpicc")
set(CMAKE_C_FLAGS "-g -Wall -O3")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_HYPRE")

file(GLOB_RECURSE LIB_SRC ${PROJECT_SOURCE_DIR}/source/*.c)
include_directories(${PROJECT_SOURCE_DIR}/source)

# 库名称
set(LIB_PASE "pase")
add_library(${LIB_PASE} STATIC ${LIB_SRC})

# 测试 PASE_VECTOR1
file(GLOB_RECURSE TEST_VECTOR_SRC ${PROJECT_SOURCE_DIR}/test/source/test_pase_vector.c)
set(BIN_TEST_VECTOR_NAME "test_vector.exe")
add_executable(${BIN_TEST_VECTOR_NAME} ${TEST_VECTOR_SRC})
target_link_libraries(${BIN_TEST_VECTOR_NAME} ${LIB_PASE} ${LIB_HYPRE} "-lm")

# 测试 PASE_MATRIX
file(GLOB_RECURSE TEST_MATRIX_SRC ${PROJECT_SOURCE_DIR}/test/source/test_pase_matrix.c)
set(BIN_TEST_MATRIX_NAME "test_matrix.exe")
add_executable(${BIN_TEST_MATRIX_NAME} ${TEST_MATRIX_SRC})
target_link_libraries(${BIN_TEST_MATRIX_NAME} ${LIB_PASE} ${LIB_HYPRE} "-lm")