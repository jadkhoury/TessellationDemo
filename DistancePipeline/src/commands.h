#ifndef COMMANDS_H
#define COMMANDS_H

#include "utility.h"
#include <glm/glm.hpp>

enum {QUADS, TRIANGLES, NUM_TYPES};

using namespace std;
using glm::uvec3;
using glm::uvec4;

typedef struct {
	GLuint  count;
	GLuint  primCount;
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

class Commands
{
private:
	const int NUM_ELEM = 8;

	GLuint quad_draw_bo_, tri_draw_bo_;
	GLuint dispatch_bo_;
    GLuint default_bo_, readback_bo_;

    DrawElementsIndirectCommand init_quad_command_;
    DrawElementsIndirectCommand init_tri_command_;
	DispatchIndirectCommand     init_dispatch_command_;

    DrawElementsIndirectCommand* quad_command_array_;
    DrawElementsIndirectCommand* tri_command_array_;

	// iterator stuff
	bool first_frame_;
	int  delete_idx_;
	int compute_R, compute_W;
	int cull_R, cull_W;
	int render_R;

	GLuint draw_bo_;
	GLsizei command_size_;

    //test
    uint num_prim_;

	bool loadDefaultBuffer()
	{
        if(glIsBuffer(default_bo_))
            glDeleteBuffers(1, &default_bo_);
        glCreateBuffers(1, &default_bo_);
		uint ui = 0;
        glNamedBufferStorage(default_bo_, sizeof(uint), (const void*)&ui, 0);
		return (glGetError() == GL_NO_ERROR);

	}

	bool loadReadbackBuffer()
	{
		if(glIsBuffer(readback_bo_))
			glDeleteBuffers(1, &readback_bo_);
		glCreateBuffers(1, &readback_bo_);
		uint ui = 0;
		glNamedBufferStorage(readback_bo_, sizeof(uint), (const void*)&ui,
							 GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT);
		return (glGetError() == GL_NO_ERROR);

	}

	bool loadQuadDrawCommandBuffers()
	{
		quad_command_array_ = new DrawElementsIndirectCommand[NUM_ELEM];
		for (int i = 0; i < NUM_ELEM; ++i) {
			quad_command_array_[i] = init_quad_command_;
		}

		if(glIsBuffer(quad_draw_bo_))
			glDeleteBuffers(1, &quad_draw_bo_);
		glCreateBuffers(1, &quad_draw_bo_);
		glNamedBufferStorage(quad_draw_bo_, NUM_ELEM * sizeof(quad_command_array_[0]),
				quad_command_array_, 0);

		return (glGetError() == GL_NO_ERROR);
	}

	bool loadTriangleDrawCommandBuffers()
    {
        tri_command_array_ = new DrawElementsIndirectCommand[NUM_ELEM];
        for (int i = 0; i < NUM_ELEM; ++i) {
			tri_command_array_[i] = init_tri_command_;
		}

		if(glIsBuffer(tri_draw_bo_))
			glDeleteBuffers(1, &tri_draw_bo_);
		glCreateBuffers(1, &tri_draw_bo_);
		glNamedBufferStorage(tri_draw_bo_, NUM_ELEM * sizeof(tri_command_array_[0]),
				tri_command_array_, 0);

		return (glGetError() == GL_NO_ERROR);
	}

	bool loadComputeCommandBuffer()
	{
		if(glIsBuffer(dispatch_bo_))
			glDeleteBuffers(1, &dispatch_bo_);
		glCreateBuffers(1, &dispatch_bo_);
		glNamedBufferStorage(dispatch_bo_, sizeof(init_dispatch_command_), &init_dispatch_command_,
							 0);

		return (glGetError() == GL_NO_ERROR);
	}

	bool loadCommandBuffers()
	{
		bool b = true;
		b &= loadDefaultBuffer();
		b &= loadReadbackBuffer();
		b &= loadQuadDrawCommandBuffers();
		b &= loadTriangleDrawCommandBuffers();
		b &= loadComputeCommandBuffer();
		return b;
	}

	void readPrimCount(GLuint buffer, int idx)
	{

		glCopyNamedBufferSubData(buffer, readback_bo_,
								 idx * command_size_ + sizeof(uint), 0, sizeof(uint));

		uint* data = (uint*) glMapNamedBuffer(readback_bo_, GL_READ_ONLY);
		{
			num_prim_ = *data;
		}
		glUnmapNamedBuffer(readback_bo_);
	}

public:
	void Init(uint quad_num_idx, uint tri_num_v, uint num_workgroup)
	{
        init_quad_command_ = { GLuint(quad_num_idx), 0u , 0u, 0u, 0u, uvec3(0)};
        init_tri_command_  = { GLuint(tri_num_v),  0u , 0u, 0u, 0u, uvec3(0)};
        init_dispatch_command_ = { GLuint(num_workgroup), 1u, 1u };
		compute_R = 0;
		first_frame_ = true;
		delete_idx_ = 0;
		command_size_ = 8 * sizeof(uint);
		num_prim_ = 0;
		loadCommandBuffers();
	}

	void BindForCompute(uint read_binding, uint write_binding, int prim_type)
    {
        if(prim_type == TRIANGLES){
            draw_bo_ = tri_draw_bo_;
        } else if (prim_type == QUADS){
            draw_bo_ = quad_draw_bo_;
        }

		if(first_frame_){
			compute_R = 0;
			compute_W = 1;
			first_frame_ = false;
		} else {
			compute_R = compute_W;
			compute_W = (compute_R + 2) % NUM_ELEM;
		}

		glBindBufferRange(GL_SHADER_STORAGE_BUFFER, read_binding,
						  draw_bo_, compute_R * command_size_, command_size_);
		glBindBufferRange(GL_SHADER_STORAGE_BUFFER, write_binding,
						  draw_bo_, compute_W * command_size_, command_size_);

		glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, dispatch_bo_);

        glCopyNamedBufferSubData(default_bo_, draw_bo_,
                                 0, delete_idx_ * command_size_ + sizeof(uint),
                                 sizeof(uint));
        glCopyNamedBufferSubData(default_bo_, draw_bo_,
                                 0, (delete_idx_ + 1) * command_size_ + sizeof(uint),
                                 sizeof(uint));
		delete_idx_ = (delete_idx_ + 2) % NUM_ELEM;


	}

	void BindForCull(uint read_binding, uint write_binding, int prim_type)
	{
		if(prim_type == TRIANGLES){
			draw_bo_ = tri_draw_bo_;
		} else if (prim_type == QUADS){
			draw_bo_ = quad_draw_bo_;
		}

		cull_R = compute_W;
		cull_W = (cull_R + 1) % NUM_ELEM;

		glBindBufferRange(GL_SHADER_STORAGE_BUFFER, read_binding,
						  draw_bo_, cull_R * command_size_, command_size_);
		glBindBufferRange(GL_SHADER_STORAGE_BUFFER, write_binding,
						  draw_bo_, cull_W * command_size_, command_size_);

		glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, dispatch_bo_);
	}



	//returns the offset for the draw function
	uint BindForRender(int prim_type)
	{
		if(prim_type == TRIANGLES){
			draw_bo_ = tri_draw_bo_;
		} else if (prim_type == QUADS){
			draw_bo_ = quad_draw_bo_;
		}
		render_R = cull_W;
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, draw_bo_);
		return  command_size_ * render_R;
	}

	int getPrimCount()
	{
		readPrimCount(draw_bo_, compute_W);
		return num_prim_;
	}

	void ReloadCommands()
	{
		loadCommandBuffers();
	}

	void ReinitializeCommands(uint quad_num_idx, uint tri_num_v, uint num_workgroup)
	{
		init_quad_command_ = { GLuint(quad_num_idx), 0u , 0u, 0u, 0u, uvec3(0)};
		init_tri_command_  = { GLuint(tri_num_v),  0u , 0u, 0u, 0u, uvec3(0)};
		init_dispatch_command_ = { GLuint(num_workgroup), 1u, 1u };
		loadCommandBuffers();

	}

	void Cleanup()
	{
		glDeleteBuffers(1, &tri_draw_bo_);
		glDeleteBuffers(1, &quad_draw_bo_);
		delete [] tri_command_array_;
		delete [] quad_command_array_;
	}

};

#endif
