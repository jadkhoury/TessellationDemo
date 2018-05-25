////////////////////////////////////////////////////////////////////////
//
// Complete program (this compiles): use the program API
//
// g++ -Wall -I ..  -I../../common/ -I../../common/imgui/ -std=c++0x program.cpp ../../common/gl_core_4_5.c ../../common/imgui/imgui*.cpp -o program -lm -lSDL2 -lGL -lpthread -ldl
//


// LIBRARIES
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <algorithm>    // std::min_element, std::max_element

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "imgui.h"
#include "imgui_impl.h"


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


// SOURCE FILES
#include "quadtree.h"
#include "mesh.h"
#include "utility.h"

// MACROS
#define LOG(fmt, ...)  fprintf(stdout, fmt, ##__VA_ARGS__); fflush(stdout);
#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))


static const GLfloat black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
static const GLfloat grey[] = { 0.05f, 0.05f, 0.1f, 1.0f };
static const GLfloat one = 1.0;
static const mat4 IDENTITY = mat4(1.0);
static vec3 INIT_CAM_POS[2];
static vec3 INIT_CAM_LOOK[2];

struct CameraManager {
    vec3 pos;
    vec3 look;
    vec3 up;
    vec3 right;
    vec3 direction;
} cam = {};

struct Transforms {
    mat4 view;
    mat4 projection;
    mat4 inv_view;
} transforms = {};

struct OpenGLManager {
    QuadTree* quadtree;
    Mesh* mesh;
    //    djg_font* font;
    djg_clock* clock;
    QuadtreeSettings* qts;
    QuadtreeSettings qts_backup;
    uint mode;

    float last_t;
    float current_t;
    float delta_T;
    bool pause;
    int w_width, w_height, gui_width, gui_height;
} g_gl = {0};

struct BenchStats {
    double tcpu, tgpu;
    double avg_tcpu, avg_tgpu;
    double total_tcpu, total_tgpu;
    int frame, fps;
    double sec_timer;
    int last_frame;
} stat = {0};

void SetupTransfo(uint mode)
{
    cam.pos = INIT_CAM_POS[mode];
    cam.look = INIT_CAM_LOOK[mode];

    cam.up = vec3(0.0f, 0.0f, 1.0f);
    cam.direction = glm::normalize(cam.look - cam.pos);
    cam.look = cam.pos + cam.direction;

    cam.right = glm::normalize(glm::cross(cam.direction, cam.up));
    cam.up = -glm::normalize(glm::cross(cam.direction, cam.right));

    transforms.view = glm::lookAt(cam.pos, cam.look, cam.up);
    transforms.projection = glm::perspective(45.0f, 1.0f, 0.1f, 1024.0f);
    transforms.inv_view = glm::inverse(glm::transpose(transforms.view));
    g_gl.quadtree->UpdateTransforms(transforms.view, transforms.projection, cam.pos);

}

void PrintCamStuff()
{
    cout << "Position: " << glm::to_string(cam.pos) << endl;
    cout << "Look: " << glm::to_string(cam.look) << endl;
    cout << "Up: " << glm::to_string(cam.up) << endl;
    cout << "Right: " << glm::to_string(cam.right) << endl << endl;
}

void UpdateTransfo()
{
    transforms.view = glm::lookAt(cam.pos, cam.look, cam.up);
    transforms.inv_view = glm::inverse(glm::transpose(transforms.view));
    g_gl.quadtree->UpdateTransforms(transforms.view, transforms.projection, cam.pos);
}

void Init()
{

    g_gl.quadtree = new QuadTree();
    g_gl.mesh = new Mesh();
    g_gl.pause = false;
    g_gl.clock = djgc_create();
    g_gl.mode = TERRAIN;
    INIT_CAM_POS[TERRAIN]  = vec3(0.482968, 0.519043, 0.293363);
    INIT_CAM_LOOK[TERRAIN] = vec3(-0.138606, -0.193060, -0.033065);
    INIT_CAM_POS[MESH]  = vec3(1.062696, 1.331637, 0.531743);
    INIT_CAM_LOOK[MESH] =  vec3(0.456712, 0.599636, 0.220361);
    cout << " MESH: " << MESH << " TERRAIN " << TERRAIN << endl;

    g_gl.qts = g_gl.mesh->Init(g_gl.quadtree, g_gl.mode);
    SetupTransfo(g_gl.mode);

    //    if (g_gl.font) djgf_release(g_gl.font);
    //    g_gl.font = djgf_create(GL_TEXTURE0);

    stat.avg_tcpu = 0;
    stat.avg_tgpu = 0;
    stat.frame = 0;
    stat.total_tcpu = 0;
    stat.total_tgpu = 0;
    stat.sec_timer = 0;
    stat.fps = 0;
    stat.last_frame = 0;
}

