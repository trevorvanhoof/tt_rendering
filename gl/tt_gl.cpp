#define TT_GLEXT_IMPLEMENTATION
#include "tt_gl.h"
#include "../../tt_cpplib/tt_messages.h"
#include "../../tt_cpplib/tt_window.h"

#pragma comment(lib, "opengl32.lib")

namespace {
    // https://gist.github.com/nickrolfe/1127313ed1dbf80254b614a721b3ee9c
    typedef HGLRC(*wglCreateContextAttribsARBP)(HDC hDC, HGLRC hshareContext, const int* attribList);
    typedef bool (*wglChoosePixelFormatARBP)(HDC hdc, const int* piAttribIList, const float* pfAttribFList, unsigned int nMaxFormats, int* piFormats, unsigned int* nNumFormats);

    // See https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_create_context.txt for all values
    constexpr const int WGL_CONTEXT_MAJOR_VERSION_ARB = 0x2091;
    constexpr const int WGL_CONTEXT_MINOR_VERSION_ARB = 0x2092;
    constexpr const int WGL_CONTEXT_PROFILE_MASK_ARB = 0x9126;
    constexpr const int WGL_CONTEXT_CORE_PROFILE_BIT_ARB = 0x00000001;
    constexpr const int WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB = 0x00000002;

    // See https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_pixel_format.txt for all values
    constexpr const int WGL_DRAW_TO_WINDOW_ARB = 0x2001;
    constexpr const int WGL_ACCELERATION_ARB = 0x2003;
    constexpr const int WGL_SUPPORT_OPENGL_ARB = 0x2010;
    constexpr const int WGL_DOUBLE_BUFFER_ARB = 0x2011;
    constexpr const int WGL_PIXEL_TYPE_ARB = 0x2013;
    constexpr const int WGL_COLOR_BITS_ARB = 0x2014;
    constexpr const int WGL_DEPTH_BITS_ARB = 0x2022;
    constexpr const int WGL_STENCIL_BITS_ARB = 0x2023;
    constexpr const int WGL_FULL_ACCELERATION_ARB = 0x2027;
    constexpr const int WGL_TYPE_RGBA_ARB = 0x202B;

    wglCreateContextAttribsARBP wglCreateContextAttribsARB;
    wglChoosePixelFormatARBP wglChoosePixelFormatARB;

    void initContext2(HDC device) {
        // Now we can choose a pixel format the modern way, using wglChoosePixelFormatARB.
        int pixel_format_attribs[] = {
            WGL_DRAW_TO_WINDOW_ARB,     GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB,     GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB,      GL_TRUE,
            WGL_ACCELERATION_ARB,       WGL_FULL_ACCELERATION_ARB,
            WGL_PIXEL_TYPE_ARB,         WGL_TYPE_RGBA_ARB,
            WGL_COLOR_BITS_ARB,         32,
            WGL_DEPTH_BITS_ARB,         24,
            WGL_STENCIL_BITS_ARB,       8,
            0
        };

        int pixel_format;
        unsigned int num_formats;
        wglChoosePixelFormatARB(device, pixel_format_attribs, 0, 1, &pixel_format, &num_formats);
        TT::assertFatal(num_formats != 0);

        PIXELFORMATDESCRIPTOR pfd;
        DescribePixelFormat(device, pixel_format, sizeof(pfd), &pfd);
        TT::assertFatal(SetPixelFormat(device, pixel_format, &pfd));
    }
}

namespace TTRendering {
    bool checkGLErrors() {
        GLenum error = glGetError();
        switch (error) {
        case GL_NO_ERROR:
            return false;
        case GL_INVALID_ENUM:
            TT::warning("GL_INVALID_ENUM");
            return true;
        case GL_INVALID_VALUE:
            TT::warning("GL_INVALID_VALUE");
            return true;
        case GL_INVALID_OPERATION:
            TT::warning("GL_INVALID_OPERATION");
            return true;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            TT::warning("GL_INVALID_FRAMEBUFFER_OPERATION");
            return true;
        case GL_OUT_OF_MEMORY:
            TT::warning("GL_OUT_OF_MEMORY");
            return true;
        default:
            TT::warning("Unknown error code %d", error);
            return true;
        }
    }

    HDC createGLContext(const TT::Window& window) {
    #if 0
        HDC device = GetDC(window);
        const PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0, 0, 0, 0, 0 };
        SetPixelFormat(device, ChoosePixelFormat(device, &pfd), &pfd);
        // Old style context initialization
        wglMakeCurrent(device, wglCreateContext(device));
    #else
        // New style context initialization
        {
            // Create a fake window
            HWND dummyW = CreateWindowExA(0, "edit", "", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, 0, 0);
            // Create a fake device
            HDC dummyD = GetDC(dummyW);
            const PIXELFORMATDESCRIPTOR pfd = { 
                sizeof(PIXELFORMATDESCRIPTOR), // Size of this struct
                1, // Version
                PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, // Draw flags
                PFD_TYPE_RGBA, // iPixelType
                32, // ColorBIts
                0, 
                0, 
                0,
                0,
                0, 
                0,
                8, // AlphaBits
                0,
                0, 
                0, 
                0,
                0, 
                0,
                24, // DepthBits
                8, // StencilBits
                0, 
                0, 
                0, 
                0, 
                0, 
                0 
            };

            SetPixelFormat(dummyD, ChoosePixelFormat(dummyD, &pfd), &pfd);
            // Create a fake old style context and activate it
            HGLRC dummyC = wglCreateContext(dummyD);
            wglMakeCurrent(dummyD, dummyC);

            int j, n;
            glGetIntegerv(GL_MAJOR_VERSION, &j);
            glGetIntegerv(GL_MINOR_VERSION, &n);

            // Now we can load function pointers required for new style initialization
            wglCreateContextAttribsARB = (wglCreateContextAttribsARBP)wglGetProcAddress("wglCreateContextAttribsARB");
            wglChoosePixelFormatARB = (wglChoosePixelFormatARBP)wglGetProcAddress("wglChoosePixelFormatARB");
            // Delete fake resources
            wglMakeCurrent(dummyD, 0);
            wglDeleteContext(dummyC);
            ReleaseDC(dummyW, dummyD);
            DestroyWindow(dummyW);
        }

        HDC device = GetDC(window.windowHandle());

        // Set up a pixel format for this device
        initContext2(device);

        // Set up a GL context for this device
        const int attribList[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 6,
            // WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
            0, // End
        };

        HGLRC ctx = wglCreateContextAttribsARB(device, nullptr, attribList);
        wglMakeCurrent(device, ctx);

    #endif

        return device;
    }

    HDC getGLContext(const TT::Window& window) {
        return GetDC(window.windowHandle());
    }
}
