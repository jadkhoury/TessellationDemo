cmake_minimum_required (VERSION 3.1)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic -Wno-unused-variable -Wno-unused-function -Wno-unused-parameter" CACHE STRING "COMPILER OPTIONS" FORCE)
message(STATUS ${CMAKE_CXX_FLAGS})


project(TessellationDemo)

macro(copy_files_once)
    set(FILES_LIST "${ARGN}")
    foreach(SOME_FILE ${FILES_LIST})
        message(STATUS "Copying ${SOME_FILE} to build folder")
        file(COPY ${SOME_FILE} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    endforeach()
endmacro()

message(STATUS "**** Tessellation Demo, Loading Configuration ****")

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# disable GLFW docs, examples and tests
# see http://www.glfw.org/docs/latest/build_guide.html
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)


# set path to dependencies
set(SUBMOD_DIR common/submodules)

add_subdirectory(${SUBMOD_DIR}/glfw)
include_directories(${SUBMOD_DIR}/glfw/include)
include_directories(${SUBMOD_DIR}/glm)
include_directories(${SUBMOD_DIR}/imgui)
include_directories(${SUBMOD_DIR}/stb)
include_directories(${SUBMOD_DIR}/dj_opengl)
include_directories(${SUBMOD_DIR}/tinyobjloader)


# imgui implementation (compiled as a library)
aux_source_directory(${SUBMOD_DIR}/imgui IMGUI_SRC_FILES)
set(IMGUI_INCLUDE_DIR  common/imgui_impl ${SUBMOD_DIR}/imgui)
add_library(imgui STATIC common/imgui_impl/imgui_impl.cpp ${IMGUI_SRC_FILES})
include_directories(${IMGUI_INCLUDE_DIR})

# glad implementation (compiled as a library)
add_library(glad STATIC common/glad/glad/glad.c)
include_directories(common/glad)


# OBj files
file(GLOB_RECURSE OBJ_FILES *.obj)
copy_files_once(${OBJ_FILES})


# **************** Project 0_FullProgram **************** #

#message(STATUS "**** 0_FullProgram ****")

#set(SRC_DIR 0_FullProgram)

#file(GLOB_RECURSE SHADERS ${SRC_DIR}/*.glsl)
#file(GLOB_RECURSE HEADERS ${SRC_DIR}/*.h)

#include_directories(${SRC_DIR})
#aux_source_directory(${SRC_DIR} SRC_FILES)

#add_executable(program ${SRC_FILES} ${SHADERS} ${HEADERS} ${GLAD_C})

#target_link_libraries(program glad glfw imgui)

#unset(SRC_FILES)
#unset(SHADERS)
#unset(HEADERS)

# **************** Project 1_DistancePipeline **************** #

#message(STATUS "**** 1_DistancePipeline ****")

#set(SRC_DIR 1_DistancePipeline)

#file(GLOB_RECURSE SHADERS ${SRC_DIR}/*.glsl)
#file(GLOB_RECURSE HEADERS ${SRC_DIR}/*.h)

#include_directories(${SRC_DIR}})
#aux_source_directory(${SRC_DIR} SRC_FILES)

#add_executable(old_distance ${SRC_FILES} ${SHADERS} ${HEADERS} ${GLAD_C})

#target_link_libraries(old_distance glad glfw imgui)

#unset(SRC_FILES)
#unset(SHADERS)
#unset(HEADERS)

# **************** Project 2_NewDistance **************** #

 message(STATUS "**** 2_NewDistance ****")

 set(SRC_DIR 2_NewDistance/)

 file(GLOB_RECURSE SHADERS ${SRC_DIR}/*.glsl)
 file(GLOB_RECURSE HEADERS ${SRC_DIR}/*.h)

 include_directories(${SRC_DIR})
 aux_source_directory(${SRC_DIR} SRC_FILES)

 add_executable(new_distance ${SRC_FILES} ${SHADERS} ${HEADERS})

 target_link_libraries(new_distance glad glfw imgui)

 unset(SRC_FILES)
 unset(SHADERS)
 unset(HEADERS)



