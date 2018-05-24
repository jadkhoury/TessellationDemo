//////////////////////////////////////////////////////////////////////////////
//
// Longest Edge Bisection (LEB) Subdivision Demo
//
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "imgui.h"
#include "imgui_impl.h"

#define DJ_OPENGL_IMPLEMENTATION 1
#include "dj_opengl.h"
#define DJ_ALGEBRA_IMPLEMENTATION 1
#include "dj_algebra.h"

#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <vector>
#include <stack>

#include <iostream>
#include <fstream>
#include <string>
#include <bitset>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include <ctime>

//// SOURCE FILES
#include "quadtree.h"
//#include "mesh.h"
//#include "viewer.h"
#include "utility.h"

#define OUT uint32_t(-1)

using glm::bvec3;

using std::cout;
using std::endl;
using std::string;
using std::bitset;
typedef std::bitset<32> bits;

struct OpenGLManager {
    int i;
} g_gl = {0};

// -----------------------------------------------------------------------------
void Init(int argc, char **argv)
{

}

void Draw()
{
    glClearColor(0.8, 0.8, 0.8, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Cleanup()
{

}

// -----------------------------------------------------------------------------
void RenderGui()
{
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(256, 1024));
    ImGui::Begin("Window");
    {
        ImGui::Text("HELLO");
    }
    ImGui::End();

    ImGui::Render();
}


// -----------------------------------------------------------------------------
void KeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int models) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard)
        return;

    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GL_TRUE);
            break;
        default: break;
        }
    }
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;
}

void MouseMotionCallback(GLFWwindow* window, double x, double y)
{
    static double x0 = 0, y0 = 0;
    static uint32_t old_active;
    double dx = x - x0,
            dy = y - y0;

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;
}

void MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;
}

// -----------------------------------------------------------------------------
int main(int argc, char **argv)
{
    std::srand(std::time(nullptr));

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifndef NDEBUG
    glfwWindowHint (GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif
    // Create the Window
    LOG("Loading {Window-Main}\n");
    GLFWwindow* window = glfwCreateWindow(1024+256, 1024, "Hello Imgui", NULL, NULL);
    if (window == NULL) {
        LOG("=> Failure <=\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, &KeyboardCallback);
    glfwSetCursorPosCallback(window, &MouseMotionCallback);
    glfwSetMouseButtonCallback(window, &MouseButtonCallback);
    glfwSetScrollCallback(window, &MouseScrollCallback);

    // Load OpenGL functions
    LOG("Loading {OpenGL}\n");
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        LOG("gladLoadGLLoader failed\n");
        return -1;
    }
#ifndef NDEBUG
    log_debug_output();
#endif
    LOG("-- Begin -- Demo\n");
    try {

        Init(argc, argv);
        ImGui::CreateContext();
        ImGui_ImplGlfwGL3_Init(window, false);
        ImGui::StyleColorsDark();

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            ImGui_ImplGlfwGL3_NewFrame();
            Draw();
            RenderGui();
            ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(window);
        }

        ImGui_ImplGlfwGL3_Shutdown();
        ImGui::DestroyContext();
        Cleanup();
        glfwTerminate();
    } catch (std::exception& e) {
        LOG("%s", e.what());
        ImGui_ImplGlfwGL3_Shutdown();
        ImGui::DestroyContext();
        glfwTerminate();
        LOG("(!) Demo Killed (!)\n");

        return EXIT_FAILURE;
    } catch (...) {
        ImGui_ImplGlfwGL3_Shutdown();
        ImGui::DestroyContext();
        glfwTerminate();
        LOG("(!) Demo Killed (!)\n");

        return EXIT_FAILURE;
    }
    LOG("-- End -- Demo\n");


    return 0;
}

