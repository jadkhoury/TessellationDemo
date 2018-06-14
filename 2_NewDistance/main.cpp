// SOURCE FILES
#include "common.h"
#include "quadtree.h"
#include "mesh_utils.h"

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
    bool pause;
    int w_width, w_height;
    int gui_width, gui_height;

    bool lbutton_down, rbutton_down;
    double x0, y0;
} gl = {};

struct Mesh {
    uint mode;
    QuadTree* quadtree;
    Transforms* tranforms;
    QuadTree::Settings init_settings;

    Mesh_Data mesh_data;
    uint grid_quads_count;
    string filepath;
    const string default_filepath = "bigguy.obj";

    void Init();
    bool LoadMeshBuffers();
    void LoadMeshData(bool init);
    void UpdateTransforms();
    void Draw(float deltaT, bool freeze);
} mesh = {};

struct BenchStats {
    float last_t;
    float current_t;
    float delta_T;

    double avg_qt_gpu_compute, avg_qt_gpu_render;
    double avg_tess_render;
    double  total_qt_gpu_compute, total_qt_gpu_render;
    int frame_count, fps;
    double sec_timer;
    int last_frame_count;

    void Init();
    void UpdateTime();
    void UpdateStats();
} benchStats = {};


////////////////////////////////////////////////////////////////////////////////
///
/// Mesh Functions
///

int roundUpToSq(int n)
{
    int sq = ceil(sqrt(n));
    return (sq * sq);
}

void reorderIndices(uint* i_array, const Vertex* v, uint count){
    uint  i0, i1, i2;
    float d01, d02, d12, max_d;
    for(uint i=0; i < count; i+= 3)
    {
        i0 = i_array[i + 0];
        i1 = i_array[i + 1];
        i2 = i_array[i + 2];

        d01 = glm::distance(v[i0].p, v[i1].p);
        d02 = glm::distance(v[i0].p, v[i2].p);
        d12 = glm::distance(v[i1].p, v[i2].p);

        max_d = std::max(d01, std::max(d02, d12));
        if(max_d == d01){
            i_array[i + 0] = i2;
            i_array[i + 1] = i0;
            i_array[i + 2] = i1;
        } else if (max_d == d02) {
            i_array[i + 0] = i1;
            i_array[i + 1] = i0;
            i_array[i + 2] = i2;
        }
    }
}

void Mesh::Init()
{
    quadtree = new QuadTree();
    tranforms = new Transforms();
    mesh_data = {};
    grid_quads_count = 1;
    grid_quads_count = roundUpToSq(grid_quads_count);

    if (filepath != default_filepath)
        mode = MESH;
    else
        mode = TERRAIN;

    init_settings = {
        /*int uni_lvl*/ 0,
        /*float adaptive_factor*/ 1,
        /*bool uniform*/ false,
        /*bool map_primcount*/ true,
        /*bool rotateMesh*/ false,
        /*bool displace*/ true,
        /*int color_mode*/ LOD,
        /*bool render_projection*/ true,

        /*int prim_type*/ TRIANGLES,
        /*bool morph*/ true,
        /*bool freeze*/ false,
        /*int cpu_lod*/ 2,
        /*bool cull*/ true,
        /*bool debug_morph*/ false,
        /*float morph_k*/ 0.0,
        /*uint wg_count*/ 512
    };

    this->LoadMeshData(true);
    quadtree->Init(&mesh.mesh_data, mesh.tranforms, init_settings);
}

/*
 * Fill the Mesh_Data structure with vertices and indices
 * Either:
 * - Loads a grid for the terrain mode
 * - Loads an .obj file using the parsing function in mesh_utils.h
 * Depending on the index array filled by the parsing function, sets the
 * quadtree to the correct polygon rendering mode
 */
void Mesh::LoadMeshData(bool init = false)
{
    QuadTree::Settings& settings = (init) ? init_settings : quadtree->settings;

    if (mode == TERRAIN)
    {
        meshutils::LoadGrid(&mesh_data,  grid_quads_count);
        LoadMeshBuffers();
        settings.displace = true;
        settings.adaptive_factor = 50.0;

    } else if (mode == MESH) {

        meshutils::ParseObj(filepath, 0, &mesh_data);
        if (mesh_data.quad_count > 0 && mesh_data.triangle_count == 0) {
            settings.prim_type = QUADS;
        } else if (mesh_data.quad_count == 0 && mesh_data.triangle_count > 0) {
            settings.prim_type = TRIANGLES;
        } else {
            cout << "ERROR when parsing obj" << endl;
        }
        settings.adaptive_factor = 1.0;
        LoadMeshBuffers();
        settings.displace = false;
    }
    if (!init)
        quadtree->Reinitialize();
}

