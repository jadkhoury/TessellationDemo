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

#define OUT uint32_t(-1)


using glm::findMSB;
using glm::bvec3;

using std::cout;
using std::endl;
using std::string;
using std::bitset;
typedef std::bitset<32> bits;

#define LOG(fmt, ...)  fprintf(stdout, fmt, ##__VA_ARGS__); fflush(stdout);

char *strcat2(char *dst, const char *src1, const char *src2)
{
    strcpy(dst, src1);

    return strcat(dst, src2);
}

static void
debug_output_logger(
        GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        GLsizei length,
        const GLchar* message,
        const GLvoid* userParam
        ) {
    char srcstr[32], typestr[32];

    switch(source) {
    case GL_DEBUG_SOURCE_API: strcpy(srcstr, "OpenGL"); break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM: strcpy(srcstr, "Windows"); break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: strcpy(srcstr, "Shader Compiler"); break;
    case GL_DEBUG_SOURCE_THIRD_PARTY: strcpy(srcstr, "Third Party"); break;
    case GL_DEBUG_SOURCE_APPLICATION: strcpy(srcstr, "Application"); break;
    case GL_DEBUG_SOURCE_OTHER: strcpy(srcstr, "Other"); break;
    default: strcpy(srcstr, "???"); break;
    };

    switch(type) {
    case GL_DEBUG_TYPE_ERROR: strcpy(typestr, "Error"); break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: strcpy(typestr, "Deprecated Behavior"); break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: strcpy(typestr, "Undefined Behavior"); break;
    case GL_DEBUG_TYPE_PORTABILITY: strcpy(typestr, "Portability"); break;
    case GL_DEBUG_TYPE_PERFORMANCE: strcpy(typestr, "Performance"); break;
    case GL_DEBUG_TYPE_OTHER: strcpy(typestr, "Message"); break;
    default: strcpy(typestr, "???"); break;
    }

    if(severity == GL_DEBUG_SEVERITY_HIGH) {
        LOG("djg_error: %s %s\n"                \
            "-- Begin -- GL_debug_output\n" \
            "%s\n"                              \
            "-- End -- GL_debug_output\n",
            srcstr, typestr, message);
    } else if(severity == GL_DEBUG_SEVERITY_MEDIUM) {
        LOG("djg_warn: %s %s\n"                 \
            "-- Begin -- GL_debug_output\n" \
            "%s\n"                              \
            "-- End -- GL_debug_output\n",
            srcstr, typestr, message);
    }
}

void log_debug_output(void)
{
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(&debug_output_logger, NULL);
}

// -----------------------------------------------------------------------------

struct AppManager {
    struct {
        const char *shader;
        const char *output;
    } dir;
} g_app = {
    /*dir*/    {"../viewer-subd-leb/shaders/", "./"},
};

struct GeometryManager {
    int subdivLevel, vertexCnt;
} g_geometry = {
    0, 0
};

enum {PROGRAM_POINT, PROGRAM_TRIANGLE, PROGRAM_TREE, PROGRAM_ACTIVENODE, PROGRAM_COUNT};

struct OpenGLManager {
    GLuint vertexArray;
    GLuint treeBuffer;
    GLuint programs[PROGRAM_COUNT];
} g_gl = {0};

struct DemoParameters {
    int maxLevel;
    uint32_t activeNode;
    uint32_t activeHn;
    dja::vec2 target;
    float radius;
    struct {bool reset, manualSplit, balance, real_time;} flags;
} g_params = {
    6, 0, 0, {0.7, 0.7}, 0.1, {true, true, true, false}
                      };

// -----------------------------------------------------------------------------
float wedge(const dja::vec2& a, const dja::vec2& b)
{
    return a.x * b.y - a.y * b.x;
}

struct triangle {
    dja::vec2 v[3];
    triangle(const dja::vec3& a, const dja::vec3& b, const dja::vec3& c)
    {
        v[0].x = a.x; v[0].y = a.y;
        v[1].x = b.x; v[1].y = b.y;
        v[2].x = c.x; v[2].y = c.y;
    }

    bool contains(const dja::vec2 &p) const {
        float w1 = wedge(v[1] - v[0], p - v[0]);
        float w2 = wedge(v[2] - v[1], p - v[1]);
        float w3 = wedge(v[0] - v[2], p - v[2]);
        return (w1 <= 0 && w2 <= 0 && w3 <= 0) || (w1 >= 0 && w2 >= 0 && w3 >= 0);
    }
};

