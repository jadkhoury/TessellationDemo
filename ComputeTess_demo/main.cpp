#include <cmath>
// SOURCE FILES
#include "common.h"
#include "bintree.h"
#include "mesh_utils.h"
#include "mesh.h"

// MACROS
#define LOG(fmt, ...)  fprintf(stdout, fmt, ##__VA_ARGS__); fflush(stdout);
#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))

////////////////////////////////////////////////////////////////////////////////
///
/// Const and Structs
///
static const GLfloat black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
static const GLfloat grey[] = { 0.05f, 0.05f, 0.1f, 1.0f };
static const GLfloat one = 1.0;
static const mat4 IDENTITY = mat4(1.0f);

struct OpenGLApp {
    int gui_width, gui_height;

    bool lbutton_down, rbutton_down;
    double x0, y0;

    uint mode;
    string filepath;
    const string default_filepath = "bigguy.obj";

    bool auto_lod;
    float light_pos[3] = {50,-50,100};

    Mesh mesh;
    CameraManager cam;
} app = {};

struct BenchStats {
    double last_t;
    double current_t;
    double delta_T;

    double avg_qt_gpu_compute, avg_qt_gpu_render;
    double  total_qt_gpu_compute, total_qt_gpu_render;
    double  avg_frame_dt, total_frame_dt;

    int frame_count, real_fps;
    double sec_timer;
    int last_frame_count;

    void Init();
    void UpdateTime();
    void UpdateStats();
} bench = {};


////////////////////////////////////////////////////////////////////////////////
///
/// Update render paramenters
///
///

void updateRenderParams()
{
    vec3 l(app.light_pos[0], app.light_pos[1], app.light_pos[2]);
    app.mesh.bintree->UpdateLightPos(l);
    app.mesh.bintree->UpdateMode(app.mode);
    app.mesh.bintree->UpdateScreenRes(std::max(app.cam.fb_height, app.cam.fb_width));
}

////////////////////////////////////////////////////////////////////////////////
///
/// Benchmarking Functions
///

void BenchStats::Init()
{
    current_t = glfwGetTime() * 0.001f;
    last_t = current_t;
    delta_T = 0;

    avg_qt_gpu_compute = 0;
    avg_qt_gpu_render = 0;
    avg_frame_dt = 0;
    frame_count = 0;
    total_qt_gpu_compute = 0;
    total_qt_gpu_render = 0;
    total_frame_dt = 0;
    sec_timer = 0;
    real_fps = 0;
    last_frame_count = 0;
}

void BenchStats::UpdateTime()
{
    current_t = glfwGetTime();
    delta_T = current_t - last_t;
    last_t = current_t;
}