/*
 * Loads the data in the Mesh_Data structure into buffers.
 * Creates:
 * - 1 buffer for (unique) vertices
 * - 1 buffer for quad indices
 * - 1 buffer for triangle indices
 * It seemed easier to have for index buffer available on the gpu and switch
 * from one to the other depending on the mesh polygon used
 */
bool Mesh::LoadMeshBuffers()
{
    utility::EmptyBuffer(&mesh_data.v.bo);
    glCreateBuffers(1, &(mesh_data.v.bo));
    glNamedBufferStorage(mesh_data.v.bo,
                         sizeof(Vertex) * mesh_data.v.count,
                         (const void*)(mesh_data.v_array),
                         0);


    utility::EmptyBuffer(&mesh_data.q_idx.bo);
    if (mesh_data.quad_count > 0) {
        glCreateBuffers(1, &(mesh_data.q_idx.bo));
        glNamedBufferStorage(mesh_data.q_idx.bo,
                             sizeof(uint) * mesh_data.q_idx.count,
                             (const void*)(mesh_data.q_idx_array),
                             0);
    }

    reorderIndices(mesh_data.t_idx_array, mesh_data.v_array, mesh_data.t_idx.count);
    utility::EmptyBuffer(&mesh_data.t_idx.bo);
    glCreateBuffers(1, &(mesh_data.t_idx.bo));
    if (mesh_data.triangle_count > 0) {
        glNamedBufferStorage(mesh_data.t_idx.bo,
                             sizeof(uint) * mesh_data.t_idx.count,
                             (const void*)(mesh_data.t_idx_array),
                             0);
    }

    cout << "Mesh has " << mesh_data.quad_count << " quads, "
         << mesh_data.triangle_count << " triangles " << endl;

    return (glGetError() == GL_NO_ERROR);
}

void Mesh::UpdateTransforms()
{
    tranforms->UpdateMV();
    quadtree->UploadTransforms();
}

void Mesh::Draw(float deltaT, bool freeze)
{
    if (!freeze && (mode == MESH) &&  quadtree->settings.rotateMesh) {
        tranforms->M = glm::rotate(tranforms->M, 2000.0f*deltaT , vec3(0.0f, 0.0f, 1.0f));
        UpdateTransforms();
    }
    quadtree->Draw(deltaT);
}




////////////////////////////////////////////////////////////////////////////////
///
/// Benchmarking Functions
///

void BenchStats::Init()
{
    current_t = glfwGetTime() * 0.001;
    last_t = current_t;
    delta_T = 0;

    avg_qt_gpu_compute = 0;
    avg_qt_gpu_render = 0;
    avg_tess_render = 0;
    frame_count = 0;
    total_qt_gpu_compute = 0;
    total_qt_gpu_render = 0;
    sec_timer = 0;
    fps = 0;
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
    if (sec_timer < 1.0) {
        total_qt_gpu_compute += mesh.quadtree->ticks.gpu_compute;
        total_qt_gpu_render += mesh.quadtree->ticks.gpu_render;
    } else {
        fps = frame_count - last_frame_count;
        last_frame_count = frame_count;
        avg_qt_gpu_compute = total_qt_gpu_compute / double(fps);
        avg_qt_gpu_render = total_qt_gpu_render / double(fps);
        total_qt_gpu_compute = 0;
        total_qt_gpu_render = 0;
        sec_timer = 0;
    }
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
    cam.pos = INIT_CAM_POS[mesh.mode];
    cam.look = INIT_CAM_LOOK[mesh.mode];

    cam.up = vec3(0.0f, 0.0f, 1.0f);
    cam.direction = glm::normalize(cam.look - cam.pos);
    cam.look = cam.pos + cam.direction;

    cam.right = glm::normalize(glm::cross(cam.direction, cam.up));
    cam.up = -glm::normalize(glm::cross(cam.direction, cam.right));

    mesh.tranforms->cam_pos = cam.pos;
    mesh.tranforms->V = glm::lookAt(cam.pos, cam.look, cam.up);
    mesh.tranforms->fov = 45.0;
    mesh.tranforms->P = glm::perspective(glm::radians(mesh.tranforms->fov), gl.w_width/(float)gl.w_height, 0.1f, 1024.0f);

    mesh.UpdateTransforms();
}

void UpdateForNewFOV()
{
    mesh.tranforms->P = glm::perspective(glm::radians(mesh.tranforms->fov), gl.w_width/(float)gl.w_height, 0.1f, 1024.0f);
    mesh.UpdateTransforms();
}

