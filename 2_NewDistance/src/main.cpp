// SOURCE FILES
#include "common.h"
#include "quadtree.h"

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

struct OpenGLManager {
    QuadTree* quadtree;
    Transforms* tranforms;
    Settings* set;

    Mesh_Data mesh_data;

    uint mode;
    string filepath;
    djg_clock* clock;

    bool force_dt;
    bool end;
    bool run_demo;
    float last_t;
    float current_t;
    float delta_T;
    bool pause;
    int w_width, w_height;
    int gui_width, gui_height;

    bool lbutton_down, rbutton_down;
    double x0, y0;

} gl = {0};

struct BenchStats {
    double avg_qt_gpu_compute, avg_qt_gpu_render;
    double avg_tess_render;
    double  total_qt_gpu_compute, total_qt_gpu_render;
    int frame_count, fps;
    double sec_timer;
    int last_frame_count;
} stat = {0};

////////////////////////////////////////////////////////////////////////////////
///
/// Utilities
///
int roundUpToSq(int n)
{
    int sq = ceil(sqrt(n));
    return (sq * sq);
}

////////////////////////////////////////////////////////////////////////////////
///
/// Mesh Functions
///

bool loadMeshBuffers(Mesh_Data* mesh_data)
{
    utility::EmptyBuffer(&mesh_data->v.bo);
    glCreateBuffers(1, &(mesh_data->v.bo));
    glNamedBufferStorage(mesh_data->v.bo,
                         sizeof(Vertex) * mesh_data->v.count,
                         (const void*)(mesh_data->v_array),
                         0);


    utility::EmptyBuffer(&mesh_data->q_idx.bo);
    if (mesh_data->quad_count > 0) {
        glCreateBuffers(1, &(mesh_data->q_idx.bo));
        glNamedBufferStorage(mesh_data->q_idx.bo,
                             sizeof(uint) * mesh_data->q_idx.count,
                             (const void*)(mesh_data->q_idx_array),
                             0);
    }


    utility::EmptyBuffer(&mesh_data->t_idx.bo);
    glCreateBuffers(1, &(mesh_data->t_idx.bo));
    if (mesh_data->triangle_count > 0) {
        glNamedBufferStorage(mesh_data->t_idx.bo,
                             sizeof(uint) * mesh_data->t_idx.count,
                             (const void*)(mesh_data->t_idx_array),
                             0);
    }

    cout << "Mesh has " << mesh_data->quad_count << " quads, "
         << mesh_data->triangle_count << " triangles " << endl;

    return (glGetError() == GL_NO_ERROR);
}

void UpdateMode(bool init = false)
{
    if (gl.mode == TERRAIN) {
        meshutils::LoadGrid(&gl.mesh_data, gl.set->grid_quads_count);
        loadMeshBuffers(&gl.mesh_data);
        gl.set->displace = true;
        gl.set->adaptive_factor = 7.0;
    } else if (gl.mode == MESH) {
        meshutils::ParseObj(gl.filepath, 0, &gl.mesh_data);
        if (gl.mesh_data.quad_count > 0 && gl.mesh_data.triangle_count == 0) {
            gl.set->prim_type = QUADS;
        } else if (gl.mesh_data.quad_count == 0 && gl.mesh_data.triangle_count > 0) {
            gl.set->prim_type = TRIANGLES;
        } else {
            cout << "ERROR when parsing obj" << endl;
        }
        loadMeshBuffers(&gl.mesh_data);
        gl.set->displace = false;
        gl.set->adaptive_factor = 7.0;
    }
    if (!init)
        gl.quadtree->Reinitialize();

}

void UpdateTransforms()
{
    gl.tranforms->updateMV();
    gl.quadtree->UploadTransforms();
}

void UpdateMeshSettings()
{
    gl.quadtree->UploadMeshSettings();
}

void ReloadBuffers() {
    loadMeshBuffers(&gl.mesh_data);
}

void DrawMesh(float deltaT, bool freeze)
{
    if (!freeze && (gl.mode == MESH) && gl.set->rotateMesh) {
        gl.tranforms->M = glm::rotate(gl.tranforms->M, 2000.0f*deltaT , vec3(0.0f, 0.0f, 1.0f));
        UpdateTransforms();
    }
    gl.quadtree->Draw(deltaT);
}

////////////////////////////////////////////////////////////////////////////////
///
/// Camera and Transforms
///

void PrintCamStuff()
{
    cout << "Position: " << glm::to_string(cam.pos) << endl;
    cout << "Look: " << glm::to_string(cam.look) << endl;
    cout << "Up: " << glm::to_string(cam.up) << endl;
    cout << "Right: " << glm::to_string(cam.right) << endl << endl;
}

