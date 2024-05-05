#include "gl/tt_gl.h"
#include <stdio.h>

#define FONTSTASH_IMPLEMENTATION
#include "fontstash.h"
#undef FONTSTASH_IMPLEMENTATION

#ifdef assert
#undef assert
#endif assert

#define GLFONTSTASH_IMPLEMENTATION
// #include "glfontstash.h"
// #include "gl3corefontstash.h"
// #include "gl46corefontstash.h"
#include "gl/tt_gl_rendering_fontstash.h"
#undef GLFONTSTASH_IMPLEMENTATION