// decode key
dja::mat3 keyToXform(uint32_t key)
{
    dja::mat3 xf = dja::mat3(1);

    while (key > 1) {
        float b = key & 0x1;
        float s = 2 * b - 1;

        xf = dja::mat3(+s*0.5f, -0.5f  , 0.5f,
                       -0.5f  , -s*0.5f, 0.5f,
                       0      , 0      , 1) * xf;
        key = key >> 1;
    }

    return xf;
}

string LongToString(long l)
{
    bool neg = (l<0);
    if (neg) l = -l;
    string s = "";
    int mod;
    while (l > 0) {
        mod = l%1000;
        s = std::to_string(mod) +  s;
        if (l>999) {
            if (mod < 10) s = "0" + s;
            if (mod < 100) s = "0" + s;
            s = " " + s;
        }
        l /= 1000;
    }
    if (neg) s = "-" + s;
    return s;
}

struct quadtree {

    std::vector<uint32_t> tree[2];
    std::vector<uint32_t> target_list[2];
    std::vector<uint32_t> merge_list[2];

    std::vector<uint32_t> target_history;
    std::vector<std::vector<uint32_t>> tree_history;
    long num_iteration = 0;



    int pingPong;
    uint max_level = 0;
    uint next_f_max, current_f_max;

    quadtree() {}

    inline uint32_t lvl(uint32_t k) {
        return findMSB(k);
    }

    inline uint32_t removeLvlBit(uint32_t k) {
        return k ^ (1 << findMSB(k));
    }

    inline uint32_t extractBit(const uint32_t& in, uint pos) {
        return ((in >> pos) & 1);
    }

    string nodeToString(uint32_t ui)
    {
        if (ui == OUT)
            return "OUT";
        bits b = bits(ui);
        string s = b.to_string();
        int max = findMSB(ui);
        s.erase(0, 32-max);
        return s;
    }

    void printNodeList(const std::vector<uint32_t>& list)
    {
        for (int i = 0; i < list.size(); ++i) {
            cout << nodeToString(list[i]) << " ";
        }
        cout << endl;
    }

    string nodeListToString(const std::vector<uint32_t>& list)
    {
        string s = "";
        for (int i = 0; i < list.size(); ++i) {
            s += nodeToString(list[i]) + " ";
        }
        return s;
    }


    void updateMaxLevel() {
        max_level = 0;
        const std::vector<uint32_t> &in = tree[pingPong];
        for (int i = 0; i < (int)in.size(); ++i)
            max_level = std::max(lvl(in[i]), max_level);
    }

    uint32_t Parent(uint32_t node) {
        return node >> 1;
    }

    uint32_t HNeighbour(uint32_t node)
    {
        uint b1, b0, offset = 0;
        uint lvl = findMSB(node);
        uint end = lvl - (lvl & 0x1); // for odd levels, don't touch the first bit
        bool done = false;
        while (offset < end && !done)
        {
            b0 = extractBit(node, offset);
            b1 = extractBit(node, offset+1);
            if (b1 == b0)
                offset += 2;
            else
                done = true;
        }
        uint32_t mask = (1 << (offset+2)) - 1;
        uint32_t result = done ? (node ^ mask) : OUT;
        return result;
    }

    void build(const dja::vec2 &target, int max_depth) {
        cout << "*****************************************************" << endl;
        max_level = 0;
        tree[0].resize(0);
        tree[1].resize(0);
        target_list[0].resize(0);
        target_list[1 ].resize(0);
        merge_list[0].resize(0);
        merge_list[1].resize(0);
        tree_history.resize(0);

        tree[0].push_back(1);
        tree_history.push_back(tree[0]);

        target_history.resize(0);

        pingPong = 0;
#if 1
        for (int i = 0; i < max_depth; ++i) {
            updateOnce2(target);
        }
#else
        while (max_level < max_depth)
            updateOnce2(target);
#endif
        cout << "DONE" << endl;
        cout << "*****************************************************" << endl;


    }