void UpdateForNewView()
{
    mesh.tranforms->V = glm::lookAt(cam.pos, cam.look, cam.up);
    mesh.UpdateTransforms();
    mesh.tranforms->cam_pos = cam.pos;
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

    QuadTree::Settings& settings_ref = mesh.quadtree->settings;
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(gl.gui_width, gl.gui_height));
    float max_lod = (mesh.mode == TERRAIN) ? 100.0 : 10.0;

    ImGui::Begin("Parameters", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    {
        if (ImGui::Combo("Mode", (int*)&mesh.mode, "Terrain\0Mesh\0\0")) {
            mesh.LoadMeshData();
            InitTranforms();
        }
        if (ImGui::Checkbox("Render Projection", &settings_ref.render_projection)) {
            mesh.quadtree->UploadSettings();
        }
        ImGui::SameLine();
        if (ImGui::SliderFloat("FOV", &mesh.tranforms->fov, 10, 75.0)) {
            UpdateForNewFOV();
        }
        if (ImGui::Button("Reinit Cam")) {
            InitTranforms();
        }
        ImGui::Text("\n------ Mesh settings ------");
        if (ImGui::Checkbox("Displace Mesh", &settings_ref.displace)) {
            mesh.quadtree->UploadSettings();
        }
        if (ImGui::Checkbox("Rotate Mesh", &settings_ref.rotateMesh)) {
            mesh.quadtree->UploadSettings();
        }

        if (ImGui::Combo("Color mode", &settings_ref.color_mode,
                         "LoD & Morph\0White Wireframe\0Primitive Highlight\0Frustum\0Cull\0Debug\0\0")) {
            mesh.quadtree->UploadSettings();
        }
        if (ImGui::Checkbox("Uniform", &settings_ref.uniform)) {
            mesh.quadtree->UploadSettings();
        }
        ImGui::SameLine();
        if (ImGui::SliderInt("", &settings_ref.uni_lvl, 0, 20)) {
            mesh.quadtree->UploadSettings();
        }
        if (ImGui::SliderFloat("LoD Factor", &settings_ref.adaptive_factor, 1, max_lod)) {
            mesh.quadtree->UploadSettings();
        }

        if (ImGui::Checkbox("Readback primitive count", &settings_ref.map_primcount)) {
            mesh.quadtree->UploadSettings();
        }
        if ( settings_ref.map_primcount) {
            ImGui::Text(utility::LongToString(mesh.quadtree->GetPrimcount()).c_str());
        }

        ImGui::Text("\n------ QuadTree settings ------");
        if (ImGui::Combo("Root primitive", &settings_ref.prim_type, "Triangle\0Quad\0\0")) {
            mesh.LoadMeshBuffers();
            mesh.quadtree->Reinitialize();
        }
        if (ImGui::SliderInt("CPU LoD", &settings_ref.cpu_lod, 0, 8)) {
            if(settings_ref.cpu_lod < 2)
                settings_ref.morph = false;
            mesh.quadtree->ReloadLeafPrimitive();
            mesh.quadtree->UploadSettings();
        }
        if (ImGui::Checkbox("Morph  ", &settings_ref.morph)) {
            mesh.quadtree->UploadSettings();
        }

        if (ImGui::Checkbox("Cull", &settings_ref.cull)) {
            mesh.quadtree->UploadSettings();
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Freeze", &settings_ref.freeze)) {
            mesh.quadtree->ReconfigureShaders();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reinitialize QuadTree")) {
            mesh.quadtree->Reinitialize();
        }
        if (ImGui::Checkbox("Debug morph", &settings_ref.debug_morph)) {
            mesh.quadtree->UploadSettings();
        }
        if (ImGui::SliderFloat("morphK", &settings_ref.morph_k, 0, 1.0)) {
            mesh.quadtree->UploadSettings();
        }

        ImGui::Text("\n------ Benchmarking ------");
        static int tmp = log2(settings_ref.wg_count);
        if (ImGui::SliderInt("wg count (pow2)", &tmp, 2, 11)) {
            settings_ref.wg_count = 1 << tmp;
            mesh.quadtree->UpdateWGCount();
        }
        ImGui::SameLine();
        ImGui::Text(std::to_string(settings_ref.wg_count).c_str());

        ImGui::Text("Frame  %07i\n", benchStats.frame_count);
        ImGui::Text("FPS    %07i\n", benchStats.fps);
        ImGuiTime("deltaT", benchStats.delta_T);
        ImGui::Text("\nQuadtree Perf:");
        ImGuiTime("avg GPU Compute dT (1s)", benchStats.avg_qt_gpu_compute);
        ImGuiTime("avg GPU Render  dT (1s)", benchStats.avg_qt_gpu_render);
        ImGuiTime("avg Total GPU   dT (1s)", benchStats.avg_qt_gpu_render + benchStats.avg_qt_gpu_compute);
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
            values_qt_gpu_compute[offset] = mesh.quadtree->ticks.gpu_compute * 1000.0;
            values_qt_gpu_render[offset]  = mesh.quadtree->ticks.gpu_render  * 1000.0;

            values_fps[offset] = ImGui::GetIO().Framerate;
            values_primcount[offset] = mesh.quadtree->GetPrimcount();

            offset = (offset+1) % IM_ARRAYSIZE(values_qt_gpu_compute);
            refresh_time += 1.0f/30.0f;
        }


        // QUADTREE COMPUTE DT
        tmp_max = *std::max_element(values_qt_gpu_compute, values_qt_gpu_compute+80);
        if (tmp_max > max_gpu_compute || tmp_max < 0.2 * max_gpu_compute)
            max_gpu_compute = tmp_max;
        tmp_val = mesh.quadtree->ticks.gpu_compute * 1000.0;
        ImGui::PlotLines("GPU compute dT", values_qt_gpu_compute, IM_ARRAYSIZE(values_qt_gpu_compute), offset,
                         std::to_string(tmp_val).c_str(), 0.0f, max_gpu_compute, ImVec2(0,80));

        //QUADTREE RENDER DT
        tmp_val = mesh.quadtree->ticks.gpu_render * 1000.0;
        tmp_max = *std::max_element(values_qt_gpu_render, values_qt_gpu_render+80);
        if (tmp_max > max_gpu_render || tmp_max < 0.2 * max_gpu_render)
            max_gpu_render = tmp_max;
        ImGui::PlotLines("GPU render dT", values_qt_gpu_render, IM_ARRAYSIZE(values_qt_gpu_render), offset,
                         std::to_string(tmp_val).c_str(), 0.0f, max_gpu_render, ImVec2(0,80));


        // FPS
        ImGui::PlotLines("FPS", values_fps, IM_ARRAYSIZE(values_fps), offset,
                         std::to_string(ImGui::GetIO().Framerate).c_str(), 0.0f, 1000, ImVec2(0,80));

        // PRIMCOUNT
        if ( settings_ref.map_primcount) {
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
            mesh.quadtree->ReloadShaders();
            break;
        case GLFW_KEY_U:
            mesh.quadtree->ReconfigureShaders();
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
        UpdateForNewView();

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
        UpdateForNewView();

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
    UpdateForNewView();
}

////////////////////////////////////////////////////////////////////////////////
///
/// The Program
///

void Init()
{
    cout << "******************************************************" << endl;
    cout << "INITIALIZATION" << endl;

    gl.pause = false;


    INIT_CAM_POS[TERRAIN]  = vec3(3.9, 3.6, 0.6);
    INIT_CAM_LOOK[TERRAIN] = vec3(3.3, 2.9, 0.33);
    INIT_CAM_POS[MESH]  = vec3(3.4, 3.4, 2.4);
    INIT_CAM_LOOK[MESH] =  vec3(2.8, 2.8, 2.0);

    mesh.Init();
    benchStats.Init();
    InitTranforms();




    cout << "END OF INITIALIZATION" << endl;
    cout << "******************************************************" << endl << endl;

}

void Draw()
{
    benchStats.UpdateTime();

    glViewport(gl.gui_width, 0, gl.w_width, gl.w_height);
    mesh.Draw(benchStats.delta_T, mesh.quadtree->settings.freeze);
    glViewport(0, 0, gl.w_width + gl.gui_width, gl.w_height);
    benchStats.UpdateStats();

    RenderImgui();
}

void Cleanup() {
    mesh.quadtree->CleanUp();
    delete[] mesh.mesh_data.v_array;
    delete[] mesh.mesh_data.q_idx_array;
    delete[] mesh.mesh_data.t_idx_array;
}

void HandleArguments(int argc, char **argv)
{
    if (argc == 1) {
        mesh.filepath = mesh.default_filepath;
       cout << "Using default mesh: " << mesh.default_filepath << endl;
    } else {
        if (argc > 2)
            cout << "Only takes in 1 obj file name, ignoring other arguments" << endl;
        string file = argv[1];
        cout << "Trying to open " << file << " ... ";
        ifstream f(file.c_str());
        if (f.good()) {
            cout << "OK" << endl;
            mesh.filepath = file;
        } else {
            mesh.filepath = mesh.default_filepath;
            cout << "failure, keeping default mesh " << mesh.filepath << endl;
        }
    }
}

int main(int argc, char **argv)
{
    HandleArguments(argc, argv);

    gl.w_width = 1920;
    gl.w_height = 1080;
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
        while (!glfwWindowShouldClose(window)  && benchStats.delta_T < 5.0)
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