void BenchStats::UpdateStats()
{
    frame_count++;
    sec_timer += delta_T;
    if (sec_timer < 1.0f) {
        total_qt_gpu_compute += app.mesh.bintree->ticks.gpu_compute;
        total_qt_gpu_render += app.mesh.bintree->ticks.gpu_render;
        total_frame_dt += delta_T;
    } else {
        real_fps = frame_count - last_frame_count;
        last_frame_count = frame_count;
        avg_qt_gpu_compute = total_qt_gpu_compute / double(real_fps);
        avg_qt_gpu_render = total_qt_gpu_render / double(real_fps);
        avg_frame_dt = total_frame_dt / double(real_fps);
        total_qt_gpu_compute = 0;
        total_qt_gpu_render = 0;
        total_frame_dt = 0;
        sec_timer = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////
///
/// GUI Functions
///

// Small trick for better timing output
void ImGuiTime(string s, float tmp)
{
    ImGui::Text("%s:  %.5f %s\n",
                s.c_str(),
                (tmp < 1. ? tmp * 1e3 : tmp),
                (tmp < 1. ? " ms" : " s"));
}

void RenderImgui()
{

    BinTree::Settings& set = app.mesh.bintree->settings;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(app.gui_width, app.gui_height));
    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoResize;
    window_flags |= ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::Begin("Benchmark and Controls", nullptr, window_flags);
    {
        static float values_compute[80] = { 0 };
        static float values_render[80]  = { 0 };
        static float values_frame_dt[80] = { 0 };
        static float values_fps[80] = { 0 };

        static int offset = 0;
        static float refresh_time = 0;
        static float max_gpu_compute = 0, max_gpu_render = 0;
        static float max_fps = 0, max_dt = 0;

        static float array_max, current_val;

        if (refresh_time == 0)
            refresh_time = ImGui::GetTime();

        while (refresh_time < ImGui::GetTime())
        {
            values_compute[offset] = app.mesh.bintree->ticks.gpu_compute * 1000;
            values_render[offset]  = app.mesh.bintree->ticks.gpu_render  * 1000;
            values_frame_dt[offset] = bench.delta_T * 1000;
            values_fps[offset] = ImGui::GetIO().Framerate;

            offset = (offset+1) % IM_ARRAYSIZE(values_compute);
            refresh_time += 1.0f/30.0f;
        }

        // BINTREE COMPUTE DT
        current_val = app.mesh.bintree->ticks.gpu_compute * 1000;
        array_max = *std::max_element(values_compute, values_compute+80);
        if (array_max > max_gpu_compute || array_max < 0.2 * max_gpu_compute)
            max_gpu_compute = array_max;
        ImGui::PlotLines("GPU compute dT", values_compute,
                         IM_ARRAYSIZE(values_compute), offset,
                         std::to_string(current_val).c_str(),
                         0.0f, max_gpu_compute, ImVec2(0,80));

        // BINTREE RENDER DT
        current_val = app.mesh.bintree->ticks.gpu_render * 1000;
        array_max = *std::max_element(values_render, values_render+80);
        if (array_max > max_gpu_render || array_max < 0.2 * max_gpu_render)
            max_gpu_render = array_max;
        ImGui::PlotLines("GPU render dT", values_render,
                         IM_ARRAYSIZE(values_render), offset,
                         std::to_string(current_val).c_str(),
                         0.0f, max_gpu_render, ImVec2(0,80));

        // FPS
        current_val = 1.0 / bench.delta_T;
        array_max = *std::max_element(values_fps, values_fps+80);
        if (array_max > max_fps || array_max < 0.2 * max_fps) max_fps = array_max;
        ImGui::PlotLines("FPS", values_fps,
                         IM_ARRAYSIZE(values_fps), offset,
                         std::to_string(current_val).c_str(),
                         0.0f, max_fps, ImVec2(0,80));

        // dT
        current_val = bench.delta_T * 1000;
        array_max = *std::max_element(values_frame_dt, values_frame_dt+80);
        if (array_max > max_dt || array_max < 0.2 * max_dt) max_dt = array_max;
        ImGui::PlotLines("Frame dT", values_frame_dt,
                         IM_ARRAYSIZE(values_frame_dt), offset,
                         std::to_string(current_val).c_str(),
                         0.0f, max_dt, ImVec2(0,80));

        ImGui::Text("\nOutput FPS (1s) %d", bench.real_fps);
        ImGuiTime("avg GPU Compute dT (1s)", bench.avg_qt_gpu_compute);
        ImGuiTime("avg GPU Render  dT (1s)", bench.avg_qt_gpu_render);
        ImGuiTime("avg Frame dT (1s)      ", bench.avg_frame_dt);
        ImGui::Text("\n");

        if (ImGui::Combo("Mode", (int*)&app.mode, "Terrain\0Mesh\0\0")) {
            app.mesh.CleanUp();
            app.cam.Init(app.mode);
            app.mesh.Init(app.mode, app.cam, app.filepath);
            updateRenderParams();
            app.mesh.bintree->UpdateLodFactor(app.cam.fb_width, app.cam.fov);
            app.mesh.bintree->UploadSettings();
        }
        ImGui::Text("\n");

        static bool advanced = false;
        if (ImGui::Checkbox("Advanced Mode", &advanced))
            set.map_nodecount = advanced;
        if (advanced)
        {
            ImGui::Text("\n------ Renderer Settings ------\n");
            if (ImGui::Checkbox("Render Projection", &set.MVP_on)) {
                app.mesh.bintree->UploadSettings();
            }
            if (ImGui::SliderFloat("FOV", &app.cam.fov, 5, 90)) {
                app.mesh.bintree->UpdateLodFactor(app.cam.fb_width, app.cam.fov);
                app.mesh.UpdateForFOV(app.cam);
                app.mesh.bintree->UploadSettings();
            }
            if (ImGui::Button("Reinit Camera")) {
                app.cam.Init(app.mode);
                app.mesh.InitTransforms(app.cam);
                app.mesh.bintree->UpdateLodFactor(app.cam.fb_width, app.cam.fov);
                app.mesh.bintree->UploadSettings();
            }

            if (ImGui::Checkbox("Wireframe", &set.wireframe_on)) {
                app.mesh.bintree->ReloadRenderProgram();
                updateRenderParams();
            }
            if(!set.wireframe_on){
            ImGui::SameLine();
            if (ImGui::Checkbox("Flat Normals", &set.flat_normal)) {
                app.mesh.bintree->ReloadRenderProgram();
                updateRenderParams();
            } }
            if (ImGui::Combo("Color mode", &set.color_mode,
                             "LoD & Morph\0White Wireframe\0Polygone Highlight\0Frustum\0Cull\0Debug\0\0")) {
                app.mesh.bintree->UploadSettings();
            }
            if(ImGui::DragFloat3("Light pos", app.light_pos, 0.1f)) {
                vec3 l(app.light_pos[0], app.light_pos[1], app.light_pos[2]);
                app.mesh.bintree->UpdateLightPos(l);
            }

            ImGui::Text("\n------ Mesh Settings ------\n");

            if (app.mode == TERRAIN){
                if (ImGui::Checkbox("Displacement Mapping", &set.displace_on)) {
                    app.mesh.bintree->ReloadShaders();
                    app.mesh.bintree->UploadSettings();
                    updateRenderParams();
                }
            }
            if(set.displace_on){
                if (ImGui::SliderFloat("Height Factor", &set.displace_factor, 0, 2)) {
                    app.mesh.bintree->UploadSettings();
                }
            }
            if (ImGui::Checkbox("Rotate Mesh", &set.rotateMesh)) {
                app.mesh.bintree->UploadSettings();
            }
            if (ImGui::Checkbox("Uniform", &set.uniform_on)) {
                app.mesh.bintree->UploadSettings();
            }
            ImGui::SameLine();
            if (ImGui::SliderInt("", &set.uniform_lvl, 0, 20)) {
                app.mesh.bintree->UploadSettings();
            }
            ImGui::Checkbox("Auto LoD", &app.auto_lod);
            float expo = log2(set.target_length);
            if (ImGui::SliderFloat("Edge Length (2^x)", &expo, 1.0f, 10.0f)) {
                set.target_length = std::pow(2.0f, expo);
                app.mesh.bintree->UpdateLodFactor(app.cam.fb_width, app.cam.fov);
                app.mesh.bintree->UploadSettings();
            }
            if (ImGui::Checkbox("Readback node count", &set.map_nodecount)) {
                app.mesh.bintree->UploadSettings();
            }
            if (set.map_nodecount) {
                int leaf_tri = (1<<(set.cpu_lod*2));
                ImGui::Text("Total    : "); ImGui::SameLine();
                ImGui::Text("%s", utility::LongToString(
                                app.mesh.bintree->full_node_count).c_str());
                ImGui::Text("Drawn    : "); ImGui::SameLine();
                ImGui::Text("%s", utility::LongToString(
                                app.mesh.bintree->drawn_node_count).c_str());
                ImGui::Text("Triangles: "); ImGui::SameLine();
                ImGui::Text("%s", utility::LongToString(
                                app.mesh.bintree->drawn_node_count*leaf_tri).c_str());
            }
            if (ImGui::Combo("Polygon type", &set.polygon_type, "Triangle\0Quad\0\0")) {
                app.mesh.LoadMeshBuffers();
                app.mesh.bintree->Reinitialize();
                updateRenderParams();
            }
            if (ImGui::SliderInt("CPU LoD", &set.cpu_lod, 0, 4)) {
                app.mesh.bintree->Reinitialize();
                app.mesh.bintree->UpdateLodFactor(app.cam.fb_width, app.cam.fov);
                app.mesh.bintree->UploadSettings();
                updateRenderParams();

            }
            if (ImGui::Checkbox("Cull", &set.cull_on)) {
                app.mesh.bintree->ReloadComputeProgram();
                app.mesh.bintree->UploadSettings();
                updateRenderParams();
            }
            ImGui::SameLine();
            if (ImGui::Checkbox("Freeze", &set.freeze)) {
                app.mesh.bintree->ReconfigureShaders();
            }
            ImGui::SameLine();
            if (ImGui::Button("Reinitialize Bintree")) {
                app.mesh.bintree->Reinitialize();
                updateRenderParams();
            }
            if (app.mode == MESH) {
                if (ImGui::Combo("Interpolation type", &set.itpl_type,
                                 "Linear\0PN Triangles\0Phong\0\0\0")) {
                    app.mesh.bintree->ReloadRenderProgram();
                    updateRenderParams();
                }
                if (ImGui::SliderFloat("alpha", &set.itpl_alpha, 0, 1)) {
                    app.mesh.bintree->UploadSettings();
                }
            }
            if (app.mesh.bintree->capped) {
                ImGui::Text(" LOD FACTOR CAPPED \n");
            }
        }
    }  ImGui::End();

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
            app.mesh.bintree->ReloadShaders();
            updateRenderParams();
            break;
        case GLFW_KEY_U:
            app.mesh.bintree->ReconfigureShaders();
            break;
        case GLFW_KEY_P:
            app.cam.PrintStatus();
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
        app.lbutton_down = (button == GLFW_MOUSE_BUTTON_LEFT);
        app.rbutton_down = (button == GLFW_MOUSE_BUTTON_RIGHT);
        glfwGetCursorPos(window, &app.x0, &app.y0);
        app.x0 -= app.gui_width;
    }  else if(GLFW_RELEASE == action) {
        app.rbutton_down = false;
        app.lbutton_down = false;
    }

}

