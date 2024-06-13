#pragma once

// #include "gl/tt_render_concepts.h"
#include "../ThirdParty/fontstash/fontstash.h"
#include "../tt_rendering.h"

FONScontext* glfonsCreate(int width, int height, int flags, TTRendering::RenderingContext* renderContext);
void glfonsDelete(FONScontext* ctx);
// void glFonsSetResolution(FONScontext* ctx, int width, int height);
const TTRendering::ImageHandle& glFonsAtlas(FONScontext* ctx);
unsigned int glfonsRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a);

#ifdef GLFONTSTASH_IMPLEMENTATION
#include "fontstash/fontstash.h"
#include "tt_glcontext.h"

namespace {
	struct FonsGLContext {
        TTRendering::RenderingContext* context;
        TTRendering::ImageHandle image;

        FonsGLContext(TTRendering::RenderingContext* context) : 
            context(context), 
            image(context->createImage(64, 64, TTRendering::ImageFormat::R8))
        {}
	};

	int _renderCreate(void* userPtr, int width, int height) {
		FonsGLContext& context = *((FonsGLContext*)userPtr);
        context.context->resizeImage(context.image, width, height);
		return 1;
	}

	int _renderResize(void* userPtr, int width, int height) {
        FonsGLContext& context = *((FonsGLContext*)userPtr);
        context.context->resizeImage(context.image, width, height);
		return 1;
	}

	void _renderUpdate(void* userPtr, int* rect, const unsigned char* data) {
        FonsGLContext& context = *((FonsGLContext*)userPtr);

		int w = rect[2] - rect[0];
		int h = rect[3] - rect[1];

        // TODO: Make renderer agnostic somehow
		int width;
		glBindTexture(GL_TEXTURE_2D, (GLuint)context.image.identifier()); TT_GL_DBG_ERR;
		
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width); TT_GL_DBG_ERR;
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1); TT_GL_DBG_ERR;
		glPixelStorei(GL_UNPACK_ROW_LENGTH, width); TT_GL_DBG_ERR;
		glPixelStorei(GL_UNPACK_SKIP_PIXELS, rect[0]); TT_GL_DBG_ERR;
		glPixelStorei(GL_UNPACK_SKIP_ROWS, rect[1]); TT_GL_DBG_ERR;
        
        glTexSubImage2D(GL_TEXTURE_2D, 0, rect[0], rect[1], w, h, GL_RED, GL_UNSIGNED_BYTE, data); TT_GL_DBG_ERR;
		glBindTexture(GL_TEXTURE_2D, 0); TT_GL_DBG_ERR;
        
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4); TT_GL_DBG_ERR;
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0); TT_GL_DBG_ERR;
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0); TT_GL_DBG_ERR;
        glPixelStorei(GL_UNPACK_SKIP_ROWS, 0); TT_GL_DBG_ERR;
	}

	void _renderDraw(void* userPtr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts) {
        // TODO: not supported (yet?)
	}

	void _renderDelete(void* userPtr) {
        // TODO: not supported yet
	}
}

FONScontext* glfonsCreate(int width, int height, int flags, TTRendering::RenderingContext* renderContext) {
	FonsGLContext* context = new FonsGLContext(renderContext);

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

void glfonsDelete(FONScontext* ctx) {
	fonsDeleteInternal(ctx);
}

unsigned int glfonsRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
	return (r) | (g << 8) | (b << 16) | (a << 24);
}

const TTRendering::ImageHandle& glFonsAtlas(FONScontext* ctx) { 
    FonsGLContext& context = *(FonsGLContext*)(ctx->params.userPtr);
    return context.image;
}
#endif
