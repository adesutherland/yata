# YATA - Yet Another Text Archive
cmake_minimum_required(VERSION 3.15.0)

# Static runtime
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

enable_language(C)

# Might as well share libraries on Ubuntu ...
# SET(CMAKE_EXE_LINKER_FLAGS "-static")
if(WIN32)
   # On windows we will link static to help portability
   set(CMAKE_EXE_LINKER_FLAGS "-static")
endif()

project ("yata")
add_executable (yata "yata.c")

#set(CPACK_PROJECT_NAME ${PROJECT_NAME})
#set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
#include(CPack)