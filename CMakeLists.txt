cmake_minimum_required(VERSION 3.13.4)
project(MyLLVMSuperbuild C CXX)

# Build zlib from source first
function(jove_add_zlib)
  set(ZLIB_LIBRARY zlibstatic)
  set(ZLIB_USE_STATIC_LIBS ON)
  set(ZLIB_BUILD_EXAMPLES OFF)
  set(BUILD_SHARED_LIBS OFF)
  add_subdirectory(llvm/projects/jove/zlib EXCLUDE_FROM_ALL)
endfunction()

jove_add_zlib()

add_subdirectory(llvm)
