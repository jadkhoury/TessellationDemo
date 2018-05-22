cmake_minimum_required (VERSION 2.6)

# disable GLFW docs, examples and tests
# see http://www.glfw.org/docs/latest/build_guide.html
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# set path to dependencies
add_subdirectory(submodules/glfw)
include_directories(submodules/glfw/include)
include_directories(submodules/imgui)
include_directories(submodules/stb)
include_directories(submodules/dj_opengl)
include_directories(submodules/dj_algebra)
# imgui source files
set(IMGUI_SRC_DIR submodules/imgui)
aux_source_directory(${IMGUI_SRC_DIR} IMGUI_SRC_FILES)

# ------------------------------------------------------------------------------
project (viewer-subd-leb)
set(SRC_DIR viewer-subd-leb)
include_directories(${SRC_DIR})
aux_source_directory(${SRC_DIR} SRC_FILES)
add_executable(viewer-subd-leb ${IMGUI_SRC_FILES} ${SRC_FILES} ${SRC_DIR}/glad/glad.c)
target_link_libraries(viewer-subd-leb glfw)
unset(SRC_FILES)

