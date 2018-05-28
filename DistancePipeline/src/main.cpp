
// LIBRARIES
#include <iostream>
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

////////////////////////////////////////////////////////////////////////////////
///
/// Struct definitions
///

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
    djg_clock* clock;
    QuadtreeSettings* qts;
    QuadtreeSettings qts_backup;
    uint mode;

    float last_t;
    float current_t;
    float delta_T;
    bool pause;
    int w_width, w_height, gui_width, gui_height;

    double x0, y0;
    bool lbutton_down, rbutton_down;
} gl = {0};

struct BenchStats {
    double tcpu, tgpu;
    double avg_tcpu, avg_tgpu;
    double total_tcpu, total_tgpu;
    int frame, fps;
    double sec_timer;
    int last_frame;
} stat = {0};

////////////////////////////////////////////////////////////////////////////////
///
/// Camera and Transforms management
///

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
    gl.quadtree->UpdateTransforms(transforms.view, transforms.projection, cam.pos);

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
    gl.quadtree->UpdateTransforms(transforms.view, transforms.projection, cam.pos);
}

////////////////////////////////////////////////////////////////////////////////
///
/// Stats and Benching
///

void UpdateTime()
{
    gl.current_t = glfwGetTime() * 0.001;
    gl.delta_T = gl.current_t - gl.last_t;
    gl.last_t = gl.current_t;
}

void UpdateStats(double cpu, double gpu)
{
    stat.frame++;
    stat.tcpu = cpu;
    stat.tgpu = gpu;
    stat.sec_timer += gl.delta_T;
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

////////////////////////////////////////////////////////////////////////////////
///
/// GUI Render
///

void ImGuiTime(string s, float tmp){
    ImGui::Text("%s:  %.5f %s\n", s.c_str(), (tmp < 1. ? tmp * 1e3 : tmp), (tmp < 1. ? "ms" : " s"));
}

void RenderImgui()
{
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(gl.gui_width, gl.gui_height));
    float max_lod = (gl.mode == TERRAIN) ? 100.0 : 2.0;

    ImGui::Begin("Window");
    {
        static int mode = 0, prim = 0;
        if (ImGui::Combo("Mode", &mode, "Terrain\0Mesh\0\0")){
            gl.mode = mode;
            gl.mesh->setMode(mode);
            SetupTransfo(mode);
            cout << "mode = " << mode << endl;
        }
        if(ImGui::Checkbox("Height displace", &gl.qts->displace)){
            gl.quadtree->ReconfigureShaders();
        }
        if(ImGui::Checkbox("Render MVP", &gl.qts->renderMVP)){
            gl.quadtree->ReconfigureShaders();
        }
        if (ImGui::Combo("Root primitive", &prim, "Triangle\0Quad\0\0")){
            if(prim == 0) {
                gl.qts->prim_type = TRIANGLES;
            } else if(prim == 1) {
                gl.qts->prim_type = QUADS;
            }
            gl.quadtree->ReinitQuadTree();
        }
        if(ImGui::Checkbox("Uniform Subdivision", &gl.qts->uniform)){
            gl.quadtree->ReconfigureShaders();
        }
        if(ImGui::SliderInt("Uniform Level", &gl.qts->uni_lvl, 0, 20)){
            gl.quadtree->ReconfigureShaders();
        }

        if(ImGui::Checkbox("Morph vertices", &gl.qts->morph)){
            if(gl.qts->cpu_lod < 2)
                gl.qts->morph = false;
            gl.quadtree->ReconfigureShaders();
        }
        if(ImGui::SliderFloat("LoD Factor", &gl.qts->adaptive_factor,
                              0.01, max_lod)){
            gl.quadtree->ReconfigureShaders();
        }
        if(ImGui::SliderInt("CPU LoD", &gl.qts->cpu_lod, 0, 5)){
            if(gl.qts->cpu_lod < 2)
                gl.qts->morph = false;
            gl.quadtree->ReconfigureShaders();
            gl.quadtree->ReloadPrimitives();
        }
        ImGui::Checkbox("Readback primitive count", &gl.qts->map_primcount);
        if(gl.qts->map_primcount){
            ImGui::SameLine();
            ImGui::Value("", gl.quadtree->GetPrimcount());
        }
        if(ImGui::Checkbox("Debug morphing", &gl.qts->debug_morph)){
            if(gl.qts->debug_morph){
                gl.qts_backup = *(gl.qts);
                gl.qts->uniform = true;
                gl.qts->uni_lvl = 1;
                gl.qts->morph = true;
            } else {
                gl.qts_backup.debug_morph = false;
                *(gl.qts) = gl.qts_backup;
            }
            gl.quadtree->ReconfigureShaders();

        }
        ImGui::SameLine();
        if(ImGui::SliderFloat("K", &gl.qts->morph_k, 0.0, 1.0)){
            gl.quadtree->ReconfigureShaders();
        }
        if (ImGui::Combo("Color mode", &gl.qts->color_mode,
                         "LoD\0White Wireframe\0Primitive Highlight\0\0")){
            gl.quadtree->UpdateColorMode();
        }
        if(ImGui::Button("Reinitialize QuadTree")){
            gl.quadtree->ReinitQuadTree();
        }
        ImGui::SameLine();
        if(ImGui::Checkbox("Freeze", &gl.qts->freeze)){
            gl.quadtree->ReconfigureShaders();
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
        ImGuiTime("deltaT", gl.delta_T);
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
            values_primcount[offset] = gl.quadtree->GetPrimcount();

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

        if(gl.qts->map_primcount){
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

////////////////////////////////////////////////////////////////////////////////
///
/// Input Callbacks
///


void keyboardCallback(GLFWwindow* window, int key, int scancode, int action,
                      int modsls)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard)
        return;

    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GL_TRUE);
            break;
        case GLFW_KEY_R:
            gl.quadtree->ReloadShaders();
            break;
        case GLFW_KEY_U:
            gl.quadtree->ReconfigureShaders();
            break;
        case GLFW_KEY_P:
            PrintCamStuff();
            break;
        case GLFW_KEY_SPACE:
            gl.pause = !gl.pause;
            break;
        default:
            break;
        }
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;


    if (GLFW_PRESS == action) {

        gl.lbutton_down = (button == GLFW_MOUSE_BUTTON_LEFT);
        gl.rbutton_down = (button == GLFW_MOUSE_BUTTON_RIGHT);
        glfwGetCursorPos(window, &gl.x0, &gl.y0);
        gl.x0 -= gl.gui_width;
    }  else if(GLFW_RELEASE == action) {
        gl.rbutton_down = false;
        gl.lbutton_down = false;
    }

}

