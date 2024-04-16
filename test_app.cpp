#include <windont.h>
#include <tt_window.h>
#include "gl/tt_gl.h"

class App : public TT::Window {
public:
    App() : TT::Window() {
        TTRendering::createGLContext(*this);
        TTRendering::loadGLFunctions();
        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
        show();
    }

private:
    void onResizeEvent(const TT::ResizeEvent& event) override {
        glViewport(0, 0, event.width, event.height);
    }

    void onPaintEvent(const TT::PaintEvent& event) override {
        glClear(GL_COLOR_BUFFER_BIT);
        SwapBuffers(TTRendering::getGLContext(*this));
    }
};

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
    App window;
    MSG msg;
    bool quit = false;

    // Track frame timings
    ULONGLONG a = GetTickCount64();
    ULONGLONG b = 0;
    constexpr ULONGLONG FPS = 30;
    constexpr ULONGLONG t = 1000 / FPS;

#if 0
    ULONGLONG counter = 0;
    int frames = 0;
#endif

    do {
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                quit = true;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        // Force UI repaints as fast as possible
        window.repaint();
        // Detect when all windows are closed and exit this loop.
        if (!TT::Window::hasVisibleWindows())
            PostQuitMessage(0);

        // Sleep for remaining frame time; minimum amount we can sleep is 500 microseconds.
        // If we want to wait less time we can use __mwaitx and __tpause instead.
        // 21:41 https://www.youtube.com/watch?v=19ae5Mq2lJE
        b = a;
        a = GetTickCount64();
        b = a - b;
        
        if (b < t) Sleep((DWORD)(t - b));

#if 0
        // track nr of ms elapsed by adding delta time
        counter += b;
        // increment frame counter
        frames += 1;
        if(counter > 1000) {
            // display frames per second once per second and reset
            auto str = TT::formatStr("%d FPS\n", frames);
            OutputDebugStringA(str);
            // does not work for some weird reson
            SetWindowTextA(window.windowHandle(), str);
            delete[] str;
            frames = 0;
            counter %= 1000;
        }
#endif
    } while (!quit);
    return (int)msg.wParam;
}