void Cleanup()
{
    g_gl.mesh->CleanUp();

}

void UpdateTime()
{
    g_gl.current_t = glfwGetTime() * 0.001;
    g_gl.delta_T = g_gl.current_t - g_gl.last_t;
    g_gl.last_t = g_gl.current_t;
}

void UpdateStats(double cpu, double gpu)
{
    stat.frame++;
    stat.tcpu = cpu;
    stat.tgpu = gpu;
    stat.sec_timer += g_gl.delta_T;
    if(stat.sec_timer < 1.0) {
        stat.total_tcpu += cpu;
        stat.total_tgpu += gpu;
    } else {
        stat.fps = stat.frame - stat.last_frame;
        stat.last_frame = stat.frame;
        stat.avg_tcpu = stat.total_tcpu / double(stat.fps);
        stat.avg_tgpu = stat.total_tgpu / double(stat.fps);
        stat.sec_timer = 0.0;
        stat.total_tcpu = 0;
        stat.total_tgpu = 0;
    }
}


void ImGuiTime(string s, float tmp){
    ImGui::Text("%s:  %.5f %s\n", s.c_str(), (tmp < 1. ? tmp * 1e3 : tmp), (tmp < 1. ? "ms" : " s"));
}

void RenderImgui()
{
    ImGui::SetNextWindowPos(ImVec2(0, 0)/*, ImGuiSetCond_FirstUseEver*/);
    ImGui::SetNextWindowSize(ImVec2(g_gl.gui_width, g_gl.gui_height)/*, ImGuiSetCond_FirstUseEver*/);
    float max_lod = (g_gl.mode == TERRAIN) ? 100.0 : 2.0;
    ImGui::Begin("Window");
    {
        static int mode = 0, prim = 0, color_mode;
        if (ImGui::Combo("Mode", &mode, "Terrain\0Mesh\0\0")){
            g_gl.mode = mode;
            g_gl.mesh->setMode(mode);
            SetupTransfo(mode);
            cout << "mode = " << mode << endl;
        }
        if(ImGui::Checkbox("Height displace", &g_gl.qts->displace)){
            g_gl.quadtree->ReconfigureShaders();
        }
        if(ImGui::Checkbox("Render MVP", &g_gl.qts->renderMVP)){
            g_gl.quadtree->ReconfigureShaders();
        }
        if (ImGui::Combo("Root primitive", &prim, "Triangle\0Quad\0\0")){
            if(prim == 0) {
                g_gl.qts->prim_type = TRIANGLES;
            } else if(prim == 1) {
                g_gl.qts->prim_type = QUADS;
            }
            g_gl.quadtree->ReinitQuadTree();
        }
        if(ImGui::Checkbox("Uniform Subdivision", &g_gl.qts->uniform)){
            g_gl.quadtree->ReconfigureShaders();
        }
        if(ImGui::SliderInt("Uniform Level", &g_gl.qts->uni_lvl, 0, 20)){
            g_gl.quadtree->ReconfigureShaders();
        }

        if(ImGui::Checkbox("Morph vertices", &g_gl.qts->morph)){
            if(g_gl.qts->cpu_lod < 2)
                g_gl.qts->morph = false;
            g_gl.quadtree->ReconfigureShaders();
        }
        if(ImGui::SliderFloat("LoD Factor", &g_gl.qts->adaptive_factor, 0.01, max_lod)){
            g_gl.quadtree->ReconfigureShaders();
        }
        if(ImGui::SliderInt("CPU LoD", &g_gl.qts->cpu_lod, 0, 5)){
            if(g_gl.qts->cpu_lod < 2)
                g_gl.qts->morph = false;
            g_gl.quadtree->ReconfigureShaders();
            g_gl.quadtree->ReloadPrimitives();
        }
        if(ImGui::Checkbox("Readback primitive count", &g_gl.qts->map_primcount)){
        }
        if(g_gl.qts->map_primcount){
            ImGui::SameLine();
            ImGui::Value("", g_gl.quadtree->GetPrimcount());
        }
        if(ImGui::Checkbox("Debug morphing", &g_gl.qts->debug_morph)){
            if(g_gl.qts->debug_morph){
                g_gl.qts_backup = *(g_gl.qts);
                g_gl.qts->uniform = true;
                g_gl.qts->uni_lvl = 1;
                g_gl.qts->morph = true;
            } else {
                g_gl.qts_backup.debug_morph = false;
                *(g_gl.qts) = g_gl.qts_backup;
            }
            g_gl.quadtree->ReconfigureShaders();

        }
        ImGui::SameLine();
        if(ImGui::SliderFloat("K", &g_gl.qts->morph_k, 0.0, 1.0)){
            g_gl.quadtree->ReconfigureShaders();
        }
        if (ImGui::Combo("Color mode", &g_gl.qts->color_mode, "LoD\0White Wireframe\0Primitive Highlight\0\0")){
            g_gl.quadtree->UpdateColorMode();
        }
        if(ImGui::Button("Reinitialize QuadTree")){
            g_gl.quadtree->ReinitQuadTree();
        }
        ImGui::SameLine();
        if(ImGui::Checkbox("Freeze", &g_gl.qts->freeze)){
            g_gl.quadtree->ReconfigureShaders();
        }
        if (ImGui::Button("Take Screenshot")) {
            static int cnt = 0;
            char buf[1024];

            snprintf(buf, 1024, "screenshot%03i", cnt);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
            djgt_save_glcolorbuffer_bmp(GL_FRONT, GL_RGBA, buf);
            ++cnt;
        }
        ImGui::Text("Frame  %07i\n", stat.frame);
        ImGui::Text("FPS    %07i\n", stat.fps);
        ImGuiTime("deltaT", g_gl.delta_T);
        ImGui::Text("\nQuadtree Perf:");
        ImGuiTime("CPU dTime      ", stat.tcpu);
        ImGuiTime("GPU dTime      ", stat.tgpu);
        ImGuiTime("avg CPU dT (1s)", stat.avg_tcpu);
        ImGuiTime("avg GPU dT (1s)", stat.avg_tgpu);
        ImGui::Text("\n\n");


        static float values_gpu[90] = { 0 };
        static float values_cpu[90] = { 0 };
        static float values_fps[90] = { 0 };
        static float values_primcount[90] = { 0 };
        static int offset = 0;
        static float refresh_time = 0.0f;
        static float max_cpu = 0.0, max_gpu = 0, tmp;
        static float max_primcount = 0.0;
        if (refresh_time == 0.0f)
            refresh_time = ImGui::GetTime();

        while (refresh_time < ImGui::GetTime())
        {
            values_gpu[offset] = stat.tgpu * 1000.0;
            values_cpu[offset] = stat.tcpu * 1000.0;
            values_fps[offset] = ImGui::GetIO().Framerate;
            values_primcount[offset] = g_gl.quadtree->GetPrimcount();

            offset = (offset+1) % IM_ARRAYSIZE(values_gpu);
            refresh_time += 1.0f/30.0f;
        }
        tmp = *std::max_element(values_cpu, values_cpu+90);
        if(tmp > max_gpu || tmp < 0.2 * max_gpu)
            max_gpu = tmp;
        ImGui::PlotLines("CPU dT", values_cpu, IM_ARRAYSIZE(values_cpu), offset,
                         std::to_string(stat.tgpu * 1000.0).c_str(), 0.0f, max_gpu, ImVec2(0,80));

        tmp = *std::max_element(values_gpu, values_gpu+90);
        if(tmp > max_cpu || tmp < 0.2 * max_cpu)
            max_cpu = tmp;
        ImGui::PlotLines("GPU dT", values_gpu, IM_ARRAYSIZE(values_gpu), offset,
                         std::to_string(stat.tgpu * 1000.0).c_str(), 0.0f, max_cpu, ImVec2(0,80));


        ImGui::PlotLines("FPS", values_fps, IM_ARRAYSIZE(values_fps), offset,
                         std::to_string(ImGui::GetIO().Framerate).c_str(), 0.0f, 1000, ImVec2(0,80));

        if(g_gl.qts->map_primcount){
            tmp = *std::max_element(values_primcount, values_primcount+90);
            if(tmp > max_primcount || tmp < 0.2 * max_primcount)
                max_primcount = tmp;
            ImGui::PlotLines("PrimCount", values_primcount, IM_ARRAYSIZE(values_primcount), offset,
                             "", 0.0f, max_primcount, ImVec2(0,80));
        }
    }
    ImGui::End();

    ImGui::Render();
}

