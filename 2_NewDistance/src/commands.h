#ifndef COMMANDS_H
#define COMMANDS_H

#include "common.h"


enum {NODES_IN_B,
      NODES_OUT_FULL_B,
      NODES_OUT_CULLED_B,
      NODECOUNTER_FULL_B,
      NODECOUNTER_CULLED_B,
      DRAW_INDIRECT_B,
      DISPATCH_INDIRECT_B,
      MESH_V_B,
      MESH_Q_IDX_B,
      MESH_T_IDX_B,
      LEAF_VERT_B,
      LEAF_IDX_B,
      BINDINGS_COUNT
     };

using namespace std;
using glm::uvec3;
using glm::uvec3;
using glm::uvec4;

#define ELEMENTS_INDIRECT

typedef struct {
    GLuint  count;
    GLuint  primCount;
    GLuint  firstIndex;
    GLuint  baseVertex;
    GLuint  baseInstance;
    uvec3  align;
} DrawElementsIndirectCommand;

typedef  struct {
    uint  count;
    uint  primCount;
    uint  first;
    uint  baseInstance;
} DrawArraysIndirectCommand;


typedef struct {
    GLuint  num_groups_x;
    GLuint  num_groups_y;
    GLuint  num_groups_z;
} DispatchIndirectCommand;

class Commands
{
private:

    enum {
        DrawIndirect,
        DispatchIndirect,
        NodeCounterFull,
        NodeCounterCulled,
        Copy,
        BUFFER_COUNT
    };
    const int NUM_ELEM = 16;
    GLuint buffers_[BUFFER_COUNT];
#ifdef ELEMENTS_INDIRECT
    DrawElementsIndirectCommand init_draw_command_;
#else
    DrawArraysIndirectCommand init_draw_command_;
#endif
    DispatchIndirectCommand     init_dispatch_command_;
    uint init_node_count_;

    // iterator stuff
    int primCount_delete_;
    int primCount_read_, primCount_write_;

    uint num_idx_;

    bool loadCounterBuffers()
    {
        uint zeros[NUM_ELEM] = {0};
        utility::EmptyBuffer(&buffers_[NodeCounterCulled]);
        glCreateBuffers(1, &buffers_[NodeCounterCulled]);
        glNamedBufferStorage(buffers_[NodeCounterCulled], NUM_ELEM * sizeof(uint),
                             (const void*)&zeros, 0);

        zeros[0] = init_node_count_;

        utility::EmptyBuffer(&buffers_[NodeCounterFull]);
        glCreateBuffers(1, &buffers_[NodeCounterFull]);
        glNamedBufferStorage(buffers_[NodeCounterFull], NUM_ELEM * sizeof(uint),
                             (const void*)&zeros, 0);

        return (glGetError() == GL_NO_ERROR);
    }

    bool loadCopyBuffer()
    {

        utility::EmptyBuffer(&buffers_[Copy]);
        glCreateBuffers(1, &buffers_[Copy]);
        uint zeros[NUM_ELEM] = {0};
        glNamedBufferStorage(buffers_[Copy], NUM_ELEM * sizeof(uint), &zeros,
                             GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
        return (glGetError() == GL_NO_ERROR);

    }

    bool loadTriangleDrawCommandBuffers()
    {
        utility::EmptyBuffer(&buffers_[DrawIndirect]);
        glCreateBuffers(1, &buffers_[DrawIndirect]);
        glNamedBufferStorage(buffers_[DrawIndirect], sizeof(init_draw_command_),
                             &init_draw_command_, 0);

        return (glGetError() == GL_NO_ERROR);
    }

    bool loadComputeCommandBuffer()
    {
        utility::EmptyBuffer (&buffers_[DispatchIndirect]);
        glCreateBuffers(1, &buffers_[DispatchIndirect]);
        glNamedBufferStorage(buffers_[DispatchIndirect], sizeof(init_dispatch_command_),
                             &init_dispatch_command_, 0);

        return (glGetError() == GL_NO_ERROR);
    }

    bool loadCommandBuffers()
    {
        bool b = true;
        b &= loadCopyBuffer();
        b &= loadCounterBuffers();
        b &= loadTriangleDrawCommandBuffers();
        b &= loadComputeCommandBuffer();
        return b;
    }

public:
    void Init(uint leaf_num_idx, uint num_workgroup, uint init_node_count)
    {
        num_idx_ = leaf_num_idx;
#ifdef ELEMENTS_INDIRECT
        init_draw_command_  = { GLuint(num_idx_),  0 , 0, 0, 0, uvec3(0)};
#else
        init_draw_command_  = { GLuint(num_idx_),  1 , 0, 0};
#endif
        init_dispatch_command_ = { GLuint(num_workgroup), 1, 1 };
        init_node_count_ = init_node_count;

        loadCommandBuffers();

        primCount_read_  = 0;
        primCount_write_ = 1;
        primCount_delete_ = floor(NUM_ELEM / 2);
    }

    void BindForCompute(GLuint program)
    {
        utility::SetUniformInt(program, "read_index", primCount_read_);
        utility::SetUniformInt(program, "write_index", primCount_write_);
        glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, NODECOUNTER_FULL_B, buffers_[NodeCounterFull]);
        glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, NODECOUNTER_CULLED_B, buffers_[NodeCounterCulled]);

        glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, buffers_[DispatchIndirect]);

        primCount_read_   = primCount_write_;
        primCount_write_  = (primCount_read_ + 1) % NUM_ELEM;
        primCount_delete_ = (primCount_delete_ + 1) % NUM_ELEM;
    }

    uint BindForCopy(GLuint program)
    {
        utility::SetUniformInt(program, "read_index", primCount_read_);
        utility::SetUniformInt(program, "delete_index", primCount_delete_);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, NODECOUNTER_FULL_B, buffers_[NodeCounterFull]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, NODECOUNTER_CULLED_B, buffers_[NodeCounterCulled]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, DISPATCH_INDIRECT_B, buffers_[DispatchIndirect]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, DRAW_INDIRECT_B, buffers_[DrawIndirect]);
    }

    void BindForRender()
    {
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffers_[DrawIndirect]);
    }

    int GetPrimCount()
    {
#ifdef ELEMENTS_INDIRECT
        glCopyNamedBufferSubData(buffers_[DrawIndirect], buffers_[Copy], sizeof(uint), 0, sizeof(uint));
        uint* data = (uint*) glMapNamedBuffer(buffers_[Copy], GL_READ_ONLY);
        glUnmapNamedBuffer(buffers_[Copy]);
        return data[0];
#else
        glCopyNamedBufferSubData(buffers_[DrawIndirect], buffers_[Copy], 0, 0, sizeof(uint));
        uint* data = (uint*) glMapNamedBuffer(buffers_[Copy], GL_READ_ONLY);
        glUnmapNamedBuffer(buffers_[Copy]);
        return data[0] / num_idx_;
#endif
    }

    void PrintAtomicArray()
    {
        cout << "AtomicArray: ";
        glCopyNamedBufferSubData(buffers_[NodeCounterFull], buffers_[Copy], 0, 0, NUM_ELEM * sizeof(uint));
        uint* data = (uint*) glMapNamedBuffer(buffers_[Copy], GL_READ_ONLY);
        glUnmapNamedBuffer(buffers_[Copy]);
        for (int i = 0; i < NUM_ELEM; ++i) {
            cout << data[i] << " ";
        }
        cout << endl;
    }

    void PrintWGCountInDispatch()
    {
        cout << "WG size X in Dispatch bufffer: ";
        glCopyNamedBufferSubData(buffers_[DispatchIndirect], buffers_[Copy], 0, 0, sizeof(uint));
        uint* data = (uint*) glMapNamedBuffer(buffers_[Copy], GL_READ_ONLY);
        glUnmapNamedBuffer(buffers_[Copy]);
        cout << data[0] << endl;
    }

    void ReloadCommands()
    {
        loadCommandBuffers();
    }

    void ReinitializeCommands(uint tri_num_idx, uint num_workgroup, uint init_node_count)
    {
        num_idx_ = tri_num_idx;
#ifdef ELEMENTS_INDIRECT
        init_draw_command_  = { GLuint(num_idx_),  0 , 0, 0, 0, uvec3(0)};
#else
        init_draw_command_  = { GLuint(num_idx_),  0 , 0, 0};
#endif
        init_dispatch_command_ = { GLuint(num_workgroup), 1, 1 };
        init_node_count_ = init_node_count;
        loadCommandBuffers();
    }

    void UpdateLeafGeometry(uint tri_num_v)
    {
        glNamedBufferSubData(buffers_[Copy], 0, sizeof(uint), &tri_num_v);
        glCopyNamedBufferSubData(buffers_[Copy], buffers_[DrawIndirect], 0, 0, sizeof(uint));
    }

    void Cleanup()
    {
        for (int i = 0; i < BUFFER_COUNT; ++i)
            utility::EmptyBuffer(&buffers_[i]);
    }

};

#endif