void InitTranforms()
{
    cam.pos = INIT_CAM_POS[gl.mode];
    cam.look = INIT_CAM_LOOK[gl.mode];

    cam.up = vec3(0.0f, 0.0f, 1.0f);
    cam.direction = glm::normalize(cam.look - cam.pos);
    cam.look = cam.pos + cam.direction;

    cam.right = glm::normalize(glm::cross(cam.direction, cam.up));
    cam.up = -glm::normalize(glm::cross(cam.direction, cam.right));

    gl.tranforms->cam_pos = cam.pos;
    gl.tranforms->V = glm::lookAt(cam.pos, cam.look, cam.up);
    gl.tranforms->fov = 45.0;
    gl.tranforms->P = glm::perspective(glm::radians(gl.tranforms->fov), 1.0f, 0.1f, 1024.0f);

    UpdateTransforms();
}

void UpdateFOV()
{
    gl.tranforms->P = glm::perspective(glm::radians(gl.tranforms->fov), 1.0f, 0.1f, 1024.0f);
    UpdateTransforms();
}

void UpdateView()
{
    gl.tranforms->V = glm::lookAt(cam.pos, cam.look, cam.up);
    UpdateTransforms();
    gl.tranforms->cam_pos = cam.pos;
}

enum {WITH_GUI, NO_GUI};

void SaveFBToBMP (int gui)
{
    //    static int cnt = 0;
    //    char buf[1024];
    //    snprintf(buf, 1024, "%03i", cnt);
    //    if (gui == WITH_GUI) {
    //        glViewport(0, 0, gl.w_width, gl.w_height);
    //        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    //        djgt_save_glcolorbuffer_bmp(GL_FRONT, GL_RGBA, buf);
    //    } else {
    //        glViewport(0, 0, gl.render_fb.width, gl.render_fb.height);
    //        glBindFramebuffer(GL_READ_FRAMEBUFFER, gl.render_fb.fbo);
    //        djgt_save_glcolorbuffer_bmp(GL_COLOR_ATTACHMENT0, GL_RGBA, buf);
    //    }
    //    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //    ++cnt;
}

////////////////////////////////////////////////////////////////////////////////
///
/// Benchmarking Functions
///

void UpdateTime()
{
    const float dT = 1.0/120.0;
    if (gl.force_dt) {
        gl.last_t = gl.current_t;
        gl.current_t += dT;
        gl.delta_T = dT;
    } else {
        gl.current_t = glfwGetTime() * 0.001;
        gl.delta_T = gl.current_t - gl.last_t;
        gl.last_t = gl.current_t;
    }
}