    void subdivNode_unbalanced(uint32_t target_key, uint32_t node)
    {
        std::vector<uint32_t> &out = tree[1 - pingPong];
        if (node == target_key) {
            out.push_back(node << 1 | 0);
            out.push_back(node << 1 | 1);
        } else {
            out.push_back(node);
        }
    }

    void subdivNode_balanced(uint32_t target_key, uint32_t node)
    {
        std::vector<uint32_t> &out = tree[1 - pingPong];
        uint current_lvl = lvl(target_key);
        uint node_lvl = lvl(node);
        uint diff_lvl = current_lvl - node_lvl;
        uint32_t h = OUT, hp = OUT, hph = HNeighbour(target_key);

        if (node == target_key) {
            out.push_back(node << 1 | 0);
            out.push_back(node << 1 | 1);
            return;
        }

        if (diff_lvl >= 0) {
            while (current_lvl > node_lvl && hph != OUT)
            {
                h = hph;
                hp = Parent(h);
                hph = HNeighbour(hp);
                current_lvl--;
            }
            if (node == hp) {
                out.push_back(h << 1 | 0);
                out.push_back(h << 1 | 1);
                out.push_back(h ^ 0x1);
                return;
            } else if (node == hph) {
                out.push_back(hph << 1 | 0);
                out.push_back(hph << 1 | 1);
                return;
            }
        }
        out.push_back(node);
    }


    void subdivTree(uint32_t target_key)
    {
        const std::vector<uint32_t> &in = tree[pingPong];
        std::vector<uint32_t> &out = tree[1 - pingPong];
        out.resize(0);

        for (int i = 0; i < (int)in.size(); ++i) {
            if (g_params.flags.balance)
                subdivNode_balanced(target_key, in[i]);
            else
                subdivNode_unbalanced(target_key, in[i]);
        }
        pingPong = 1 - pingPong;
        target_history.push_back(target_key);

    }

    void mergeNode(uint32_t target_key, uint32_t node)
    {
        std::vector<uint32_t> &out = tree[1 - pingPong];
        uint32_t target_p   = Parent(target_key);
        uint32_t target_ph = HNeighbour(target_p) ;
        uint32_t node_p = Parent(node);

        if (node_p == target_p || node_p == target_ph) {
            if (node & 0x1)
                out.push_back(node >> 1);
        } else {
            out.push_back(node);
        }
    }

    void mergeTree(uint32_t target_key)
    {
        const std::vector<uint32_t> &in = tree[pingPong];
        std::vector<uint32_t> &out = tree[1 - pingPong];
        out.resize(0);

        for (int i = 0; i < (int)in.size(); ++i) {
            mergeNode(target_key, in[i]);
        }
        pingPong = 1 - pingPong;
        cout << endl;
    }


    // bvec3 split = (x,y,z)
    // split[0] = split node
    // split[1] = split child 0
    // split[2] = split child 1
    bvec3 checkSplit(uint32_t node, uint32_t target_key)
    {
        if (node == target_key) return bvec3(1,0,0);

        uint current_lvl = lvl(target_key);
        uint node_lvl = lvl(node);
        uint diff_lvl = current_lvl - node_lvl;

        bvec3 r = bvec3(false, false, false);

        if (diff_lvl < 0) return r;

        uint32_t h = OUT, hp = OUT, hph = HNeighbour(target_key);

        while (current_lvl > node_lvl && hph != OUT)
        {
            h = hph;
            hp = Parent(h);
            hph = HNeighbour(hp);
            current_lvl--;
        }
        if (node == hp) {
            r[0] = true;
            r[(h & 0x1) + 1] = true;
            return r;
        } else if (node == hph) {
            r[0] = true;
            return r;
        }
        return r;
    }

    bool mouseLoD(const dja::vec2 &target, uint32_t node)
    {
        dja::mat3 m = keyToXform(node);
        dja::vec3 a = m * dja::vec3(0, 0, 1),
                b = m * dja::vec3(1, 0, 1),
                c = m * dja::vec3(0, 1, 1);

        return triangle(a, b, c).contains(target);
    }
    float sq(float f) {return f*f;}

    bool insideDisc(const dja::vec2 &target, const dja::vec3& p)
    {
       return std::sqrt(sq(p.x-target.x) + sq(p.y-target.y)) < g_params.radius;
    }

