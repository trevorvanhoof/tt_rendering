#include <windont.h>
#include <tt_window.h>
#include <tt_files.h>
#include "gl/tt_glcontext.h"
#include "gl/tt_gl_rendering_fontstash.h"

#include <unordered_set>

enum class EKeyState {
    // Lease significant bit is up/down, other bit is 'changed since last frame'.
    Press = 0b11,
    Down = 0b01,
    Release = 0b10,
    Up = 0b00,
};

struct RenderResourcePool {
    TTRendering::HandleDict<std::string, TTRendering::BufferHandle> buffers;
    TTRendering::HandleDict<std::string, TTRendering::MeshHandle> meshes;
    TTRendering::HandleDict<std::string, TTRendering::ShaderHandle> shaders;
    TTRendering::HandleDict<std::string, TTRendering::ImageHandle> images;
    TTRendering::HandleDict<std::string, TTRendering::FramebufferHandle> framebuffers;
};

struct SimpleScene {
    // Active entities, order may be important.
    std::vector<class Entity*> entities;
    
    // Components will initialize their draws in this pass.
    TTRendering::RenderPass renderPass;
};

struct TickContext {
    // Rendering 
    // Context, may not be null once the context is used.
    TTRendering::RenderingContext* render;
    RenderResourcePool resources;

    // Keyboard
    std::unordered_map<unsigned int, EKeyState> keyStates;

    // Time
    double runtime;
    double deltaTime;

    // Screen
    TT::Vec2 resolution;

    // Active scene, may not be null once the context is used.
    SimpleScene* scene;
    /*
    // Active entities, order may be important
    std::vector<class Entity*> entities;

    // Shared rendering resources
    // TODO: we should probably have a good way to track these, maybe a bunch of dicts? maybe a specific component that the user can edit? probably dicts...
    TTRendering::NullableHandle<TTRendering::BufferHandle> quadVbo;
    TTRendering::NullableHandle<TTRendering::MeshHandle> quadMesh;
    TTRendering::NullableHandle<TTRendering::ShaderHandle> imageShader;
    TTRendering::NullableHandle<TTRendering::ShaderHandle> instancedImageShader;
    TTRendering::RenderPass mainPass;

    TTRendering::NullableHandle<TTRendering::FramebufferHandle> fbo;
    TTRendering::RenderPass fboTestPass;
    */
};

enum class ComponentType {
    Transform,
    Sprite,
    ExampleParticle,
    ExampleFont,
};

#define DECL_COMPONENT_TYPE(LABEL, IS_SINGLE_USE) static const ComponentType componentType = ComponentType::LABEL; virtual ComponentType type() const override; static const bool isSingleUse = IS_SINGLE_USE;
#define IMPL_COMPONENT_TYPE(LABEL) ComponentType LABEL::type() const { return ComponentType::LABEL; }

class Component {
protected:
    friend class Entity;
    Entity& entity;
    Component(Entity& entity) : entity(entity) {}

public:
    virtual ComponentType type() const = 0;
    virtual void initRenderingResources(TickContext& context) {}
    virtual void tick(TickContext& context) {}
};

class Entity {
    std::unordered_map<ComponentType, std::vector<Component*>> _components;

public:
    Entity() {}
    Entity(const Entity&) = delete;
    Entity(Entity&&) = delete;
    Entity& operator=(const Entity&) = delete;
    Entity& operator=(Entity&&) = delete;

    ~Entity() {
        for(const auto& pair : _components)
            for(Component* component : pair.second)
                delete component;
    }

    template<typename T> T* addComponent() {
        auto it = _components.find(T::componentType);
        if(T::isSingleUse && it != _components.end() && it->second.size() == 1) return nullptr;
        T* component = new T(*this);
        _components[T::componentType].push_back(component);
        return component;
    }

    template<typename T> T* component() const {
        auto it = _components.find(T::componentType);
        if(it == _components.end() || it->second.size() == 0)
            return nullptr;
        return (T*)it->second[0];
    }

    template<typename T> const std::vector<T*> components() const {
        auto it = _components.find(T::componentType);
        if(it == _components.end())
            return {};
        return it->second;
    }

