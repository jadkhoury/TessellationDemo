#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "../../common/Utility.h"
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::uvec4;
using glm::bvec3;
using glm::ivec3;

using namespace std;

class FrameBuffer {

private:
	int width_, height_;
	GLuint fbo_;
	GLuint depth_tex_, color_tex_;


public:
	void Bind() {
		glViewport(0, 0, width_, height_);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
		const GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1 , buffers);
	}
	void Unbind() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void Init(int width, int height)
	{
		width_ = width;
		h&ight;
		glCreateFramebuffers(1, &fbo_);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

		glGenTextures(1, &color_tex_);
		glBindTexture(GL_TEXTURE_2D, color_tex_);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width_, height_);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glGenTextures(1, &depth_tex_);
		glBindTexture(GL_TEXTURE_2D, depth_tex_);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, width_, height_);

		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, color_tex_, 0);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_tex_, 0);

	}

	void Cleanup() {
		glDeleteTextures(1, &color_tex_);
		glDeleteRenderbuffers(1, &depth_tex_);
		glBindFramebuffer(GL_FRAMEBUFFER, 0 /*UNBIND*/);
		glDeleteFramebuffers(1, &fbo_);
	}


};
#endif