void mouseMotionCallback(GLFWwindow* window, double x, double y)
{

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;
    x = x - gl.gui_width;

    if (gl.lbutton_down)
    {
        double dx, dy;
        dx = x - gl.x0;
        dy = y - gl.y0;
        dx /= gl.w_width;
        dy /= gl.w_height;

        mat4 h_rotation = glm::rotate(IDENTITY, float(dx), cam.up);
        mat4 v_rotation = glm::rotate(IDENTITY, float(dy), cam.right);
        cam.direction = glm::normalize(mat3(h_rotation) * mat3(v_rotation) * cam.direction);
        cam.right     = glm::normalize(mat3(h_rotation) * mat3(v_rotation) * cam.right);
        cam.up = -glm::normalize(glm::cross(cam.direction, cam.right));
        cam.look = cam.pos + cam.direction;
        UpdateTransfo();

        gl.x0 = x;
        gl.y0 = y;
    }

    if (gl.rbutton_down)
    {
        double dx, dy;
        dx = x - gl.x0;
        dy = y - gl.y0;
        dx /= gl.w_width;
        dy /= gl.w_height;

        cam.pos += -cam.right * vec3(dx) + cam.up * vec3(dy);
        cam.look = cam.pos + cam.direction;
        UpdateTransfo();

        gl.x0 = x;
        gl.y0 = y;
    }
}

void mouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;

    vec3 forward = vec3(yoffset * 0.05) * cam.direction ;
    cam.pos += forward;
    cam.look = cam.pos + cam.direction;
    UpdateTransfo();
}

////////////////////////////////////////////////////////////////////////////////
///
/// The Main Program
///

void Draw()
{
    UpdateTime();
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl.mesh->Draw(gl.delta_T, gl.qts->freeze);

    double tcpu, tgpu;
    gl.quadtree->GetTicks(tcpu, tgpu);
    UpdateStats(tcpu, tgpu);

    RenderImgui();

}

void Init()
{

    gl.quadtree = new QuadTree();
    gl.mesh = new Mesh();
    gl.pause = false;
    gl.clock = djgc_create();
    gl.mode = TERRAIN;
    INIT_CAM_POS[TERRAIN]  = vec3(0.482968, 0.519043, 0.293363);
    INIT_CAM_LOOK[TERRAIN] = vec3(-0.138606, -0.193060, -0.033065);
    INIT_CAM_POS[MESH]  = vec3(1.062696, 1.331637, 0.531743);
    INIT_CAM_LOOK[MESH] =  vec3(0.456712, 0.599636, 0.220361);
    cout << " MESH: " << MESH << " TERRAIN " << TERRAIN << endl;

    gl.qts = gl.mesh->Init(gl.quadtree, gl.mode);
    SetupTransfo(gl.mode);

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
    gl.mesh->CleanUp();

}

int main(int argc, char **argv)
{
    gl.w_width = 1024;
    gl.w_height = 1024;
    gl.gui_width = 512;
    gl.gui_height = gl.w_height;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifndef NDEBUG
    glfwWindowHint (GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

    // Create the Window
    LOG("Loading {Window-Main}\n");
    GLFWwindow* window = glfwCreateWindow((gl.w_width + gl.gui_width), gl.w_height, "Hello Imgui", NULL, NULL);
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