    bool discLoD(const dja::vec2 &target, uint32_t node) {
        dja::mat3 m = keyToXform(node);
        dja::vec3 a = m * dja::vec3(0, 0, 1),
                b = m * dja::vec3(1, 0, 1),
                c = m * dja::vec3(0, 1, 1);

        return /*(insideDisc(target, b) && insideDisc(target, c)) ||*/
                triangle(a, b, c).contains(target);;

    }
    bool diagLoD(uint32_t node)
    {
        dja::mat3 m = keyToXform(node);
        dja::vec3 a = m * dja::vec3(0, 0, 1),
                b = m * dja::vec3(1, 0, 1),
                c = m * dja::vec3(0, 1, 1);

//        bool onDiag = (a.x > 0.75) || (b.x > 0.75) || (c.x > 0.75);

        return std::rand() > (RAND_MAX / 2.0);
        bool onDiag = (a.x == a.y) || (b.x == b.y) || (c.x == c.y);
        return onDiag;
    }

    void computePass(uint32_t node , const dja::vec2 &point_pos = {1,1})
    {
//        cout << "node " << nodeToString(node) << " : " ;
        std::vector<uint32_t> &tree_out = tree[1 - pingPong];

        const std::vector<uint32_t> &splitter_in = target_list[pingPong];
        std::vector<uint32_t> &splitter_out = target_list[1 - pingPong];

        const std::vector<uint32_t> &merger_in = merge_list[pingPong];
        std::vector<uint32_t> &merger_out = merge_list[1 - pingPong];

        bvec3 node_split   = bvec3(false, false, false);
        bvec3 parent_split = bvec3(false, false, false);

        bool merge = false;
        uint32_t splitter;
        uint32_t nodes_out[4]= {node, OUT, OUT, OUT};
        uint max_depth = g_params.maxLevel;
        uint node_lvl = lvl(node);


        // Check for split in list
        for (int i = 0; i < splitter_in.size() && !glm::all(node_split); ++i)
        {
            splitter = splitter_in[i];
            node_split   = node_split   || checkSplit(node, splitter);
            num_iteration++;
        }

        // Computing Output Nodes
        if (node_split[0]) {
            nodes_out[0] = (node_split[1]) ? (node << 2) | 0 : (node << 1) | 0;
            nodes_out[1] = (node_split[1]) ? (node << 2) | 1 : OUT;
            nodes_out[2] = (node_split[2]) ? (node << 2) | 2 : (node << 1) | 1;
            nodes_out[3] = (node_split[2]) ? (node << 2) | 3 : OUT;
        } else {
//            bool mergeable = !glm::any(parent_split);
//            mergeable = mergeable && lvl(node) == current_f_max;
//            mergeable = mergeable && current_f_max > 1;
//            mergeable = mergeable && !discLoD(point_pos, Parent(node));
//            if (mergeable) {
//                nodes_out[0] = (node & 0x1) ? Parent(node) : OUT;
//                cout << "merging " << nodeToString(node) << " at level " << lvl(node) << endl;
//            }
        }

       // Checking new LoD for split next pass
        uint32_t n;

        for (int i = 0; i < 4; ++i)
        {
            n = nodes_out[i];
            if (n != OUT) {
                if (discLoD(point_pos, n)) {
                    if (lvl(n) < max_depth) {
                        splitter_out.push_back(n);
                        next_f_max = std::max(next_f_max, lvl(n)+1);
                    }
                }
            }
        }


        // Outputing nodes
        for (int i = 0; i < 4; ++i)
        {
            n = nodes_out[i];
            if (n != OUT) {
                tree_out.push_back(n);
                next_f_max = std::max(next_f_max, lvl(n));
            }
        }
    }

