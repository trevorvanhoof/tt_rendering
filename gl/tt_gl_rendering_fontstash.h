#pragma once

// #include "gl/tt_render_concepts.h"
#include "tt_rendering.h"

FONScontext* glfonsCreate(int width, int height, int flags, TTRendering::RenderingContext* renderContext);
void glfonsDelete(FONScontext* ctx);
void glFonsSetResolution(FONScontext* ctx, int width, int height);
unsigned int glfonsRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a);

#ifdef GLFONTSTASH_IMPLEMENTATION
#include "fontstash/fontstash.h"

namespace {
	struct Context {
        TTRendering::RenderingContext* context;
        TTRendering::ImageHandle image;
        TTRendering::MaterialHandle material;
        TTRendering::BufferHandle vbo;
        TTRendering::MeshHandle vao;
        TTRendering::RenderPass renderPass {};

        Context(TTRendering::RenderingContext* context) : 
            context(context), 
            image(context->createImage(64, 64, TTRendering::ImageFormat::R8)) ,
            material(context->createMaterial(context->fetchShader({context->fetchShaderStage("fontStash.vert.glsl"), context->fetchShaderStage("fontStash.frag.glsl")}))),
            vbo(context->createBuffer(FONS_VERTEX_COUNT * sizeof(float) * 6, nullptr, TTRendering::BufferMode::StaticDraw)),
            vao(context->createMesh({{TTRendering::MeshAttribute::Dimensions::D2, TTRendering::MeshAttribute::ElementType::F32, 0}, {TTRendering::MeshAttribute::Dimensions::D2, TTRendering::MeshAttribute::ElementType::F32, 1}, {TTRendering::MeshAttribute::Dimensions::D2, TTRendering::MeshAttribute::ElementType::F32, 2}}, vbo, FONS_VERTEX_COUNT, nullptr))
        {
        }
	};

	int _renderCreate(void* userPtr, int width, int height) {
		Context& context = *((Context*)userPtr);
        context.context->resizeImage(context.image, width, height);

		return 1;
	}

	int _renderResize(void* userPtr, int width, int height) {
        Context& context = *((Context*)userPtr);
        context.context->resizeImage(context.image, width, height);
		return 1;
	}

	void _renderUpdate(void* userPtr, int* rect, const unsigned char* data) {
        Context& context = *((Context*)userPtr);

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
	}

	void _renderDraw(void* userPtr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts) {
		Context& context = *((Context*)userPtr);
		context.material.set("uImage", context.image);

        // TODO: Make renderer agnostic somehow
		glBindBuffer(GL_ARRAY_BUFFER, (GLuint)context.vbo.identifier());

		unsigned char* buf = (unsigned char*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        memcpy(buf, (const void*)verts, sizeof(float) * 2 * nverts);
        buf += sizeof(float) * 2 * FONS_VERTEX_COUNT;
        memcpy(buf, (const void*)tcoords, sizeof(float) * 2 * nverts);
        buf += sizeof(float) * 2 * FONS_VERTEX_COUNT;
        memcpy(buf, (const void*)colors, 4 * nverts);

        // TODO: Make renderer agnostic somehow
		glUnmapBuffer(GL_ARRAY_BUFFER);

        context.renderPass.emptyQueue();
        context.renderPass.addToDrawQueue(context.vao, context.material, {});
        context.context->drawPass(context.renderPass);
	}

	void _renderDelete(void* userPtr) {
        // TODO: not supported yet
	}
}

FONScontext* glfonsCreate(int width, int height, int flags, TTRendering::RenderingContext* renderContext) {
	Context* context = new Context(renderContext);

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
	((Context*)ctx->params.userPtr)->material.set("uResolution", (float)width, (float)height);
}

void glfonsDelete(FONScontext* ctx) {
	fonsDeleteInternal(ctx);
}

unsigned int glfonsRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
	return (r) | (g << 8) | (b << 16) | (a << 24);
}
#endif
