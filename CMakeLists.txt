cmake_minimum_required(VERSION 3.13.4)
project(MyLLVMSuperbuild C CXX)

add_compile_options(
  -fno-semantic-interposition
)

set(ZLIB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/llvm/projects/jove/zlib")
set(ZLIB_USE_STATIC_LIBS ON)
set(ZLIB_BUILD_EXAMPLES OFF)

# Build zlib from source first
function(jove_add_zlib)
  set(BUILD_SHARED_LIBS OFF)
  add_subdirectory(llvm/projects/jove/zlib)
endfunction()
jove_add_zlib()

set(ZLIB_LIBRARY "${CMAKE_BINARY_DIR}/llvm/projects/jove/zlib/libz.a")

add_subdirectory(llvm)