    void updateOnce2(const dja::vec2 &target)
    {
        num_iteration = 0;
        const std::vector<uint32_t> &tree_in = tree[pingPong];
        std::vector<uint32_t> &tree_out = tree[1 - pingPong];

        const std::vector<uint32_t> &targets_in = target_list[pingPong];
        std::vector<uint32_t> &targets_out = target_list[1 - pingPong];

        const std::vector<uint32_t> &merger_in = merge_list[pingPong];
        std::vector<uint32_t> &merger_out = merge_list[1 - pingPong];

        tree_out.resize(0);
        targets_out.resize(0);

        current_f_max = next_f_max;
        next_f_max = 0;


        for (int i = 0; i < (int)tree_in.size(); ++i)  {
            computePass(tree_in[i], target);
        }
        pingPong = 1-pingPong;
        tree_history.push_back(tree_out);
        updateMaxLevel();
//        cout << "num nodes     " << tree_out.size() << endl;
//        cout << "num target in " << targets_in.size() << endl;
//        cout << "num_iteration " << num_iteration << endl;
        cout << "targets: "; printNodeList(targets_out);
        cout << "next_f_max: " << next_f_max << endl << endl;;
//        cout << "mergers: "; printNodeList(merger_out);
//        cout  << endl;
    }


    void updateOnce(const dja::vec2 &target, bool merge = false)
    {
        const std::vector<uint32_t> &in = tree[pingPong];
        std::vector<uint32_t> &out = tree[1 - pingPong];
        uint32_t target_key;
        out.resize(0);

        bool found = false;
        for (int i = 0; i < (int)in.size() && !found; ++i)
        {
            uint32_t key = in[i];
            dja::mat3 m = keyToXform(key);
            dja::vec3 a = m * dja::vec3(0, 0, 1),
                    b = m * dja::vec3(1, 0, 1),
                    c = m * dja::vec3(0, 1, 1);

            bool lod_criterion = triangle(a, b, c).contains(target);
            // We can't merge a node not at max lvl if we balance the tree
            if (lod_criterion) {
                bool valid = merge ? (max_level > 0 && lvl(key) == max_level) : true;
                if (valid){
                    target_key = key;
                    found = true;
                }
            }
        }
        if (found) {
            cout << "*** Target " << nodeToString(target_key) << " ***" << endl;
            if (merge)
                mergeTree(target_key);
            else
                subdivTree(target_key);
            updateMaxLevel();
            tree_history.push_back(out);
        }
    }

    void undoLastAction()
    {
        if(tree_history.size() <= 1)
            return;
        std::vector<uint32_t> &out = tree[1 - pingPong];
        out.resize(0);
        tree_history.pop_back();
        out = tree_history.back();
        pingPong = 1 - pingPong;
        updateMaxLevel();
    }

    uint32_t getNode(const dja::vec2 &target) const
    {
        const std::vector<uint32_t> &in = tree[pingPong];
        for (int i = 0; i < (int)in.size(); ++i) {
            uint32_t key = in[i];
            dja::mat3 m = keyToXform(key);
            dja::vec3 a = m * dja::vec3(0, 0, 1),
                    b = m * dja::vec3(1, 0, 1),
                    c = m * dja::vec3(0, 1, 1);
            if (triangle(a, b, c).contains(target))
                return key;
        }
        return 0;
    }

    int size() const {
        (int)tree[pingPong].size();
    }

} g_quadtree;

// -----------------------------------------------------------------------------

void loadTreeBuffer()
{
    const std::vector<uint32_t> &keys = g_quadtree.tree[g_quadtree.pingPong];
    if (glIsBuffer(g_gl.treeBuffer))
        glDeleteBuffers(1, &g_gl.treeBuffer);
    glGenBuffers(1, &g_gl.treeBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_gl.treeBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 sizeof(uint32_t) * keys.size(),
                 &keys[0],
            GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, g_gl.treeBuffer);
}

void loadEmptyVertexArray()
{
    if (glIsVertexArray(g_gl.vertexArray))
        glDeleteVertexArrays(1, &g_gl.vertexArray);
    glGenVertexArrays(1, &g_gl.vertexArray);
    glBindVertexArray(g_gl.vertexArray);
    glBindVertexArray(0);
}

// -----------------------------------------------------------------------------

void loadPointProgram()
{
    djg_program *djp = djgp_create();
    GLuint *glp = &g_gl.programs[PROGRAM_POINT];
    char buf[1024];

    djgp_push_file(djp, strcat2(buf, g_app.dir.shader, "point.glsl"));
    if (!djgp_to_gl(djp, 450, false, true, glp)) {
        djgp_release(djp);

        throw std::runtime_error("shader creation error");
    }
    djgp_release(djp);
}

