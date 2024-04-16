#pragma once

#include "tt_gl_handles.h"
#include "stb/stb_image.h"

namespace {
	GLenum glChannels(int channels) {
		switch (channels) {
		case 1: return GL_RED;
		case 2: return GL_RG;
		case 3: return GL_RGB;
		case 4: return GL_RGBA;
		default: TT::assertFatal(false); return 0;
		}
	}
}

namespace TT {
	void Buffer::alloc(size_t size, GLenum anchor, void* data, GLenum mode) {
		assert(handle == 0);
		glGenBuffers(1, &handle);
		if (size)
			realloc(size, anchor, data, mode);
	}

	void Buffer::realloc(size_t size, GLenum anchor, void* data, GLenum mode) const {
		assert(handle != 0);
		glBindBuffer(anchor, handle);
		glBufferData(anchor, size, data, mode);
		glBindBuffer(anchor, 0);
	}

	void Buffer::cleanup() {
		if (handle) {
			glDeleteBuffers(1, &handle);
		}
		handle = 0;
	}



	void VAO::alloc() {
		assert(handle == 0);
		glGenVertexArrays(1, &handle);
	}

	void VAO::cleanup() {
		if (handle) {
			glDeleteVertexArrays(1, &handle);
		}
		handle = 0;
	}



	int Image::geti(GLenum v) const {
		int i;
		glBindTexture(anchor, handle); TT_GL_DBG_ERR;
		glGetTexLevelParameteriv(anchor, 0, v, &i); TT_GL_DBG_ERR;
		glBindTexture(anchor, 0); TT_GL_DBG_ERR;
		return i;
	}

	void Image::repeat() const {
		glBindTexture(anchor, handle); TT_GL_DBG_ERR;
		glTexParameteri(anchor, GL_TEXTURE_WRAP_S, GL_REPEAT); TT_GL_DBG_ERR;
		glTexParameteri(anchor, GL_TEXTURE_WRAP_T, GL_REPEAT); TT_GL_DBG_ERR;
		glTexParameteri(anchor, GL_TEXTURE_WRAP_R, GL_REPEAT); TT_GL_DBG_ERR;
		glBindTexture(anchor, 0); TT_GL_DBG_ERR;
	}

	void Image::clamp() const {
		glBindTexture(anchor, handle); TT_GL_DBG_ERR;
		glTexParameteri(anchor, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); TT_GL_DBG_ERR;
		glTexParameteri(anchor, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); TT_GL_DBG_ERR;
		glTexParameteri(anchor, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE); TT_GL_DBG_ERR;
		glBindTexture(anchor, 0); TT_GL_DBG_ERR;
	}

	void Image::defaults() const {
		glBindTexture(anchor, handle); TT_GL_DBG_ERR;
		glTexParameteri(anchor, GL_TEXTURE_WRAP_S, GL_REPEAT); TT_GL_DBG_ERR;
		glTexParameteri(anchor, GL_TEXTURE_WRAP_T, GL_REPEAT); TT_GL_DBG_ERR;
		glTexParameteri(anchor, GL_TEXTURE_WRAP_R, GL_REPEAT); TT_GL_DBG_ERR;
		glTexParameteri(anchor, GL_TEXTURE_MIN_FILTER, GL_LINEAR); TT_GL_DBG_ERR;
		glTexParameteri(anchor, GL_TEXTURE_MAG_FILTER, GL_LINEAR); TT_GL_DBG_ERR;
		glBindTexture(anchor, 0); TT_GL_DBG_ERR;
	}

	void Image::alloc() {
		assert(handle == 0);
		glGenTextures(1, &handle); TT_GL_DBG_ERR;
		defaults();
	}

	int Image::width() const { return geti(GL_TEXTURE_WIDTH); }
	int Image::height() const { return geti(GL_TEXTURE_HEIGHT); }
	int Image::depth() const { return geti(GL_TEXTURE_DEPTH); }

	void Image::alloc(int width, int height, GLenum internalFormat, GLenum channels, GLenum elementType, void* data) {
		alloc();
		realloc(width, height, internalFormat, channels, elementType, data);
	}

	void Image::alloc(int width, int height, int depth, GLenum internalFormat, GLenum channels, GLenum elementType, void* data) {
		alloc();
		realloc(width, height, depth, internalFormat, channels, elementType, data);
	}

	void Image::realloc(int width, int height, GLenum internalFormat, GLenum channels, GLenum elementType, void* data) const {
		assert(handle != 0);
		glBindTexture(anchor, handle); TT_GL_DBG_ERR;
		glTexImage2D(anchor, 0, internalFormat, width, height, 0, channels, elementType, data); TT_GL_DBG_ERR;
		glBindTexture(anchor, 0); TT_GL_DBG_ERR;
	}