void Draw()
{
    UpdateTime();
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    g_gl.mesh->Draw(g_gl.delta_T, g_gl.qts->freeze);
//    glClearBufferfv(GL_COLOR, 0, grey);
//    glViewport(0, 0, g_gl.w_width, g_gl.w_height);

    double tcpu, tgpu;
    g_gl.quadtree->GetTicks(tcpu, tgpu);
    UpdateStats(tcpu, tgpu);

    RenderImgui();

}

//void HandleEvent(const SDL_Event *event)
//{
//    const static float mouse_factor = 8e-4;
//    ImGuiIO& io = ImGui::GetIO();
//    if (io.WantCaptureKeyboard || io.WantCaptureMouse)
//        return;
//    //Event handling
//    switch (event->type)
//    {
//    case SDL_KEYDOWN:
//        switch(event->key.keysym.sym)
//        {
//        case SDLK_r:
//            g_gl.quadtree->ReloadShaders();
//            break;
//        case SDLK_u:
//            g_gl.quadtree->ReconfigureShaders();
//        case SDLK_p:
//            PrintCamStuff();
//            break;
//        case SDLK_SPACE:
//            g_gl.pause = !g_gl.pause;
//            break;
//        default:
//            break;
//        }
//    case SDL_MOUSEMOTION: {
//        int x, y;
//        unsigned int button= SDL_GetRelativeMouseState(&x, &y);
//        //const Uint8 *state = SDL_GetKeyboardState(NULL);