void UpdateStats()
{
    stat.frame_count++;
    stat.sec_timer += gl.delta_T;
    if (stat.sec_timer < 1.0) {
        stat.total_qt_gpu_compute += gl.quadtree->ticks.gpu_compute;
        stat.total_qt_gpu_render += gl.quadtree->ticks.gpu_render;
    } else {
        stat.fps = stat.frame_count - stat.last_frame_count;
        stat.last_frame_count = stat.frame_count;
        stat.avg_qt_gpu_compute = stat.total_qt_gpu_compute / double(stat.fps);
        stat.avg_qt_gpu_render = stat.total_qt_gpu_render / double(stat.fps);
        stat.total_qt_gpu_compute = 0;
        stat.total_qt_gpu_render = 0;
        stat.sec_timer = 0;
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
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(gl.gui_width, gl.gui_height));
    float max_lod = (gl.mode == TERRAIN) ? 200.0 : 2.0;

    ImGui::Begin("Parameters", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    {
        if (ImGui::Combo("Mode", (int*)&gl.mode, "Terrain\0Mesh\0\0")) {
            UpdateMode();
            InitTranforms();
        }
        if (ImGui::Checkbox("Render Projection", &gl.set->render_projection)) {
            UpdateMeshSettings();
        }
        ImGui::SameLine();
        if (ImGui::SliderFloat("FOV", &gl.tranforms->fov, 10, 75.0)) {
            UpdateFOV();
        }
        if (ImGui::Button("Reinit Cam")) {
            InitTranforms();
        }
        ImGui::SameLine();
        if (ImGui::Button("Take Screenshot")) {
            SaveFBToBMP(NO_GUI);
        }

        ImGui::Text("\n------ Mesh settings ------");
        ImGui::Checkbox("Rotate Mesh", &gl.set->rotateMesh);
        if (ImGui::Combo("Color mode", &gl.set->color_mode,
                         "LoD & Morph\0White Wireframe\0Primitive Highlight\0Frustum\0Cull\0Debug\0\0")) {
            UpdateMeshSettings();
        }
        if (ImGui::Checkbox("Uniform", &gl.set->uniform)) {
            if(gl.set->uniform)
                gl.set->morph = false;
            UpdateMeshSettings();
        }
        ImGui::SameLine();
        if (ImGui::SliderInt("", &gl.set->uni_lvl, 0, 20)) {
            UpdateMeshSettings();
        }
        if (ImGui::SliderInt("LoD Factor", &gl.set->adaptive_factor, 1, 12)) {
            UpdateMeshSettings();
        }

        if (ImGui::Checkbox("Readback primitive count", &gl.set->map_primcount)) {
            UpdateMeshSettings();
        }
        if (gl.set->map_primcount) {
        }
        if (ImGui::Combo(" ", &gl.set->interpolation, "PN\0Phong\0No Interpolation\0\0")) {
            UpdateMeshSettings();
        }
        if (ImGui::SliderFloat("alpha", &gl.set->alpha, 0, 1.0)) {
            UpdateMeshSettings();
        }

        ImGui::Text("\n------ QuadTree settings ------");
        if (ImGui::Combo("Root primitive", &gl.set->prim_type, "Triangle\0Quad\0\0")) {
            ReloadBuffers();
            gl.quadtree->Reinitialize();
        }
        if (ImGui::SliderInt("CPU LoD", &gl.set->cpu_lod, 0, 5)) {
            gl.quadtree->ReloadLeafPrimitive();
            gl.quadtree->UploadQuadtreeSettings();
        }
        if (ImGui::Checkbox("Morph  ", &gl.set->morph)) {
            if(gl.set->uniform)
                gl.set->morph = false;
            gl.quadtree->UploadQuadtreeSettings();
        }

        if (ImGui::Checkbox("Cull", &gl.set->cull)) {
            gl.quadtree->UploadQuadtreeSettings();
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Freeze", &gl.set->freeze)) {
            gl.quadtree->ReconfigureShaders();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reinitialize QuadTree")) {
            gl.quadtree->Reinitialize();
        }
        if (ImGui::Checkbox("Debug morph", &gl.set->debug_morph)) {
            gl.quadtree->UploadQuadtreeSettings();
        }
        if (ImGui::SliderFloat("morphK", &gl.set->morph_k, 0, 1.0)) {
            gl.quadtree->UploadQuadtreeSettings();
        }



        ImGui::Text("Frame  %07i\n", stat.frame_count);
        ImGui::Text("FPS    %07i\n", stat.fps);
        ImGuiTime("deltaT", gl.delta_T);
        ImGui::Text("\nQuadtree Perf:");
        ImGuiTime("avg Total   dT (1s)", stat.avg_qt_gpu_render + stat.avg_qt_gpu_compute);
        ImGuiTime("avg Compute dT (1s)", stat.avg_qt_gpu_compute);
        ImGuiTime("avg Render  dT (1s)", stat.avg_qt_gpu_render);
        ImGui::Text("\n\n");

        static float values_qt_gpu_compute[80] = { 0 };
        static float values_qt_gpu_render[80]  = { 0 };

        static float values_fps[80] = { 0 };
        static float values_primcount[80] = { 0 };
        static int offset = 0;
        static float refresh_time = 0.0f;
        static float max_gpu_compute = 0.0, max_gpu_render = 0.0;
        static float tmp_max, tmp_val;

        static float max_primcount = 0.0;
        if (refresh_time == 0.0f)
            refresh_time = ImGui::GetTime();

        while (refresh_time < ImGui::GetTime())
        {
            values_qt_gpu_compute[offset] = gl.quadtree->ticks.gpu_compute * 1000.0;
            values_qt_gpu_render[offset]  = gl.quadtree->ticks.gpu_render  * 1000.0;

            values_fps[offset] = ImGui::GetIO().Framerate;
            values_primcount[offset] = gl.quadtree->GetPrimcount();

            offset = (offset+1) % IM_ARRAYSIZE(values_qt_gpu_compute);
            refresh_time += 1.0f/30.0f;
        }


        // QUADTREE COMPUTE DT
        tmp_max = *std::max_element(values_qt_gpu_compute, values_qt_gpu_compute+80);
        if (tmp_max > max_gpu_compute || tmp_max < 0.2 * max_gpu_compute)
            max_gpu_compute = tmp_max;
        tmp_val = gl.quadtree->ticks.gpu_compute * 1000.0;
        ImGui::PlotLines("GPU compute dT", values_qt_gpu_compute, IM_ARRAYSIZE(values_qt_gpu_compute), offset,
                         std::to_string(tmp_val).c_str(), 0.0f, max_gpu_compute, ImVec2(0,80));

        //QUADTREE RENDER DT
        tmp_val = gl.quadtree->ticks.gpu_render * 1000.0;
        tmp_max = *std::max_element(values_qt_gpu_render, values_qt_gpu_render+80);
        if (tmp_max > max_gpu_render || tmp_max < 0.2 * max_gpu_render)
            max_gpu_render = tmp_max;
        ImGui::PlotLines("GPU render dT", values_qt_gpu_render, IM_ARRAYSIZE(values_qt_gpu_render), offset,
                         std::to_string(tmp_val).c_str(), 0.0f, max_gpu_render, ImVec2(0,80));


        // FPS
        ImGui::PlotLines("FPS", values_fps, IM_ARRAYSIZE(values_fps), offset,
                         std::to_string(ImGui::GetIO().Framerate).c_str(), 0.0f, 1000, ImVec2(0,80));

        // PRIMCOUNT
        if (gl.set->map_primcount) {
            tmp_max = *std::max_element(values_primcount, values_primcount+80);
            if (tmp_max > max_primcount || tmp_max < 0.2 * max_primcount)
                max_primcount = tmp_max;
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
        UpdateView();

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
        UpdateView();

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
    UpdateView();
}

// *************************************************************************** //
// |                                 PROGRAM                                    |
// *************************************************************************** //


void Init()
{
    cout << "******************************************************" << endl;
    cout << "INITIALIZATION" << endl;

    gl.quadtree;
    gl.pause = false;
    gl.clock = djgc_create();
    gl.quadtree= new QuadTree();
    gl.tranforms = new Transforms();
    gl.set = new Settings();
    gl.mesh_data = {};

    //    gl.filepath = "../bigguy.obj";
    gl.filepath = "cube.obj";
    gl.run_demo = false;
    gl.end = false;
    gl.force_dt = false;

    gl.current_t = 0;

    gl.mode = MESH;
    INIT_CAM_POS[TERRAIN]  = vec3(8.5, 8.5, 4);
    INIT_CAM_LOOK[TERRAIN] = vec3(7, 7, 3.5);
    INIT_CAM_POS[MESH]  = vec3(3.4, 3.4, 2.4);
    INIT_CAM_LOOK[MESH] =  vec3(2.8, 2.8, 2.0);

    gl.set->grid_quads_count = roundUpToSq(gl.set->grid_quads_count);

    UpdateMode(true);

    gl.quadtree->Init(&gl.mesh_data, gl.tranforms, gl.set);

    InitTranforms();

    stat.avg_qt_gpu_compute = 0;
    stat.avg_qt_gpu_render = 0;
    stat.avg_tess_render = 0;
    stat.frame_count = 0;
    stat.total_qt_gpu_compute = 0;
    stat.total_qt_gpu_render = 0;
    stat.sec_timer = 0;
    stat.fps = 0;
    stat.last_frame_count = 0;

    cout << "END OF INITIALIZATION" << endl;
    cout << "******************************************************" << endl << endl;

}

void Draw()
{
    UpdateTime();

    glViewport(gl.gui_width, 0, gl.w_width, gl.w_height);
    DrawMesh(gl.delta_T, gl.set->freeze);
    // gl.point->Draw(gl.delta_T);
    glViewport(0, 0, gl.w_width + gl.gui_width, gl.w_height);
    UpdateStats();

    RenderImgui();
}

void Cleanup() {
    gl.quadtree->CleanUp();
    delete[] gl.mesh_data.v_array;
    delete[] gl.mesh_data.q_idx_array;
    delete[] gl.mesh_data.t_idx_array;
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
    GLFWwindow* window = glfwCreateWindow((gl.w_width + gl.gui_width), gl.w_height,
                                          "Distance Based Tessellation", NULL, NULL);
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

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
    glDebugMessageCallback(debug_output_logger, NULL);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    try {
        Init();
        ImGui::CreateContext();
        ImGui_ImplGlfwGL3_Init(window, false);
        ImGui::StyleColorsDark();

        // delta_T condition to avoid crashing my system
        while (!glfwWindowShouldClose(window)  && gl.delta_T < 5.0)
        {
            glfwPollEvents();
            if (!gl.pause)
            {
                ImGui_ImplGlfwGL3_NewFrame();
                Draw();
                ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
                glfwSwapBuffers(window);
            }
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
