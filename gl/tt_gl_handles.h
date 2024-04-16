#pragma once

#include "tt_gl.h"
#include "tt_messages.h"

namespace TT {
	struct Buffer {
		GLuint handle = 0;

		void alloc(size_t size, GLenum anchor = GL_SHADER_STORAGE_BUFFER, void* data = nullptr, GLenum mode = GL_STATIC_DRAW);
		void realloc(size_t size, GLenum anchor = GL_SHADER_STORAGE_BUFFER, void* data = nullptr, GLenum mode = GL_STATIC_DRAW) const;
		void cleanup();
	};

	struct VAO {
		GLuint handle = 0;

		void alloc();
		void cleanup();
	};

	struct Image {
	private:
		int geti(GLenum v) const;
		void defaults() const;
		void alloc();

	public:
		GLuint handle = 0;
		GLenum anchor = GL_TEXTURE_2D;

		int width() const;
		int height() const;
		int depth() const;

		void alloc(int width, int height, GLenum internalFormat = GL_RGBA32F, GLenum channels = GL_RGBA, GLenum elementType = GL_FLOAT, void* data = nullptr);
		void alloc(int width, int height, int depth, GLenum internalFormat, GLenum channels, GLenum elementType, void* data);
		void realloc(int width, int height, GLenum internalFormat = GL_RGBA32F, GLenum channels = GL_RGBA, GLenum elementType = GL_FLOAT, void* data = nullptr) const;
		void realloc(int width, int height, int depth, GLenum internalFormat, GLenum channels, GLenum elementType, void* data) const;
		void cleanup();
		void alloc(const char* filePath);
		void realloc(const char* filePath) const;

		void repeat() const;
		void clamp() const;
		void bind() const;
	};

	struct RenderTarget {
	private:
		unsigned int width = 0;
		unsigned int height = 0;
		unsigned int numCbos = 0;

	public:
		GLuint handle = 0;

		void alloc(std::initializer_list<const TT::Image*> colorBuffers = {}, TT::Image* depthStencilBuffer = nullptr, GLenum depthStencilMode = GL_DEPTH_ATTACHMENT);
		void cleanup();
		void use() const;
		static void restore(int windowWidth, int windowHeight);
	};
}