    void initRenderingResources(TickContext& context) {
        for(auto& pair : _components)
            for(Component* component : pair.second)
                component->initRenderingResources(context);
    }

    void tick(TickContext& context) {
        for(auto& pair : _components)
            for(Component* component : pair.second)
                component->tick(context);
    }
};

class Transform : public Component {
protected:
    using Component::Component;

    Transform* _parent = nullptr;
    std::unordered_set<Transform*> _children;

    TT::Vec3 _translate = {0,0,0};
    TT::Vec3 _radians = {0,0,0};
    TT::Vec3 _scale = {1,1,1};
    TT::ERotateOrder _rotateOrder = TT::ERotateOrder::YXZ;

    TT::Mat44 _cachedLocalMatrix = TT::MAT44_IDENTITY;
    bool _cachedLocalMatrixDirty = false;

    TT::Mat44 _cachedWorldMatrix = TT::MAT44_IDENTITY;
    bool _cachedWorldMatrixDirty = false;

    void registerChild(Transform& child) {
        _children.insert(&child);
        dirtyPropagation(); 
    }

    void deregisterChild(Transform& child) { 
        _children.erase(&child);
        dirtyPropagation(); 
    }

    void dirtyPropagation() {
        // If the local matrix changed, then so did the world matrix.
        _cachedWorldMatrixDirty = true;

        // If the world matrix changed, then the child world matrices also changed.
        for(Transform* child : _children) {
            child->dirtyPropagation();
        }
    }

public:
    DECL_COMPONENT_TYPE(Transform, true);
    
    Transform* parent() const { return _parent; }
    const std::unordered_set<Transform*>& children() const { return _children; }

    const TT::Vec3& translate() const { return _translate;}
    const TT::Vec3& radians() const { return _radians;}
    const TT::Vec3& scale() const { return _scale;}
    TT::ERotateOrder rotateOrder() const { return _rotateOrder;}

    void setTranslate(const TT::Vec3& translate) { 
        if(_translate != translate) { 
            _cachedLocalMatrixDirty = true; 
            dirtyPropagation(); 
        }
        _translate = translate;
    }

    void setRadians(const TT::Vec3& radians) { 
        if(_radians != radians) { 
            _cachedLocalMatrixDirty = true;
            dirtyPropagation(); 
        }
        _radians = radians; 
    }

    void setScale(const TT::Vec3& scale) { 
        if(_scale != scale) { 
            _cachedLocalMatrixDirty = true; 
            dirtyPropagation(); 
        }
        _scale = scale; 
    }

    void setRotateOrder(TT::ERotateOrder rotateOrder) { 
        if(_rotateOrder != rotateOrder) { 
            _cachedLocalMatrixDirty = true; 
            dirtyPropagation(); 
        }
        _rotateOrder = rotateOrder; 
    }

    const TT::Mat44& localMatrix() {
        if(_cachedLocalMatrixDirty) {
            _cachedLocalMatrix = TT::Mat44::TRS(_translate, _radians, _scale, _rotateOrder);
            _cachedLocalMatrixDirty = false;
        }
        return _cachedLocalMatrix;
    }

    const TT::Mat44& worldMatrix() {
        if(_cachedWorldMatrixDirty) {
            if(_parent)
                _cachedWorldMatrix = _parent->worldMatrix() * localMatrix();
            else
                _cachedWorldMatrix = localMatrix();
            _cachedWorldMatrixDirty = false;
        }
        return _cachedWorldMatrix;
    }

    void setParent(Transform& parent) { 
        if(_parent)
            _parent->deregisterChild(*this); 
        _parent = &parent; 
        if(_parent)
            _parent->registerChild(*this); 
    }
};

IMPL_COMPONENT_TYPE(Transform);

class Sprite : public Component {
protected:
    using Component::Component;

    TTRendering::PushConstants pushConstants;
    Transform* transform = nullptr;

public:
    DECL_COMPONENT_TYPE(Sprite, true);

    TTRendering::NullableHandle<TTRendering::ImageHandle> image = nullptr;