void loadTriangleProgram()
{
    djg_program *djp = djgp_create();
    GLuint *glp = &g_gl.programs[PROGRAM_TRIANGLE];
    char buf[1024];

    djgp_push_file(djp, strcat2(buf, g_app.dir.shader, "triangles.glsl"));
    if (!djgp_to_gl(djp, 450, false, true, glp)) {
        djgp_release(djp);

        throw std::runtime_error("shader creation error");
    }
    djgp_release(djp);
}

void loadTreeProgram()
{
    djg_program *djp = djgp_create();
    GLuint *glp = &g_gl.programs[PROGRAM_TREE];
    char buf[1024];

    djgp_push_file(djp, strcat2(buf, g_app.dir.shader, "tree.glsl"));
    if (!djgp_to_gl(djp, 450, false, true, glp)) {
        djgp_release(djp);

        throw std::runtime_error("shader creation error");
    }
    djgp_release(djp);
}

void loadActiveNodeProgram()
{
    djg_program *djp = djgp_create();
    GLuint *glp = &g_gl.programs[PROGRAM_ACTIVENODE];
    char buf[1024];

    djgp_push_file(djp, strcat2(buf, g_app.dir.shader, "activeNode.glsl"));
    if (!djgp_to_gl(djp, 450, false, true, glp)) {
        djgp_release(djp);

        throw std::runtime_error("shader creation error");
    }
    djgp_release(djp);
}

// -----------------------------------------------------------------------------
// allocate resources
// (typically before entering the game loop)
void load(int argc, char **argv)
{
    g_quadtree.build(g_params.target, g_params.maxLevel);

    loadEmptyVertexArray();
    loadTreeBuffer();
    loadPointProgram();
    loadTriangleProgram();
    loadTreeProgram();
    loadActiveNodeProgram();
}

// free resources
// (typically after exiting the game loop but before context deletion)
void release()
{
    glDeleteVertexArrays(1, &g_gl.vertexArray);
    for (int i = 0; i < PROGRAM_COUNT; ++i)
        glDeleteProgram(g_gl.programs[i]);
    glDeleteBuffers(1, &g_gl.treeBuffer);
}

// -----------------------------------------------------------------------------
void renderTriangleScene()
{
    // target helper
    glViewport(256, 0, 1024, 1024);
    glDisable(GL_CULL_FACE);
    glUseProgram(g_gl.programs[PROGRAM_POINT]);
    glUniform2f(
                glGetUniformLocation(g_gl.programs[PROGRAM_POINT], "u_Target"),
                g_params.target.x, g_params.target.y
                );
    glPointSize(8.f);
    glBindVertexArray(g_gl.vertexArray);
    glDrawArrays(GL_POINTS, 0, 1);
    glBindVertexArray(0);

    // triangles
    glViewport(256, 0, 1024, 1024);
    glLineWidth(1.f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glUseProgram(g_gl.programs[PROGRAM_TRIANGLE]);
    glBindVertexArray(g_gl.vertexArray);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 3, g_quadtree.size());
    glBindVertexArray(0);

    // tree helper
    glViewport(256+1024, 0, 1024, 1024);
    glUseProgram(g_gl.programs[PROGRAM_TREE]);
    glLineWidth(1.f);
    glUniform1f(
                glGetUniformLocation(g_gl.programs[PROGRAM_TREE], "u_MaxLevel"),
                g_params.maxLevel
                );
    glBindVertexArray(g_gl.vertexArray);
    glDrawArrays(GL_POINTS, 0, g_quadtree.size());
    glBindVertexArray(0);

    // active node helper
    if (g_params.flags.manualSplit) {
        glViewport(256+1024, 0, 1024, 1024);
        glUseProgram(g_gl.programs[PROGRAM_ACTIVENODE]);
        glLineWidth(2.f);
        glUniform1f(
                    glGetUniformLocation(g_gl.programs[PROGRAM_ACTIVENODE], "u_MaxLevel"),
                    g_params.maxLevel
                    );
        glUniform1ui(
                    glGetUniformLocation(g_gl.programs[PROGRAM_ACTIVENODE], "u_ActiveNode"),
                    g_params.activeNode
                    );
        glBindVertexArray(g_gl.vertexArray);
        glDrawArrays(GL_POINTS, 0, 1);
        glBindVertexArray(0);
    }

    glUseProgram(0);
}

