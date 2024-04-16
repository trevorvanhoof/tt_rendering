#pragma once

#include "windont.h"
#include <gl/gl.h>
#include "KHR/glext.h"
#include "tt_messages.h"
#include "tt_window.h"

namespace TTRendering {
    void loadGLFunctions();
    bool checkGLErrors();
    HDC createGLContext(const TT::Window& window);
    HDC getGLContext(const TT::Window& window);
}

#ifndef TT_GLEXT_IMPLEMENTATION
#ifdef TT_GL_DBG
#include "tt_gl_defs_dbg.inc"
#define TT_GL_DBG_ERR TT::checkGLErrors();
#else
#include "tt_gl_defs.inc"
#define TT_GL_DBG_ERR
#endif

#else
#ifdef TT_GL_DBG
#include "tt_gl_impl_dbg.inc"
#else
#include "tt_gl_impl.inc"
#endif
#endif