void mouseMotionCallback(GLFWwindow* window, double x, double y)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;
    x = x - app.gui_width;

    if (app.lbutton_down || app.rbutton_down)
    {
        double dx, dy;
        dx = (x - app.x0) / app.cam.fb_width;
        dy = (y - app.y0) / app.cam.fb_height;

        if (app.lbutton_down)
            app.cam.ProcessMouseLeft(dx, dy);
        if (app.rbutton_down)
            app.cam.ProcessMouseRight(dx, dy);

        app.mesh.UpdateForView(app.cam);

        app.x0 = x;
        app.y0 = y;
    }
}

void mouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;

    app.cam.ProcessMouseScroll(yoffset);
    app.mesh.UpdateForView(app.cam);
}

void resizeCallback(GLFWwindow* window, int new_width, int new_height) {
    app.cam.fb_width  = new_width - app.gui_width;
    app.cam.fb_height = new_height;
    app.gui_height = new_height;
    app.mesh.bintree->UpdateScreenRes(std::max(app.cam.fb_height, app.cam.fb_width));
    app.mesh.UpdateForSize(app.cam);
    app.mesh.bintree->UpdateLodFactor(app.cam.fb_width, app.cam.fov);
    app.mesh.bintree->UploadSettings();
}

////////////////////////////////////////////////////////////////////////////////
///
/// The Program
///