    void initRenderingResources(TickContext& context) override {
        if(!image)
            return;

        TTRendering::MaterialHandle material = context.render->createMaterial(context.resources.shaders["imageShader"], TTRendering::MaterialBlendMode::AlphaTest);
        material.set("uImage", *image);
        context.scene->renderPass.addToDrawQueue(context.resources.meshes["quadMesh"], material, &pushConstants);
        
        transform = entity.component<Transform>();
    }

    void tick(TickContext& context) override {
        if(transform)
            pushConstants.modelMatrix = transform->worldMatrix();
    }
};

IMPL_COMPONENT_TYPE(Sprite);

class ExampleParticle : public Component {
protected:
    using Component::Component;

    const int instanceCount = 128;
    
    // TODO: Switch ssbo bindings on the fly so these can become static
    TTRendering::NullableHandle<TTRendering::MaterialHandle> initMtl;
    TTRendering::NullableHandle<TTRendering::MaterialHandle> tickMtl;

    TTRendering::PushConstants pushConstants;
    Transform* transform = nullptr;

public:
    DECL_COMPONENT_TYPE(ExampleParticle, true);

    void initRenderingResources(TickContext& context) override {
        // Reimplemented from Sprite.
        // if(!image) return;

        // Generate particle positions in a buffer
        TTRendering::BufferHandle ssbo = context.render->createBuffer(instanceCount * sizeof(float) * 6, nullptr, TTRendering::BufferMode::DynamicDraw);

        {
            initMtl = context.render->createMaterial(context.render->fetchShader({context.render->fetchShaderStage("particles.compute.glsl")}));
            initMtl->set(0, ssbo);
            context.render->dispatchCompute(*initMtl, instanceCount, 1, 1);
        }

        {
            tickMtl = context.render->createMaterial(context.render->fetchShader({context.render->fetchShaderStage("particles_tick.compute.glsl")}));
            tickMtl->set(0, ssbo);
        }

        // Build an instanced quad
        TTRendering::MeshHandle instancedQuad = context.render->createMesh(
            4, context.resources.buffers["quadVbo"], {
                // vec2[4] vertex positions
                { TTRendering::MeshAttribute::Dimensions::D2, TTRendering::MeshAttribute::ElementType::F32, 0 } 
            }, 
            nullptr, TTRendering::PrimitiveType::TriangleFan, 
            instanceCount, &ssbo, {
                // [vec3 position, vec3 velocity][instanceCount]
                { TTRendering::MeshAttribute::Dimensions::D3, TTRendering::MeshAttribute::ElementType::F32, 1 },
                { TTRendering::MeshAttribute::Dimensions::D3, TTRendering::MeshAttribute::ElementType::F32, 2 }
            } 
        );
        
        // Draw instanced particles
        // TODO: these should be global / shared
        TTRendering::MaterialHandle material = context.render->createMaterial(context.resources.shaders["instancedImageShader"], TTRendering::MaterialBlendMode::AlphaTest);

        std::vector<TTRendering::NullableHandle<TTRendering::ImageHandle>> images = {
            context.resources.images["tulip"],
            context.resources.images["fanta"],
            context.resources.images["frikandel"],
            context.resources.images["grolsch"],
            context.resources.images["jesus"],
            context.resources.images["kaasaugurkui"],
            context.resources.images["kaasblokjes"],
            context.resources.images["rookworst"],
        };

        int i = 0;
        for(const auto& image : images) {
            if(image) {
                const char* label = TT::formatStr("uImage[%d]", i++);
                material.set(label, *image);
                delete[] label;
            }
            material.set("uImageCount", i);
        }

        context.scene->renderPass.addToDrawQueue(instancedQuad, material, &pushConstants, instanceCount);

        transform = entity.component<Transform>();
    }

