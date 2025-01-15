#  MinHook - The Minimalistic API Hooking Library for x64/x86
#  Copyright (C) 2009-2017 Tsuda Kageyu.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#   1. Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#   2. Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in the
#      documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
#  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
#  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
#  OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

cmake_minimum_required(VERSION 3.0)

#project(jvmti-unlock LANGUAGES C)

include(CMakePackageConfigHelpers)

set(JVMTI_UNLOCK_MAJOR_VERSION 1)
set(JVMTI_UNLOCK_MINOR_VERSION 3)
set(JVMTI_UNLOCK_PATCH_VERSION 3)
set(JVMTI_UNLOCK_VERSION ${JVMTI_UNLOCK_MAJOR_VERSION}.${JVMTI_UNLOCK_MINOR_VERSION}.${JVMTI_UNLOCK_PATCH_VERSION})

################
#    BUILD     # 
################

# Enable FetchContent module
include(FetchContent)

# Fetch MinHook repository
FetchContent_Declare(
    MinHook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG master  # Or use a specific tag/commit
)

FetchContent_MakeAvailable(MinHook)

option(BUILD_SHARED_LIBS "build shared version" OFF)

set(SOURCES_JVMTI_UNLOCK
  "src/jvmti_unlock.cpp"
)

find_package(JNI REQUIRED)

include_directories(${JNI_INCLUDE_DIRS} ${minhook_SOURCE_DIR}/include)

add_library(jvmti-unlock ${SOURCES_JVMTI_UNLOCK} "src/hooks.cpp" "src/hooks.h")

target_include_directories(jvmti-unlock PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
  $<INSTALL_INTERFACE:include>
)

target_include_directories(jvmti-unlock PRIVATE "src/" ${JNI_INCLUDE_DIRS})
target_link_libraries(jvmti-unlock PRIVATE ${JNI_LIBRARIES} ${minhook_SOURCE_DIR}/../minhook-build/minhook.x64d.lib)

if(WIN32)
  set_target_properties(jvmti-unlock PROPERTIES PREFIX "")
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)   
    set_target_properties(jvmti-unlock PROPERTIES DEBUG_POSTFIX ".x64d")
    set_target_properties(jvmti-unlock PROPERTIES RELEASE_POSTFIX ".x64")
    set_target_properties(jvmti-unlock PROPERTIES RELWITHDEBINFO_POSTFIX ".x64")
    set_target_properties(jvmti-unlock PROPERTIES MINSIZEREL_POSTFIX ".x64")
  else()
    set_target_properties(jvmti-unlock PROPERTIES DEBUG_POSTFIX ".x32d")
    set_target_properties(jvmti-unlock PROPERTIES RELEASE_POSTFIX ".x32")
    set_target_properties(jvmti-unlock PROPERTIES RELWITHDEBINFO_POSTFIX ".x32")
    set_target_properties(jvmti-unlock PROPERTIES MINSIZEREL_POSTFIX ".x32")
  endif()
else()
  set_target_properties(jvmti-unlock PROPERTIES PREFIX "lib")
  set_target_properties(jvmti-unlock PROPERTIES POSTFIX "")
  set_target_properties(jvmti-unlock PROPERTIES DEBUG_POSTFIX "d")
endif()

################
# CMAKE CONFIG # 
################

configure_package_config_file(
    "cmake/jvmti-unlock-config.cmake.in"
    "jvmti-unlock-config.cmake"
  INSTALL_DESTINATION 
    "lib/jvmti-unlock"
)

write_basic_package_version_file(
  "jvmti-unlock-config-version.cmake"
VERSION 
  ${JVMTI_UNLOCK_VERSION}
COMPATIBILITY
  AnyNewerVersion
)

install(
  FILES 
    "${CMAKE_CURRENT_BINARY_DIR}/jvmti-unlock-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/jvmti-unlock-config-version.cmake"
  DESTINATION 
    "lib/jvmti-unlock"
)

###################
#     INSTALL     #
###################

install(TARGETS jvmti-unlock
        EXPORT jvmti-unlock-targets
        RUNTIME DESTINATION "bin"
        ARCHIVE DESTINATION "lib"
        LIBRARY DESTINATION "lib"
)

install(
  EXPORT
    jvmti-unlock-targets
  NAMESPACE 
    jvmti-unlock::
  DESTINATION 
    "lib/jvmti-unlock"
)

install(
  DIRECTORY include DESTINATION .
)