// -----------------------------------------------------------------------------
void render()
{
    glClearColor(0.8, 0.8, 0.8, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (g_params.flags.real_time) {
        g_quadtree.updateOnce2(g_params.target);
        loadTreeBuffer();
    }
    renderTriangleScene();
}

// -----------------------------------------------------------------------------
void renderGui()
{
    ImGui::SetNextWindowPos(ImVec2(0, 0)/*, ImGuiSetCond_FirstUseEver*/);
    ImGui::SetNextWindowSize(ImVec2(256, 1024)/*, ImGuiSetCond_FirstUseEver*/);
    ImGui::Begin("Window");
    {
        ImGui::SliderInt("MaxLevel", &g_params.maxLevel, 0, 16);
        ImGui::SliderFloat("TargetX", &g_params.target.x, 0, 1);
        ImGui::SliderFloat("TargetY", &g_params.target.y, 0, 1);
        ImGui::Checkbox("Real Time", &g_params.flags.real_time);
        if (ImGui::Button("Reset Tree")) {
            g_quadtree.build(g_params.target, g_params.maxLevel);
            loadTreeBuffer();
        }
        if (ImGui::Button("Update Tree")) {
            g_quadtree.updateOnce2(g_params.target);
            loadTreeBuffer();
        }
        if (ImGui::Button("Undo")){
            g_quadtree.undoLastAction();
            loadTreeBuffer();
        }
        ImGui::Checkbox("Manual Split", &g_params.flags.manualSplit);
        ImGui::Checkbox("Balance", &g_params.flags.balance);
        ImGui::Text("Node: ");
        ImGui::SameLine();
        ImGui::Text(g_quadtree.nodeToString(g_params.activeNode).c_str());
        ImGui::Text("Hn  : ");
        ImGui::SameLine();
        ImGui::Text(g_quadtree.nodeToString(g_params.activeHn).c_str());
        ImGui::Value("Max LoD", g_quadtree.max_level);

        for (int i = 0; i < g_quadtree.tree[g_quadtree.pingPong].size(); ++i) {
            uint32_t n = g_quadtree.tree[g_quadtree.pingPong][i];
            ImGui::Text(g_quadtree.nodeToString(n).c_str());

        }
    }
    ImGui::End();

    ImGui::Render();
}

// -----------------------------------------------------------------------------
void
keyboardCallback(
        GLFWwindow* window,
        int key, int scancode, int action, int modsls
        ) {
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

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;

    if (g_params.flags.manualSplit) {
        if (action == GLFW_PRESS && (button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT)) {
            double xpos, ypos;

            glfwGetCursorPos(window, &xpos, &ypos);
            xpos-= 256.0;
            xpos/=1024.0;
            ypos/=1024.0;
            g_quadtree.updateOnce(dja::vec2(xpos, 1.0 - ypos), (button == GLFW_MOUSE_BUTTON_RIGHT));
            loadTreeBuffer();
            g_params.activeNode =
                    g_quadtree.getNode(dja::vec2(xpos, 1.0 - ypos));
        }
    }


}

void mouseMotionCallback(GLFWwindow* window, double x, double y)
{
    static double x0 = 0, y0 = 0;
    static uint32_t old_active;
    double dx = x - x0,
            dy = y - y0;

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;

    if (g_params.flags.manualSplit) {
        x-= 256.0;
        x/=1024.0;
        y/=1024.0;
        old_active = g_params.activeNode;
        g_params.activeNode =
                g_quadtree.getNode(dja::vec2(x, 1.0 - y));
        if (old_active != g_params.activeNode && g_params.activeNode > 0)
            g_params.activeHn = g_quadtree.HNeighbour(g_params.activeNode);
    }

    x0 = x;
    y0 = y;
}

void mouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
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
    GLFWwindow* window = glfwCreateWindow(2048+256, 1024, "Hello Imgui", NULL, NULL);
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
    LOG("-- Begin -- Demo\n");
    try {
        load(argc, argv);
        ImGui::CreateContext();
        ImGui_ImplGlfwGL3_Init(window, false);
        ImGui::StyleColorsDark();

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            ImGui_ImplGlfwGL3_NewFrame();
            render();
            renderGui();
            ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(window);
        }

        ImGui_ImplGlfwGL3_Shutdown();
        ImGui::DestroyContext();
        release();
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

