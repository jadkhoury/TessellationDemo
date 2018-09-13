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
        NodeCounterFull,   // Array of atomic counters of unculled nodes
        NodeCounterCulled, // Array of atomic counters of all nodes
        Proxy,              // Proxy buffer used to read back from GPU
        BUFFER_COUNT
    };
    static const int NUM_ELEM = 16; // Number of atomic counters in the array
    GLuint buffers_[BUFFER_COUNT]; // Array of buffers
    DrawElementsIndirectCommand init_draw_command_;
    DispatchIndirectCommand     init_dispatch_command_;
    uint init_node_count_; // Number of nodes when starting the program

    // indices of the atomic array
    int nodeCount_delete_;
    int nodeCount_read_, nodeCount_write_;

    uint num_idx_; // Number of vertex indices for the current leaf geometry


    // Loads the buffer that will contain the atomic counter array
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
        utility::EmptyBuffer(&buffers_[Proxy]);
        glCreateBuffers(1, &buffers_[Proxy]);
        uint zeros[NUM_ELEM] = {0};
        glNamedBufferStorage(buffers_[Proxy], NUM_ELEM * sizeof(uint), &zeros,
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
        init_draw_command_  = { GLuint(num_idx_),  0 , 0, 0, 0, uvec3(0)};
        init_dispatch_command_ = { GLuint(num_workgroup), 1, 1 };
        init_node_count_ = init_node_count;

        loadCommandBuffers();

        nodeCount_read_  = 0;
        nodeCount_write_ = 1;
        nodeCount_delete_ = floor(NUM_ELEM / 2);
    }

    // Binds the relevant buffers for the compute pass
    // Uploads the atomic array indices
    // Update the atomic counters indices
    void BindForCompute(GLuint program)
    {
        utility::SetUniformInt(program, "u_read_index", nodeCount_read_);
        utility::SetUniformInt(program, "u_write_index", nodeCount_write_);
        glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, NODECOUNTER_FULL_B, buffers_[NodeCounterFull]);
        glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, NODECOUNTER_CULLED_B, buffers_[NodeCounterCulled]);

        glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, buffers_[DispatchIndirect]);

        nodeCount_read_   = nodeCount_write_;
        nodeCount_write_  = (nodeCount_read_ + 1) % NUM_ELEM;
        nodeCount_delete_ = (nodeCount_delete_ + 1) % NUM_ELEM;
    }

    // Binds the relevant buffers for the copy pass
    // Uploads the atomic array indices
    void BindForCopy(GLuint program)
    {
        utility::SetUniformInt(program, "u_read_index", nodeCount_read_);
        utility::SetUniformInt(program, "u_delete_index", nodeCount_delete_);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, NODECOUNTER_FULL_B, buffers_[NodeCounterFull]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, NODECOUNTER_CULLED_B, buffers_[NodeCounterCulled]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, DISPATCH_INDIRECT_B, buffers_[DispatchIndirect]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, DRAW_INDIRECT_B, buffers_[DrawIndirect]);
    }

    void BindForRender()
    {
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffers_[DrawIndirect]);
    }

    // Return the number of nodes to render stored in the current Draw command
    // To help asynchronicity, first copy from command buffer to proxy buffer, then from proxy to CPU
    int GetDrawnNodeCount()
    {
        glCopyNamedBufferSubData(buffers_[DrawIndirect], buffers_[Proxy],
                                 sizeof(uint), 0, sizeof(uint));
        uint* data = (uint*) glMapNamedBuffer(buffers_[Proxy], GL_READ_ONLY);
        glUnmapNamedBuffer(buffers_[Proxy]);
        return data[0];
    }

    int GetFullNodeCount()
    {
        glCopyNamedBufferSubData(buffers_[NodeCounterFull], buffers_[Proxy],
                                 sizeof(uint)*nodeCount_read_, 0, sizeof(uint));

        uint* data = (uint*) glMapNamedBuffer(buffers_[Proxy], GL_READ_ONLY);
        glUnmapNamedBuffer(buffers_[Proxy]);
        return data[0];
    }

    // Print the content of the atomic counter array
    void PrintAtomicArray()
    {
        cout << "AtomicArray: ";
        glCopyNamedBufferSubData(buffers_[NodeCounterFull], buffers_[Proxy], 0, 0, NUM_ELEM * sizeof(uint));
        uint* data = (uint*) glMapNamedBuffer(buffers_[Proxy], GL_READ_ONLY);
        glUnmapNamedBuffer(buffers_[Proxy]);
        for (int i = 0; i < NUM_ELEM; ++i) {
            cout << data[i] << " ";
        }
        cout << endl;
    }

    // Print the number of workgroup in the Dispatch command buffer
    void PrintWGCountInDispatch()
    {
        cout << "WG size X in Dispatch bufffer: ";
        glCopyNamedBufferSubData(buffers_[DispatchIndirect], buffers_[Proxy], 0, 0, sizeof(uint));
        uint* data = (uint*) glMapNamedBuffer(buffers_[Proxy], GL_READ_ONLY);
        glUnmapNamedBuffer(buffers_[Proxy]);
        cout << data[0] << endl;
    }

    void ReloadCommands()
    {
        loadCommandBuffers();
    }

    // Update the command buffer to match a new leaf geometry
    // (i.e. different CPU LoD => different number of indices )
    void UpdateLeafGeometry(uint tri_num_v)
    {
        glNamedBufferSubData(buffers_[Proxy], 0, sizeof(uint), &tri_num_v);
        glCopyNamedBufferSubData(buffers_[Proxy], buffers_[DrawIndirect], 0, 0, sizeof(uint));
    }

    void Cleanup()
    {
        for (int i = 0; i < BUFFER_COUNT; ++i)
            utility::EmptyBuffer(&buffers_[i]);
    }

};

#endif
