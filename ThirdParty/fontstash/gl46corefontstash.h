#pragma once

#include "tt_render_concepts.h"

FONScontext* glfonsCreate(int width, int height, int flags);
void glfonsDelete(FONScontext* ctx);
void glFonsSetResolution(FONScontext* ctx, int width, int height);
unsigned int glfonsRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a);

#ifdef GLFONTSTASH_IMPLEMENTATION
#include "fontstash/fontstash.h"

namespace {
	struct Context {
		TT::Image image;
		TT::Material material;
		TT::Buffer verts;
		TT::Buffer coords;
		TT::Buffer colors;
		TT::VAO vao;
	};

	int _renderCreate(void* userPtr, int width, int height) {
		Context& context = *((Context*)userPtr);
		context.image.alloc(width, height, GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr);

		context.material.program = TT::ProgramManager::fetchProgram({ "fontStash.vert.glsl", "fontStash.frag.glsl" });
		// context.material.set("uImage", context.image);

		context.verts.alloc(FONS_VERTEX_COUNT * sizeof(float) * 2, GL_ARRAY_BUFFER, nullptr, GL_DYNAMIC_DRAW);
		context.coords.alloc(FONS_VERTEX_COUNT * sizeof(float) * 2, GL_ARRAY_BUFFER, nullptr, GL_DYNAMIC_DRAW);
		context.colors.alloc(FONS_VERTEX_COUNT * sizeof(float) * 2, GL_ARRAY_BUFFER, nullptr, GL_DYNAMIC_DRAW);

		context.vao.alloc();

		glBindVertexArray(context.vao.handle);

		glBindBuffer(GL_ARRAY_BUFFER, context.verts.handle);
		glEnableVertexAttribArray(0);
		TT::vertexAttribPointer(0, 2, GL_FLOAT, false, 2 * sizeof(float), nullptr);

		glBindBuffer(GL_ARRAY_BUFFER, context.coords.handle);
		glEnableVertexAttribArray(1);
		TT::vertexAttribPointer(1, 2, GL_FLOAT, false, 2 * sizeof(float), nullptr);

		glBindBuffer(GL_ARRAY_BUFFER, context.colors.handle);
		glEnableVertexAttribArray(2);
		TT::vertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, false, 4, nullptr);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		return 1;
	}

	int _renderResize(void* userPtr, int width, int height) {
		TT::Image& image = ((Context*)userPtr)->image;
		image.realloc(width, height, GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr);
		return 1;
	}

	void _renderUpdate(void* userPtr, int* rect, const unsigned char* data) {
		TT::Image& image = ((Context*)userPtr)->image;
#if 0
		int w = image.width();
		int h = image.height();
		image.realloc(image.width(), image.height(), GL_R8, GL_RED, GL_UNSIGNED_BYTE, (void*)data);
#else
		int w = rect[2] - rect[0];
		int h = rect[3] - rect[1];

		int width;
		glBindTexture(image.anchor, image.handle); TT_GL_DBG_ERR;
		glGetTexLevelParameteriv(image.anchor, 0, GL_TEXTURE_WIDTH, &width); TT_GL_DBG_ERR;
		// glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT); TT_GL_DBG_ERR;

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1); TT_GL_DBG_ERR;
		glPixelStorei(GL_UNPACK_ROW_LENGTH, width); TT_GL_DBG_ERR;
		glPixelStorei(GL_UNPACK_SKIP_PIXELS, rect[0]); TT_GL_DBG_ERR;
		glPixelStorei(GL_UNPACK_SKIP_ROWS, rect[1]); TT_GL_DBG_ERR;

		glTexSubImage2D(image.anchor, 0, rect[0], rect[1], w, h, GL_RED, GL_UNSIGNED_BYTE, data); TT_GL_DBG_ERR;

		// glPopClientAttrib(); TT_GL_DBG_ERR;

		glBindTexture(image.anchor, 0); TT_GL_DBG_ERR;
#endif
	}

	void _renderDraw(void* userPtr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts) {
		Context& context = *((Context*)userPtr);
		context.material.set("uImage", context.image);
		context.material.use();

#if 0
		context.verts.realloc(nverts * 2 * sizeof(float), GL_ARRAY_BUFFER, (void*)verts);
		context.coords.realloc(nverts * 2 * sizeof(float), GL_ARRAY_BUFFER, (void*)tcoords);
		context.colors.realloc(nverts * 4, GL_ARRAY_BUFFER, (void*)colors);
#else
		{
			glBindBuffer(GL_ARRAY_BUFFER, context.verts.handle);
			void* buf = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
			memcpy(buf, (const void*)verts, sizeof(float) * 2 * nverts);
			glUnmapBuffer(GL_ARRAY_BUFFER);
		}

		{
			glBindBuffer(GL_ARRAY_BUFFER, context.coords.handle);
			void* buf = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
			memcpy(buf, (const void*)tcoords, sizeof(float) * 2 * nverts);
			glUnmapBuffer(GL_ARRAY_BUFFER);
		}

		{
			glBindBuffer(GL_ARRAY_BUFFER, context.colors.handle);
			void* buf = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
			memcpy(buf, (const void*)colors, 4 * nverts);
			glUnmapBuffer(GL_ARRAY_BUFFER);
		}
#endif

		glBindVertexArray(context.vao.handle);
		glDrawArrays(GL_TRIANGLES, 0, nverts); TT_GL_DBG_ERR;
		glBindVertexArray(0);
	}

	void _renderDelete(void* userPtr) {
		TT::Image& image = ((Context*)userPtr)->image;
		image.cleanup();
		delete (Context*)userPtr;
	}
}

FONScontext* glfonsCreate(int width, int height, int flags) {
	Context* context = new Context;

	FONSparams params = {
		.width = width,
		.height = height,
		.flags = (unsigned char)flags,
		.userPtr = context,
		.renderCreate = _renderCreate,
		.renderResize = _renderResize,
		.renderUpdate = _renderUpdate,
		.renderDraw = _renderDraw,
		.renderDelete = _renderDelete,
	};

	return fonsCreateInternal(&params);
}

void glFonsSetResolution(FONScontext* ctx, int width, int height) {
	((Context*)ctx->params.userPtr)->material.set("uResolution", { (float)width, (float)height });
}

void glfonsDelete(FONScontext* ctx) {
	fonsDeleteInternal(ctx);
}

unsigned int glfonsRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
	return (r) | (g << 8) | (b << 16) | (a << 24);
}
#endif
