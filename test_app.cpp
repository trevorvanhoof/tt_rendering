#include <windont.h>
#include <tt_window.h>
#include "gl/tt_glcontext.h"

enum class EKeyState {
    // Lease significant bit is up/down, other bit is 'changed since last frame'.
    Press = 0b11,
    Down = 0b01,
    Release = 0b10,
    Up = 0b00,
};

struct Context {
    TTRendering::RenderingContext* context;
    std::unordered_map<unsigned int, EKeyState> keyStates;
    double runtime;
    double deltaTime;
    TT::Vec2 resolution;
};

struct Sprite {
    TTRendering::ImageHandle* image;
    TTRendering::MaterialHandle* material;
};

struct Entity2D {
    TT::Vec2 pos{};
    TT::Vec2 size{}; // derived from image
    float angle = 0.0f;

    // TODO: Should this hold the init info? Should we provide it? We should probably do components instead and let the sprite component init itself during initREnderingResource, and it should own the push constants, and then have some system pull the transform matrix into that if valid.
    Sprite initInfo;

    virtual void init() {

    }

    TT::Mat44 localMatrix() {
        float sa = sin(-angle);
        float ca = cos(-angle);
        TT::Mat44 b {
            ca * size.x, sa * size.x, 0.0f, 0.0f,
            -sa * size.y, ca * size.y, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            pos.x, pos.y, 0.0f, 1.0f,
        };
        return b * TT::Mat44::translate(-0.5f, -0.5f, 0.0f);
    }

    TTRendering::PushConstants* pushConstants = nullptr;

    virtual void tick(const Context& tickContext) {
        angle += tickContext.deltaTime;
        if(pushConstants)
            pushConstants->modelMatrix = localMatrix();
    }
};

const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;
const int PIXEL_SIZE = 3;

class App : public TT::Window {
public:
    App() : TT::Window(), context(*this) {
        tickContext.context = &context;

        show();

        // Resize the window to HD
        // TODO: Fullscreen
        // TODO: Disable maximize toggle and window resize
        RECT r{ 0, 0, (int)SCREEN_WIDTH, (int)SCREEN_HEIGHT };
        AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, false);
        SetWindowPos(window, 0, 0, 0, r.right, r.bottom, SWP_NOMOVE);
    }

    ~App() {
        for (Entity2D* entity : scene)
            delete entity;
    }

    void tick(double runtime, double deltaTime) {
        tickContext.runtime = runtime;
        tickContext.deltaTime = deltaTime;

        for (Entity2D* entity : scene) 
            entity->tick(tickContext);

        dropKeystates();
    }

private:
    Context tickContext;
    std::vector<Entity2D*> scene;

    bool sizeKnown = false;
    TTRendering::OpenGLContext context;

    TTRendering::RenderPass mainPass;

    void initRenderingResources() {
        mainPass.clearColor = { 0.1f, 0.2f, 0.3f, 1.0f };
        
        // Get the sprite shader
        TTRendering::ShaderHandle imageShader = context.fetchShader({ context.fetchShaderStage("image.vert.glsl"),context.fetchShaderStage("image.frag.glsl") });

        // Get the UBO for that shader
        TTRendering::UniformBlockHandle forwardPassUniforms = context.createUniformBuffer(imageShader, TTRendering::UniformBlockSemantics::Pass);

        // Apply the view projection matrices
        TT::Mat44 viewProjectionMatrix = TT::Mat44::orthoSymmetric(tickContext.resolution.x / PIXEL_SIZE, tickContext.resolution.y / PIXEL_SIZE, -1.0f, 1.0f);
        forwardPassUniforms.set("uVP", viewProjectionMatrix);
        mainPass.setPassUniforms(forwardPassUniforms);

        // Get a quad
        float quadVerts[] = {0,0, 1,0, 1,1, 0,1};
        TTRendering::BufferHandle quadVbo = context.createBuffer(sizeof(quadVerts), (unsigned char*)quadVerts);
        TTRendering::MeshHandle quad = context.createMesh({ { TTRendering::MeshAttribute::Dimensions::D2, TTRendering::MeshAttribute::ElementType::F32, 0 } }, quadVbo, 4, nullptr, TTRendering::PrimitiveType::TriangleFan);

        // For each sprite, create a material
        auto maybe = context.loadImage("frikandel.png", TTRendering::ImageInterpolation::Nearest, TTRendering::ImageTiling::Clamp);
        TT::assertFatal(!maybe.isEmpty());
        TTRendering::MaterialHandle frikandel = context.createMaterial(imageShader, TTRendering::MaterialBlendMode::AlphaTest);
        frikandel.set("uImage", *maybe.value());
        
        // Spawn an entity with a sprite material to use
        scene.push_back(new Entity2D());
        scene.back()->pushConstants = mainPass.addToDrawQueue(quad, frikandel, {});
        scene.back()->size = { (float)maybe.value()->width(), (float)maybe.value()->height() };
    }

    void onResizeEvent(const TT::ResizeEvent& event) override {
        tickContext.resolution = TT::Vec2(event.width, event.height);
        context.windowResized(event.width, event.height);
        if (!sizeKnown)
            initRenderingResources();
    }

    void onPaintEvent(const TT::PaintEvent& event) override {
        context.beginFrame();
        context.drawPass(mainPass);
        context.endFrame();
    }

    void dropKeystates() {
        for (auto pair : tickContext.keyStates) {
            pair.second = (EKeyState)((int)pair.second & 0b1);
        }
    }

    void onKeyEvent(const TT::KeyEvent& event) override {
        if (event.type == TT::Event::EType::KeyDown) {
            // if (event.isRepeat) return; // TODO: isRepeat is/was bugged
            tickContext.keyStates[event.key] = EKeyState::Press;
        }

        if (event.type == TT::Event::EType::KeyUp) {
            tickContext.keyStates[event.key] = EKeyState::Release;
        }
    }
};

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
    App window;
    MSG msg;
    bool quit = false;

    // Track frame timings
    ULONGLONG startTime = GetTickCount64();
    ULONGLONG a = GetTickCount64();
    ULONGLONG b = 0;
    constexpr ULONGLONG FPS = 30;
    constexpr ULONGLONG t = 1000 / FPS;
    double runtime = 0.0;

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
        double prevRuntime = runtime;
        runtime = (double)(GetTickCount64() - startTime) / 1000.0;
        window.tick(runtime, runtime - prevRuntime);
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