    void tick(TickContext& context) override {
        if(initMtl) {
            if(context.keyStates[VK_SPACE] == EKeyState::Press) {
                context.render->dispatchCompute(*initMtl, instanceCount, 1, 1);
            }
        }

        if(tickMtl) {
            tickMtl->set("uDeltaTime", (float)context.deltaTime);
            context.render->dispatchCompute(*tickMtl, instanceCount, 1, 1);
        }

        if(transform) {
            auto r = transform->radians();
            r.y += (float)context.deltaTime;
            transform->setRadians(r);

            pushConstants.modelMatrix = transform->worldMatrix();
        }
    }
};

IMPL_COMPONENT_TYPE(ExampleParticle);

class ExampleFont : public Component {
    using Component::Component;

    // TODO: This be global
    FONScontext* fs;
    // TODO: This be global
    int fontNormal;
    struct FONSpoint {
        float x, y, u, v;
        unsigned int cd;
    };
    static void layoutText(FONScontext* fs, std::vector<FONSpoint>& layout, const std::string& text, float& x, float& y, unsigned int color = 0, bool noClear = false) {
        if (noClear) {
            layout.reserve(layout.size() + text.size());
        } else {
            layout.clear();
            layout.reserve(text.size());
        }
        FONStextIter iter;
        fonsTextIterInit(fs, &iter, x, y, text.data(), text.data() + text.size());
        FONSquad q;
        while (fonsTextIterNext(fs, &iter, &q)) {
            layout.push_back({q.x0,q.y0,q.s0,q.t0,color});
            layout.push_back({q.x1,q.y1,q.s1,q.t1,color});
        }
        x = iter.x;
        y = iter.y;
        // This is where the used glyphs are actually submitted to the texture.
        // It can also try to emit triangles but since we don't submit any draw commands via fontstash that is avoided.
        fonsFlush(fs); 
    }

    std::string _prevText;
    TTRendering::RenderEntry _renderableHandle;
    TTRendering::PushConstants pushConstants;
    Transform* transform = nullptr;

public:
    DECL_COMPONENT_TYPE(ExampleFont, true);

    std::string text = "Hello World!";

    void initRenderingResources(TickContext& context) override {
        // TODO: Investigate multiple texture pooling and unlimited font count of https://github.com/akrinke/Font-Stash
        // Create GL stash for 512x512 texture, our coordinate system has zero at top-left.
        fs = glfonsCreate(512, 512, FONS_ZERO_BOTTOMLEFT, context.render);
        // Add font to stash.
        fontNormal = fonsAddFont(fs, "sans", "C:\\Windows\\fonts\\arial.ttf"); // "DroidSerif-Regular.ttf");
        auto shader = context.render->fetchShader({
            context.render->fetchShaderStage("fontstash.vert.glsl"), 
            context.render->fetchShaderStage("fontstash.geom.glsl"), 
            context.render->fetchShaderStage("fontstash.frag.glsl")});

        // Per text mesh we may want a unique material so we can colorize
        auto material = context.render->createMaterial(shader, TTRendering::MaterialBlendMode::Alpha);
        material.set("uImage", glFonsAtlas(fs));

        std::vector<FONSpoint> layout;
        /*fonsSetFont(fs, fontNormal);
        fonsSetSize(fs, 124.0f);
        layoutText(fs, layout, text, 0, 0, false);*/

        float dx = 10, dy = 100;
        unsigned int white = glfonsRGBA(255,255,255,128);
        unsigned int brown = glfonsRGBA(255,128,0,128);
        fonsSetFont(fs, fontNormal);
        fonsSetSize(fs, 124.0f);
        layoutText(fs, layout, "The big ", dx, dy, white, false);
        fonsSetSize(fs, 24.0f);
        layoutText(fs, layout, "frikandel XXL", dx, dy, brown, true);

        auto vbo = context.render->createBuffer(layout.size() * sizeof(FONSpoint), (unsigned char*)&layout[0]);
        auto mesh = context.render->createMesh(layout.size() * 2, vbo, {
            // alternating top left and bottom right points
            { TTRendering::MeshAttribute::Dimensions::D2, TTRendering::MeshAttribute::ElementType::F32, 0 },
            { TTRendering::MeshAttribute::Dimensions::D2, TTRendering::MeshAttribute::ElementType::F32, 1 },
            { TTRendering::MeshAttribute::Dimensions::D1, TTRendering::MeshAttribute::ElementType::U32, 2 },
            }, nullptr, TTRendering::PrimitiveType::Line);

        _renderableHandle = context.scene->renderPass.addToDrawQueue(mesh, material, &pushConstants);
        _prevText = text;

        transform = entity.component<Transform>();
    }