	void Image::realloc(int width, int height, int depth, GLenum internalFormat, GLenum channels, GLenum elementType, void* data) const {
		assert(handle != 0);
		glBindTexture(anchor, handle); TT_GL_DBG_ERR;
		glTexImage3D(anchor, 0, internalFormat, width, height, depth, 0, channels, elementType, data);
		glBindTexture(anchor, 0); TT_GL_DBG_ERR;
	}

	void Image::cleanup() {
		if (handle) {
			glDeleteTextures(1, &handle); TT_GL_DBG_ERR;
		}
		handle = 0;
	}

	void Image::alloc(const char* filePath) {
		alloc();
		realloc(filePath);
	}

	void Image::bind() const {
		assert(handle != 0);
		glBindTexture(anchor, handle); TT_GL_DBG_ERR;
	}

	void Image::realloc(const char* filePath) const {
		int width, height, channels;
		unsigned char* data = stbi_load(filePath, &width, &height, &channels, 0);
		GLenum glChannels;
		switch (channels) {
		case 1: glChannels = GL_RED; break;
		case 2: glChannels = GL_RG; break;
		case 3: glChannels = GL_RGB; break;
		case 4: glChannels = GL_RGBA; break;
		default: error("Invalid image: %s", filePath); return;
		}
		GLenum internalFormat;
		switch (channels) {
		case 1: internalFormat = GL_R8; break;
		case 2: internalFormat = GL_RG8; break;
		case 3: internalFormat = GL_RGB8; break;
		case 4: internalFormat = GL_RGBA8; break;
		default: error("Invalid image: %s", filePath); return;
		}
		realloc(width, height, internalFormat, glChannels, GL_UNSIGNED_BYTE, data);
		stbi_image_free(data);
	}

	void RenderTarget::alloc(std::initializer_list<const TT::Image*> colorBuffers, TT::Image* depthStencilBuffer, GLenum depthStencilMode) {
		assert(handle == 0);
		glGenFramebuffers(1, &handle);
		glBindFramebuffer(GL_FRAMEBUFFER, handle);

		int i = 0;
		for (const TT::Image* image : colorBuffers) {
			assert(image);
			if (width == 0) {
				width = image->width();
				height = image->height();
			}
			else {
				assert(width == image->width());
				assert(height == image->height());
			}
			assert(image->handle);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i++, GL_TEXTURE_2D, image->handle, 0);
		}
		numCbos = i;

		if (depthStencilBuffer) {
			assert(depthStencilBuffer->handle);
			if (width == 0) {
				width = depthStencilBuffer->width();
				height = depthStencilBuffer->height();
			}
			else {
				assert(width == depthStencilBuffer->width());
				assert(height == depthStencilBuffer->height());
			}
			glFramebufferTexture2D(GL_FRAMEBUFFER, depthStencilMode, GL_TEXTURE_2D, depthStencilBuffer->handle, 0);
		}

		assert(width != 0);
		assert(height != 0);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void RenderTarget::cleanup() {
		if (handle)
			glDeleteFramebuffers(1, &handle);
		handle = 0;
	}

	namespace {
		const GLenum drawBufferEnums[32] = {
			GL_COLOR_ATTACHMENT0 , GL_COLOR_ATTACHMENT1 , GL_COLOR_ATTACHMENT2 , GL_COLOR_ATTACHMENT3,
			GL_COLOR_ATTACHMENT4 , GL_COLOR_ATTACHMENT5 , GL_COLOR_ATTACHMENT6 , GL_COLOR_ATTACHMENT7,
			GL_COLOR_ATTACHMENT8 , GL_COLOR_ATTACHMENT9 , GL_COLOR_ATTACHMENT10, GL_COLOR_ATTACHMENT11,
			GL_COLOR_ATTACHMENT12, GL_COLOR_ATTACHMENT13, GL_COLOR_ATTACHMENT14, GL_COLOR_ATTACHMENT15,
			GL_COLOR_ATTACHMENT16, GL_COLOR_ATTACHMENT17, GL_COLOR_ATTACHMENT18, GL_COLOR_ATTACHMENT19,
			GL_COLOR_ATTACHMENT20, GL_COLOR_ATTACHMENT21, GL_COLOR_ATTACHMENT22, GL_COLOR_ATTACHMENT23,
			GL_COLOR_ATTACHMENT24, GL_COLOR_ATTACHMENT25, GL_COLOR_ATTACHMENT26, GL_COLOR_ATTACHMENT27,
			GL_COLOR_ATTACHMENT28, GL_COLOR_ATTACHMENT29, GL_COLOR_ATTACHMENT30, GL_COLOR_ATTACHMENT31,
		};
	}

	void RenderTarget::use() const {
		assert(handle != 0);
		glBindFramebuffer(GL_FRAMEBUFFER, handle);
		glViewport(0, 0, width, height);
		glDrawBuffers(numCbos, drawBufferEnums);
	}

	void RenderTarget::restore(int windowWidth, int windowHeight) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, windowWidth, windowHeight);
	}
}