void Init()
{
    cout << "******************************************************" << endl;
    cout << "INITIALIZATION" << endl;

    app.auto_lod = false;

    app.mode = TERRAIN;
    if(app.filepath != app.default_filepath)
        app.mode = MESH;

    app.cam.Init(app.mode);
    app.mesh.Init(app.mode, app.cam, app.filepath);
    bench.Init();
    updateRenderParams();

    app.mesh.bintree->UpdateLodFactor(app.cam.fb_width, app.cam.fov);
    app.mesh.bintree->UploadSettings();

    cout << "END OF INITIALIZATION" << endl;
    cout << "******************************************************" << endl << endl;

}

void Draw()
{

    glViewport(app.gui_width, 0, app.cam.fb_width, app.cam.fb_height);
    app.mesh.Draw(bench.delta_T, app.mode);
    glViewport(0, 0, app.cam.fb_width + app.gui_width, app.cam.fb_height);
    bench.UpdateStats();
    RenderImgui();

    if (app.auto_lod && !app.mesh.bintree->settings.uniform_on) {
        float& target = app.mesh.bintree->settings.target_length;
        static float upperFPS = 75.0f, lowerFPS = 60.0f;
        if (bench.delta_T < 1.0/upperFPS) {
            target *= 0.99;
            app.mesh.bintree->UpdateLodFactor(app.cam.fb_width, app.cam.fov);
            app.mesh.bintree->UploadSettings();
        } else if (bench.delta_T > 1.0/lowerFPS){
            target *= 1.01;
            app.mesh.bintree->UpdateLodFactor(app.cam.fb_width, app.cam.fov);
            app.mesh.bintree->UploadSettings();
        }
    }
    bench.UpdateTime();
}