    void tick(TickContext& context) override {
        if(transform)
            pushConstants.modelMatrix = transform->worldMatrix();
    }
};

IMPL_COMPONENT_TYPE(ExampleFont);

const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;
const int PIXEL_SIZE = 1;

class App : public TT::Window {
public:
    App() : TT::Window(), context(*this) {
        tickContext.render = &context;

        // Resize the window to HD
        // TODO: Fullscreen
        // TODO: Disable maximize toggle and window resize
        RECT r{ 0, 0, (int)SCREEN_WIDTH, (int)SCREEN_HEIGHT };
        AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, false);
        SetWindowPos(window, 0, 0, 0, r.right, r.bottom, SWP_NOMOVE);

        show();
    }

    ~App() {
        for (Entity* entity : tickContext.scene->entities)
            delete entity;
    }

    void tick(double runtime, double deltaTime) {
        tickContext.runtime = runtime;
        tickContext.deltaTime = deltaTime;

        // Switch scenes
        tickContext.scene = &particleScene;
        tickContext.scene = &tunnelScene;

        // Update active scene
        for (Entity* entity : tickContext.scene->entities)
            entity->tick(tickContext);

        dropKeystates();
    }

private:
    TickContext tickContext;

    bool sizeKnown = false;
    TTRendering::OpenGLContext context;
    
    TTRendering::UniformBlockHandle forwardPassUniforms;

    TTRendering::RenderPass presentPass;

    void initSharedResources() {
        // Framebuffer for scenes to render into
        {
            tickContext.resources.images.insert("fbo_cd", context.createImage(width(), height(), TTRendering::ImageFormat::RGBA32F, TTRendering::ImageInterpolation::Nearest, TTRendering::ImageTiling::Clamp));
            auto fbo_d = context.createImage(width(), height(), TTRendering::ImageFormat::Depth32F, TTRendering::ImageInterpolation::Nearest, TTRendering::ImageTiling::Clamp);
            tickContext.resources.images.insert("fbo_d", fbo_d);
            tickContext.resources.framebuffers.insert("fbo", context.createFramebuffer({ tickContext.resources.images["fbo_cd"] }, &fbo_d));
        }

        // Sprite shaders
        {
            tickContext.resources.shaders.insert("imageShader", context.fetchShader({ context.fetchShaderStage("image.vert.glsl"),context.fetchShaderStage("image.frag.glsl") }));
            tickContext.resources.shaders.insert("instancedImageShader", context.fetchShader({context.fetchShaderStage("image_instanced.vert.glsl"),context.fetchShaderStage("image_instanced.frag.glsl")}));

            // Get the pass uniforms from the first shader we load because I don't support arbitrary UBO definitions yet.
            forwardPassUniforms = context.createUniformBuffer(tickContext.resources.shaders["imageShader"], TTRendering::UniformBlockSemantics::Pass);
        }

        // Quad
        {
            float quadVerts[] = { 0,0, 1,0, 1,1, 0,1 };
            tickContext.resources.buffers.insert("quadVbo", context.createBuffer(sizeof(quadVerts), (unsigned char*)quadVerts));
            tickContext.resources.meshes.insert("quadMesh", context.createMesh(4, tickContext.resources.buffers["quadVbo"], { {TTRendering::MeshAttribute::Dimensions::D2, TTRendering::MeshAttribute::ElementType::F32, 0} }, nullptr, TTRendering::PrimitiveType::TriangleFan));
        }

        // Set up the present pass
        {
            auto blitMaterial = context.createMaterial(context.fetchShader({ context.fetchShaderStage("noop.vert.glsl"), context.fetchShaderStage("blit.frag.glsl") }));
            blitMaterial.set("uImage", tickContext.resources.images["fbo_cd"]);
            presentPass.addToDrawQueue(tickContext.resources.meshes["quadMesh"], blitMaterial);
            presentPass.clearColor = { 0.0f, 0.0f, 1.0f, 1.0f };
        }

        // Load our funky sprites
        {
            tickContext.resources.images.insert("tulip", *context.loadImage("Sprites/Atari Tulip_01_large 600x600.png", TTRendering::ImageInterpolation::Nearest, TTRendering::ImageTiling::Clamp));
            tickContext.resources.images.insert("fanta", *context.loadImage("Sprites/Fanta blikje.png", TTRendering::ImageInterpolation::Nearest, TTRendering::ImageTiling::Clamp));
            tickContext.resources.images.insert("frikandel", *context.loadImage("Sprites/Frikandel Speciaal.png", TTRendering::ImageInterpolation::Nearest, TTRendering::ImageTiling::Clamp));
            tickContext.resources.images.insert("grolsch", *context.loadImage("Sprites/grolsch beugel.png", TTRendering::ImageInterpolation::Nearest, TTRendering::ImageTiling::Clamp));
            tickContext.resources.images.insert("jesus", *context.loadImage("Sprites/Jesus approves (1).png", TTRendering::ImageInterpolation::Nearest, TTRendering::ImageTiling::Clamp));
            tickContext.resources.images.insert("kaasaugurkui", *context.loadImage("Sprites/kaasaugurkui 400x400.png", TTRendering::ImageInterpolation::Nearest, TTRendering::ImageTiling::Clamp));
            tickContext.resources.images.insert("kaasblokjes", *context.loadImage("Sprites/kaasblokjes.png", TTRendering::ImageInterpolation::Nearest, TTRendering::ImageTiling::Clamp));
            tickContext.resources.images.insert("rookworst", *context.loadImage("Sprites/Rookworst 600x400.png", TTRendering::ImageInterpolation::Nearest, TTRendering::ImageTiling::Clamp));
        }
    }

    void finalizeScene(SimpleScene& scene) {
        // Finalize scene
        scene.renderPass.setPassUniforms(forwardPassUniforms);
        tickContext.scene = &scene;
        for (Entity* entity : scene.entities)
            entity->initRenderingResources(tickContext);
    }
    
    SimpleScene particleScene;
    void initParticleScene() {
        particleScene.renderPass.clearColor = { 0.1f, 0.2f, 0.3f, 1.0f };
        particleScene.renderPass.setFramebuffer(tickContext.resources.framebuffers["fbo"]);

        {
            // Spawn an entity with a sprite to use
            Entity* e = new Entity;
            particleScene.entities.push_back(e);

            auto frikandel = tickContext.resources.images["frikandel"];
            unsigned int w, h;
            context.imageSize(frikandel, w, h);

            e->addComponent<Transform>()->setScale({ (float)w, (float)h, 1.0f });
            e->component<Transform>()->setTranslate({ (float)w * -0.5f, (float)h * -0.5f, 0.0f });
            e->addComponent<Sprite>()->image = frikandel;
        }

        {
            // Spawn the particles
            Entity* e = new Entity;
            particleScene.entities.push_back(e);
            e->addComponent<Transform>();
            e->addComponent<ExampleParticle>();
        }

        {
            // Spawn example font
            Entity* e = new Entity;
            particleScene.entities.push_back(e);
            e->addComponent<Transform>();
            e->addComponent<ExampleFont>();
        }

        finalizeScene(particleScene);
    }

    SimpleScene tunnelScene;
    void initTunnelScene() {
        tunnelScene.renderPass.clearColor = { 0.1f, 0.2f, 0.3f, 1.0f };
        tunnelScene.renderPass.setFramebuffer(tickContext.resources.framebuffers["fbo"]);

        {
            // Spawn an entity with a sprite to use
            Entity* e = new Entity;
            tunnelScene.entities.push_back(e);

            unsigned int w, h;
            auto rookworst = tickContext.resources.images["rookworst"];
            context.imageSize(rookworst, w, h);

            e->addComponent<Transform>()->setScale({ (float)w, (float)h, 1.0f });
            e->component<Transform>()->setTranslate({ (float)w * -0.5f, (float)h * -0.5f, 0.0f });
            e->addComponent<Sprite>()->image = rookworst;
        }

        finalizeScene(tunnelScene);
    }

    void initRenderingResources() {
        initSharedResources(); // <- must fill out forwardPassUniforms!
        initParticleScene(); // <- must each call finalizeScene on the respective scene at the end
        initTunnelScene();
    }

    void onResizeEvent(const TT::ResizeEvent& event) override {
        tickContext.resolution = TT::Vec2((float)event.width, (float)event.height);
        context.windowResized(event.width, event.height);

        if (!sizeKnown) { 
            sizeKnown = true;
            initRenderingResources();
        } else {
            context.resizeFramebuffer(tickContext.resources.framebuffers["fbo"], event.width, event.height);
        }
    }

    void onPaintEvent(const TT::PaintEvent& event) override {
        // Apply the view projection matrices
        // TT::Mat44 viewProjectionMatrix = TT::Mat44::orthoSymmetric(tickContext.resolution.x / PIXEL_SIZE, tickContext.resolution.y / PIXEL_SIZE, -1000.0f, 1000.0f);
        TT::Mat44 uP = TT::Mat44::perspectiveY(1.0, tickContext.resolution.x / tickContext.resolution.y, zoom * 0.1f, zoom * 10.0f);
        TT::Mat44 uV = TT::Mat44::rotate(tilt, orbit, 0.0f, TT::ERotateOrder::YXZ) * TT::Mat44::translate(0.0f, 0.0f, -zoom);
        uV = TT::Mat44::translate(origin) * uV;
        forwardPassUniforms.set("uVP", uV * uP);

        context.beginFrame();
        context.drawPass(tickContext.scene->renderPass);
        context.drawPass(presentPass);
        context.endFrame();
    }

    void dropKeystates() {
        for (auto& pair : tickContext.keyStates) {
            pair.second = (EKeyState)((int)pair.second & 0b1);
        }
    }

    void onKeyEvent(const TT::KeyEvent& event) override {
        if (event.type == TT::Event::EType::KeyDown) {
            if (event.isRepeat) return;
            tickContext.keyStates[event.key] = EKeyState::Press;
        }

        if (event.type == TT::Event::EType::KeyUp) {
            tickContext.keyStates[event.key] = EKeyState::Release;
        }
    }

    TT::Vec3 origin = {};
    float orbit = 0.0f;
    float tilt = 0.0f;
    float zoom = 1000.0f;
    float dragStartOrbit = 0.0f;
    float dragStartTilt = 0.0f;
    int dragStartX = 0;
    int dragStartY = 0;
    bool dragPanAction = false;
    TT::Vec3 dragStartOrigin = {};
    TT::Mat44 dragPanSpace = {};

    void onMouseEvent(const TT::MouseEvent& event) override {
        if (event.type == TT::Event::EType::MouseDown) {
            if(event.button == 2) {
                dragPanAction = true;
                dragStartOrigin = origin;
                dragPanSpace = TT::Mat44::rotate(-tilt, -orbit, 0.0f, TT::ERotateOrder::ZXY);
            } else {
                dragPanAction = false;
                dragStartOrbit = orbit;
                dragStartTilt = tilt;
            }
            dragStartX = event.x;
            dragStartY = event.y;
        }

        if (event.type == TT::Event::EType::MouseMove) {
            float dx = (event.x - dragStartX) * 0.001f;
            float dy = (event.y - dragStartY) * 0.001f;
            if(dragPanAction) {
                origin = dragStartOrigin + TT::Vec3(dragPanSpace.col[0]) * (dx * zoom * 0.5f) + TT::Vec3(dragPanSpace.col[1]) * -(dy * zoom * 0.5f);
            } else {
                orbit = dragStartOrbit + dx;
                tilt = dragStartTilt + dy;
            }
        }
    }

    void onWheelEvent(const TT::WheelEvent& event) override {
        zoom *= powf(1.0001f, -event.delta * 4.0f);
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
    } while (!quit);
    return (int)msg.wParam;
}