//        if (button & SDL_BUTTON(SDL_BUTTON_LEFT)) {
//            mat4 h_rotation = glm::rotate(IDENTITY, float(x * mouse_factor), cam.up);
//            mat4 v_rotation = glm::rotate(IDENTITY, float(y * mouse_factor), cam.right);
//            cam.direction = glm::normalize(mat3(h_rotation) * mat3(v_rotation) * cam.direction);
//            cam.right     = glm::normalize(mat3(h_rotation) * mat3(v_rotation) * cam.right);
//            cam.up = -glm::normalize(glm::cross(cam.direction, cam.right));
//            cam.look = cam.pos + cam.direction;
//            UpdateTransfo();
//        } else if (button & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
//            cam.pos += -cam.right * vec3(x * mouse_factor) + cam.up * vec3(y * mouse_factor);
//            cam.look = cam.pos + cam.direction;
//            UpdateTransfo();

//        }
//    } break;
//    case SDL_MOUSEWHEEL: {
//        vec3 forward = vec3(event->wheel.y * 0.05) * cam.direction ;
//        cam.pos += forward;
//        cam.look = cam.pos + cam.direction;
//        UpdateTransfo();
//    } break;
//    default:
//        break;
//    }
//}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action,
                      int modsls)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard)
        return;
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;
}

void mouseMotionCallback(GLFWwindow* window, double x, double y)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;
}

void mouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;
}

int main(int argc, char **argv)
{
    g_gl.w_width = 1024;
    g_gl.w_height = 1024;
    g_gl.gui_width = 512;
    g_gl.gui_height = g_gl.w_height;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifndef NDEBUG
    glfwWindowHint (GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

    // Create the Window
    LOG("Loading {Window-Main}\n");
    GLFWwindow* window = glfwCreateWindow((g_gl.w_width + g_gl.gui_width), g_gl.w_height, "Hello Imgui", NULL, NULL);
    if (window == NULL) {
        LOG("=> Failure <=\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, &keyboardCallback);
    glfwSetCursorPosCallback(window, &mouseMotionCallback);
    glfwSetMouseButtonCallback(window, &mouseButtonCallback);
    glfwSetScrollCallback(window, &mouseScrollCallback);

    // Load OpenGL functions
    LOG("Loading {OpenGL}\n");
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        LOG("gladLoadGLLoader failed\n");
        return -1;
    }
#ifndef NDEBUG
    log_debug_output();
#endif

    glClearColor(1,1,1,1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    LOG("-- Begin -- Demo\n");



    try {
        Init();
        ImGui::CreateContext();
        ImGui_ImplGlfwGL3_Init(window, false);
        ImGui::StyleColorsDark();
        bool running = true;

        while(!glfwWindowShouldClose(window) && running)
        {

            glfwPollEvents();
            ImGui_ImplGlfwGL3_NewFrame();
            Draw();
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
}
//////////////////////////////////////////////////////////////////////////////
