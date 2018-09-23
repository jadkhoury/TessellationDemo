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
     } Bindings;

typedef struct {
    GLuint  count;
    GLuint  nodeCount;
    GLuint  firstIndex;
    GLuint  baseVertex;
    GLuint  baseInstance;
    uvec3  align;
} DrawElementsIndirectCommand;

typedef struct {
    GLuint  num_groups_x;
    GLuint  num_groups_y;
    GLuint  num_groups_z;
} DispatchIndirectCommand;

class CommandManager
{
private:

    // Buffer indices for the ...
    enum {
        DrawIndirect,      // Draw command
        DispatchIndirect,  // Dispatch command
        NodeCounterFull,   // Pingpong atomic counters for unculled nodes
        NodeCounterCulled, // Pingpong atomic counters for all nodes
        Proxy,              // Proxy buffer used to read back from GPU
        BUFFER_COUNT
    };

    GLuint buffers_[BUFFER_COUNT]; // Array of buffers
    DrawElementsIndirectCommand init_draw_;
    DispatchIndirectCommand     init_dispatch_;

    int counters_read; // pingpong idx for counters
    uint num_idx_; // Number of vertex indices for the current leaf geometry


    // Loads the buffer that will contain the atomic counter array
    bool loadCounterBuffers()
    {
        uint zeros[2] = {0};
        utility::EmptyBuffer(&buffers_[NodeCounterCulled]);
        glCreateBuffers(1, &buffers_[NodeCounterCulled]);
        glNamedBufferStorage(buffers_[NodeCounterCulled], 2 * sizeof(uint),
                             (const void*)&zeros, 0);

        utility::EmptyBuffer(&buffers_[NodeCounterFull]);
        glCreateBuffers(1, &buffers_[NodeCounterFull]);
        glNamedBufferStorage(buffers_[NodeCounterFull], 2 * sizeof(uint),
                             (const void*)&zeros, 0);

        return (glGetError() == GL_NO_ERROR);
    }

    bool loadProxyBuffer()
    {
        utility::EmptyBuffer(&buffers_[Proxy]);
        glCreateBuffers(1, &buffers_[Proxy]);
        uint zeros[4] = {0};
        glNamedBufferStorage(buffers_[Proxy], 2 * sizeof(uint), &zeros,
                             GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT);
        return (glGetError() == GL_NO_ERROR);

    }

    bool loadDrawCommandBuffers()
    {
        utility::EmptyBuffer(&buffers_[DrawIndirect]);
        glCreateBuffers(1, &buffers_[DrawIndirect]);
        glNamedBufferStorage(buffers_[DrawIndirect], sizeof(init_draw_),
                             &init_draw_, 0);
        return (glGetError() == GL_NO_ERROR);
    }

    bool loadComputeCommandBuffer()
    {
        utility::EmptyBuffer (&buffers_[DispatchIndirect]);
        glCreateBuffers(1, &buffers_[DispatchIndirect]);
        glNamedBufferStorage(buffers_[DispatchIndirect], sizeof(init_dispatch_),
                             &init_dispatch_, 0);
        return (glGetError() == GL_NO_ERROR);
    }

    bool loadCommandBuffers()
    {
        bool b = true;
        b &= loadProxyBuffer();
        b &= loadCounterBuffers();
        b &= loadDrawCommandBuffers();
        b &= loadComputeCommandBuffer();
        return b;
    }

public:
    void Init(uint leaf_num_idx, uint num_workgroup)
    {
        num_idx_ = leaf_num_idx;
        init_draw_  = { GLuint(num_idx_),  0 , 0, 0, 0, uvec3(0)};
        init_dispatch_ = { GLuint(num_workgroup), 1, 1 };
        counters_read  = 0;
        loadCommandBuffers();
    }

    // Binds the relevant buffers for the compute pass
    // Updates the pingpong index
    void BindForCompute(GLuint program)
    {
        utility::SetUniformInt(program, "u_read_index", counters_read);
        glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, NODECOUNTER_FULL_B,
                         buffers_[NodeCounterFull]);
        glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, NODECOUNTER_CULLED_B,
                         buffers_[NodeCounterCulled]);
        glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, buffers_[DispatchIndirect]);
        counters_read   = 1 - counters_read;
    }

    // Binds the relevant buffers for the copy pass
    void BindForCopy(GLuint program)
    {
        utility::SetUniformInt(program, "u_read_index", counters_read);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, NODECOUNTER_FULL_B,
                         buffers_[NodeCounterFull]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, NODECOUNTER_CULLED_B,
                         buffers_[NodeCounterCulled]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, DISPATCH_INDIRECT_B,
                         buffers_[DispatchIndirect]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, DRAW_INDIRECT_B,
                         buffers_[DrawIndirect]);
    }

    // Binds the relevant buffers for the render pass
    void BindForRender()
    {
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffers_[DrawIndirect]);
    }

    // Return the number of nodes to render
    int GetDrawnNodeCount()
    {
        glCopyNamedBufferSubData(buffers_[NodeCounterCulled], buffers_[Proxy],
                                 sizeof(uint)*counters_read, 0, sizeof(uint));
        uint* data = (uint*) glMapNamedBuffer(buffers_[Proxy], GL_READ_ONLY);
        glUnmapNamedBuffer(buffers_[Proxy]);
        return data[0];
    }

    // Return the total number of nodes in the compute buffer
    int GetFullNodeCount()
    {
        glCopyNamedBufferSubData(buffers_[NodeCounterFull], buffers_[Proxy],
                                 sizeof(uint)*counters_read, 0, sizeof(uint));
        uint* data = (uint*) glMapNamedBuffer(buffers_[Proxy], GL_READ_ONLY);
        glUnmapNamedBuffer(buffers_[Proxy]);
        return data[0];
    }

    // Print the number of workgroup in the Dispatch command buffer
    void PrintWGCountInDispatch()
    {
        cout << "WG size X in Dispatch bufffer: ";
        glCopyNamedBufferSubData(buffers_[DispatchIndirect], buffers_[Proxy],
                                 0, 0, sizeof(uint));
        uint* data = (uint*) glMapNamedBuffer(buffers_[Proxy], GL_READ_ONLY);
        glUnmapNamedBuffer(buffers_[Proxy]);
        cout << data[0] << endl;
    }

    void Cleanup()
    {
        for (int i = 0; i < BUFFER_COUNT; ++i)
            utility::EmptyBuffer(&buffers_[i]);
    }

};

#endif
