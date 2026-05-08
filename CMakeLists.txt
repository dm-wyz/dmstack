# 
# 
#
cmake_minimum_required (VERSION 3.8)


project ("dmstack"
  VERSION 1.1.0
  DESCRIPTION "CallStack Collector && Parser"
  LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# 
add_subdirectory ("dmstack")