void Cleanup() {
    app.mesh.CleanUp();
}

void HandleArguments(int argc, char **argv)
{
    if (argc == 1) {
        app.filepath = app.default_filepath;
        cout << "Using default mesh: " << app.default_filepath << endl;
    } else {
        if (argc > 2)
            cout << "Only takes in 1 obj file name, ignoring other arguments" << endl;
        string file = argv[1];
        cout << "Trying to open " << file << " ... ";
        ifstream f(file.c_str());
        if (f.good()) {
            cout << "OK" << endl;
            app.filepath = file;
        } else {
            app.filepath = app.default_filepath;
            cout << "failure, keeping default mesh " << app.filepath << endl;
        }
    }
}

int main(int argc, char **argv)
{
    HandleArguments(argc, argv);

    app.cam.fb_width = 1024;
    app.cam.fb_height = 1024;
    app.gui_width = 352;
    app.gui_height = app.cam.fb_height;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifndef NDEBUG
    glfwWindowHint (GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

    // Create the Window
    LOG("Loading {Window-Main}\n");
    GLFWwindow* window = glfwCreateWindow((app.cam.fb_width + app.gui_width),
                                          app.cam.fb_height,
                                          "Distance Based Tessellation",
                                          NULL, NULL);
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
    glfwSetFramebufferSizeCallback(window, resizeCallback);

    // Load OpenGL functions
    LOG("Loading {OpenGL}\n");
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        LOG("gladLoadGLLoader failed\n");
        return -1;
    }
#ifndef NDEBUG
    log_debug_output();
#endif

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
    glDebugMessageCallback((GLDEBUGPROC)debug_output_logger, NULL);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    try {
        Init();
        ImGui::CreateContext();
        ImGui_ImplGlfwGL3_Init(window, false);
        ImGui::StyleColorsDark();

        // delta_T condition to avoid crashing my system
        while (!glfwWindowShouldClose(window)  && bench.delta_T < 5.0)
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

    return 0;
}
//////////////////////////////////////////////////////////////////////////////
