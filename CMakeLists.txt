cmake_minimum_required (VERSION 3.1)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

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
add_subdirectory(submodules/glfw)
include_directories(submodules/glfw/include)
include_directories(submodules/glm)
include_directories(submodules/imgui)
include_directories(submodules/stb)
include_directories(submodules/dj_opengl)
include_directories(submodules/tinyobjloader)

# imgui implementation (compiled as a library)
aux_source_directory(submodules/imgui IMGUI_SRC_FILES)
set(IMGUI_INCLUDE_DIR imgui submodules/imgui)
add_library(imgui STATIC imgui/imgui_impl.cpp ${IMGUI_SRC_FILES})

file(GLOB_RECURSE OBJ_FILES *.obj)
copy_files_once(${OBJ_FILES})

# **************** Project 0_FullProgram **************** #
message(STATUS "**** 0_FullProgram ****")

set(SRC_DIR 0_FullProgram/src)
file(GLOB_RECURSE SHADERS 0_FullProgram/*.glsl)
include_directories(${SRC_DIR}  ${IMGUI_INCLUDE_DIR})
aux_source_directory(${SRC_DIR} SRC_FILES)

add_executable(program ${SRC_FILES} ${SHADERS} ${SRC_DIR}/glad/glad.c)

target_link_libraries(program glfw imgui)
unset(SRC_FILES)
unset(SHADERS)


# **************** Project 1_DistancePipeline **************** #
message(STATUS "**** 1_DistancePipeline ****")

set(SRC_DIR 1_DistancePipeline/src)
file(GLOB_RECURSE SHADERS 1_DistancePipeline/*.glsl)
include_directories(${SRC_DIR}  ${IMGUI_INCLUDE_DIR})
aux_source_directory(${SRC_DIR} SRC_FILES)

add_executable(distance-tess ${SRC_FILES} ${SHADERS} ${SRC_DIR}/glad/glad.c)

target_link_libraries(distance-tess glfw imgui)
unset(SRC_FILES)
unset(SHADERS)


# **************** Project 2_NewDistance **************** #
message(STATUS "**** 2_NewDistance ****")

set(SRC_DIR 2_NewDistance/src)
file(GLOB_RECURSE SHADERS 2_NewDistance/*.glsl)
include_directories(${SRC_DIR}  ${IMGUI_INCLUDE_DIR})
aux_source_directory(${SRC_DIR} SRC_FILES)

add_executable(new-distance ${SRC_FILES} ${SHADERS} ${SRC_DIR}/glad/glad.c)

target_link_libraries(new-distance glfw imgui)
unset(SRC_FILES)
unset(SHADERS)


