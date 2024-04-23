// #define ORBIT_CAMERA

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
    InstancedMesh,
    Font,
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

struct FONSpoint {
    float x, y, u, v;
    unsigned int cd;
};

class FontGlobals{
    FONScontext* fs;
    std::unordered_map<std::string, int> fontByName;
    TTRendering::NullableHandle<TTRendering::ShaderHandle> _shader;

public:
    void loadFont(const char* path, const char* name) {
        fontByName[name] = fonsAddFont(fs, "sans", path);
    }

    int font(const char* name) const {
        auto it = fontByName.find(name);
        TT::assert(it != fontByName.end());
        return it->second;
    }

    void initRenderingResources(TTRendering::RenderingContext& context) {
        // TODO: Investigate multiple texture pooling and unlimited font count of https://github.com/akrinke/Font-Stash
        // Create GL stash for 512x512 texture, our coordinate system has zero at top-left.
        fs = glfonsCreate(512, 512, FONS_ZERO_BOTTOMLEFT, &context);

        _shader = context.fetchShader({
            context.fetchShaderStage("fontstash.vert.glsl"),
            context.fetchShaderStage("fontstash.geom.glsl"),
            context.fetchShaderStage("fontstash.frag.glsl") });
    }

    const TTRendering::ShaderHandle& shader() const {
        return *_shader;
    }

    const TTRendering::ImageHandle& image() const { 
        return glFonsAtlas(fs); 
    }

    void layoutText(std::vector<FONSpoint>& layout, const std::string& text, int font, float size, float& x, float& y, unsigned int color = 0, bool noClear = false) const {
        fonsSetFont(fs, font);
        fonsSetSize(fs, size);

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
            layout.push_back({ q.x0,q.y0,q.s0,q.t0,color });
            layout.push_back({ q.x1,q.y1,q.s1,q.t1,color });
        }
        x = iter.x;
        y = iter.y;
        // This is where the used glyphs are actually submitted to the texture.
        // It can also try to emit triangles but since we don't submit any draw commands via fontstash that is avoided.
        fonsFlush(fs);
    }
};

// TODO: Move to tickContext? Could (additionally?) be a private static of Font.
FontGlobals* gFontGlobals = new FontGlobals;

class Font : public Component {
    using Component::Component;

    int _font = -1;
    float _size = 24.0f;
    unsigned int _color = glfonsRGBA(255, 255, 255, 255);
    std::string _text;
    std::vector<FONSpoint> _layout;
    Transform* transform = nullptr;
    TTRendering::RenderEntry _renderableHandle;
    TTRendering::PushConstants pushConstants;

    void _update() {
        if (_font == -1)
            return;
        float x = 0.0f, y = 0.0f;
        gFontGlobals->layoutText(_layout, _text, _font, _size, x, y, _color);
        // TODO: If we have a renderable handle we should re-mesh
        // TODO: That also means we need to support mesh deletion
    }

public:
    DECL_COMPONENT_TYPE(Font, false);

    const std::string& text() const { return _text; }

    void setText(const std::string& text) { 
        _text = text;
        _update();
    }

    void setColor(float r, float g, float b, float a) {
        _color = glfonsRGBA(
            (unsigned char)TT::clamp(r * 255.0f, 0.0f, 255.0f),
            (unsigned char)TT::clamp(g * 255.0f, 0.0f, 255.0f),
            (unsigned char)TT::clamp(b * 255.0f, 0.0f, 255.0f),
            (unsigned char)TT::clamp(a * 255.0f, 0.0f, 255.0f));
        for (FONSpoint& point : _layout)
            point.cd = _color;
    }

    void setFont(const char* name) {
        _font = gFontGlobals->font(name);
        _update();
    }

    void setSize(float size) {
        _size = size;
        _update();
    }

    const std::vector<FONSpoint>& layout() const { 
        return _layout; 
    }

    void initRenderingResources(TickContext& context) override {
        // Per text mesh we may want a unique material so we can colorize
        auto material = context.render->createMaterial(gFontGlobals->shader(), TTRendering::MaterialBlendMode::Alpha);
        material.set("uImage", gFontGlobals->image());

        auto vbo = context.render->createBuffer(_layout.size() * sizeof(FONSpoint), (unsigned char*)&_layout[0]);
        auto mesh = context.render->createMesh(_layout.size() * 2, vbo, {
            // alternating top left and bottom right points
            { TTRendering::MeshAttribute::Dimensions::D2, TTRendering::MeshAttribute::ElementType::F32, 0 },
            { TTRendering::MeshAttribute::Dimensions::D2, TTRendering::MeshAttribute::ElementType::F32, 1 },
            { TTRendering::MeshAttribute::Dimensions::D1, TTRendering::MeshAttribute::ElementType::U32, 2 },
            }, nullptr, TTRendering::PrimitiveType::Line);

        _renderableHandle = context.scene->renderPass.addToDrawQueue(mesh, material, &pushConstants);

        transform = entity.component<Transform>();
    }

    void tick(TickContext& context) override {
        if (transform)
            pushConstants.modelMatrix = transform->worldMatrix();
    }
};

IMPL_COMPONENT_TYPE(Font);

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

class InstancedMesh : public Component {
protected:
    using Component::Component;

    TTRendering::PushConstants pushConstants;
    Transform* transform = nullptr;

public:
    DECL_COMPONENT_TYPE(InstancedMesh, false);

    // Config
    TTRendering::NullableHandle<TTRendering::MeshHandle> mesh = nullptr;
    TTRendering::NullableHandle<TTRendering::MaterialHandle> material = nullptr;
    size_t instanceCount;

    TTRendering::NullableHandle<TTRendering::ImageHandle> image = nullptr;

    void initRenderingResources(TickContext& context) override {
        if (!image || !material || !mesh)
            return;
        
        material->set("uImage", *image);
        context.scene->renderPass.addToDrawQueue(*mesh, *material, &pushConstants, instanceCount);

        transform = entity.component<Transform>();
    }

    void tick(TickContext& context) override {
        if (transform)
            pushConstants.modelMatrix = transform->worldMatrix();
        if (material)
            material->set("uSeconds", (float)context.runtime);

    }
};

IMPL_COMPONENT_TYPE(InstancedMesh);

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
        for(SimpleScene& scene : scenes)
            for (Entity* entity : scene.entities)
                delete entity;
    }

    void tick(double runtime, double deltaTime) {
        tickContext.runtime = runtime;
        tickContext.deltaTime = deltaTime;

        // Switch scenes
        for (int i = 0; i < TT::min((int)scenes.size(), 9); ++i) {
            char idx = '1' + i;
            if (i == 10)
                idx = '0';
            if (tickContext.keyStates[idx] == EKeyState::Press)
                tickContext.scene = &scenes[i];
        }

        // Update active scene
        if(tickContext.scene)
            for (Entity* entity : tickContext.scene->entities)
                entity->tick(tickContext);

#ifndef ORBIT_CAMERA
        tickCamera(deltaTime);
#endif

        dropKeystates();
    }

private:
    TickContext tickContext;

    bool sizeKnown = false;
    TTRendering::OpenGLContext context;
    
    TTRendering::UniformBlockHandle forwardPassUniforms;

    TTRendering::RenderPass presentPass;

    void initSharedResources() {
        // Font support
        gFontGlobals->initRenderingResources(context);
        gFontGlobals->loadFont("C:\\Windows\\fonts\\arial.ttf", "arial");

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
            // auto shader = tickContext.resources.shaders["imageShader"];
            auto shader = context.fetchShader({ context.fetchShaderStage("rookworsttunnel.vert.glsl"), context.fetchShaderStage("rookworsttunnel.frag.glsl") });
            forwardPassUniforms = context.createUniformBuffer(shader, TTRendering::UniformBlockSemantics::Pass);
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
            tickContext.resources.images.insert("infinidel", *context.loadImage("Sprites/Infinidel.png", TTRendering::ImageInterpolation::Nearest, TTRendering::ImageTiling::Clamp));
        }
    }

    void finalizeScene(SimpleScene& scene) {
        // Finalize scene
        scene.renderPass.setPassUniforms(forwardPassUniforms);
        tickContext.scene = &scene;
        for (Entity* entity : scene.entities)
            entity->initRenderingResources(tickContext);
    }
    
    static void initParticleScene(const TickContext& tickContext, SimpleScene& scene) {
        scene.renderPass.clearColor = { 0.1f, 0.2f, 0.3f, 1.0f };
        scene.renderPass.setFramebuffer(tickContext.resources.framebuffers["fbo"]);

        {
            // Spawn an entity with a sprite to use
            Entity* e = new Entity;
            scene.entities.push_back(e);

            auto frikandel = tickContext.resources.images["frikandel"];
            unsigned int w, h;
            tickContext.render->imageSize(frikandel, w, h);

            e->addComponent<Transform>()->setScale({ (float)w, (float)h, 1.0f });
            e->component<Transform>()->setTranslate({ (float)w * -0.5f, (float)h * -0.5f, 0.0f });
            e->addComponent<Sprite>()->image = frikandel;
        }

        {
            // Spawn the particles
            Entity* e = new Entity;
            scene.entities.push_back(e);
            e->addComponent<Transform>();
            e->addComponent<ExampleParticle>();
        }

        {
            // Spawn example font
            Entity* e = new Entity;
            scene.entities.push_back(e);
            e->addComponent<Transform>();
            e->addComponent<ExampleFont>();
        }
    }

    static void initTunnelScene(const TickContext& ctx, SimpleScene& scene) {
        scene.renderPass.clearColor = { 0.1f, 0.2f, 0.3f, 1.0f };
        scene.renderPass.setFramebuffer(ctx.resources.framebuffers["fbo"]);

        {
            // Spawn an entity with a sprite to use
            Entity* e = new Entity;
            scene.entities.push_back(e);

            unsigned int w, h;
            auto rookworst = ctx.resources.images["rookworst"];
            ctx.render->imageSize(rookworst, w, h);

            e->addComponent<Transform>()->setScale({ (float)w, (float)h, 1.0f });
            e->component<Transform>()->setTranslate({ (float)w * -0.5f, (float)h * -0.5f, 0.0f });

            auto component = e->addComponent<InstancedMesh>();
            component->material = ctx.render->createMaterial(ctx.render->fetchShader({
                ctx.render->fetchShaderStage("rookworsttunnel.vert.glsl"),
                ctx.render->fetchShaderStage("rookworsttunnel.frag.glsl"),
                }), TTRendering::MaterialBlendMode::AlphaTest);
            component->mesh = ctx.resources.meshes["quadMesh"];
            component->instanceCount = 1000;
            component->image = rookworst;
        }
    }

    static void initScrollerScene(const TickContext& ctx, SimpleScene& scene) {
        scene.renderPass.clearColor = { 0.1f, 0.2f, 0.3f, 1.0f };
        scene.renderPass.setFramebuffer(ctx.resources.framebuffers["fbo"]);

        {
            // Spawn an entity with a sprite to use
            Entity* e = new Entity;
            scene.entities.push_back(e);

            unsigned int w, h;
            auto infinidel = ctx.resources.images["infinidel"];
            ctx.render->imageSize(infinidel, w, h);

            auto transform = e->addComponent<Transform>(); // ->setScale({ (float)w, (float)h, 1.0f });
            transform->setScale({ 10, 10, 10 });
            // e->component<Transform>()->setTranslate({ (float)w * -0.5f, (float)h * -0.5f, 0.0f });

            auto component = e->addComponent<InstancedMesh>();
            component->material = ctx.render->createMaterial(ctx.render->fetchShader({
                ctx.render->fetchShaderStage("infinidel.vert.glsl"),
                ctx.render->fetchShaderStage("infinidel.frag.glsl"),
                }), TTRendering::MaterialBlendMode::AlphaTest);
            component->mesh = ctx.resources.meshes["quadMesh"];
            component->instanceCount = 50;
            component->material->set("uInstanceCount", (int)component->instanceCount);
            component->image = infinidel;

            Entity* eText = new Entity;
            scene.entities.push_back(eText);
            auto tText = eText->addComponent<Transform>();
            tText->setParent(*transform);
            tText->setTranslate({ 0, 10, 0 });
            tText->setScale({ 0.01f, 0.01f, 0.01f });
            
            auto cText = eText->addComponent<Font>();
            // cText->setColor
            cText->setFont("arial");
            cText->setSize(48.0f);
            cText->setText("frikandel XXL: the big sequel");
        }
    }

    std::vector<SimpleScene> scenes;

    void initRenderingResources() {
        initSharedResources(); // <- must fill out forwardPassUniforms!
        
        initParticleScene(tickContext, scenes.emplace_back());
        initTunnelScene(tickContext, scenes.emplace_back());
        initScrollerScene(tickContext, scenes.emplace_back());

        for(SimpleScene& scene : scenes)
            finalizeScene(scene);
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
        // TT::Mat44 uP = TT::Mat44::perspectiveY(1.0, tickContext.resolution.x / tickContext.resolution.y, 0.1f, 10000.0f); // zoom * 0.1f, zoom * 10.0f);

#ifdef ORBIT_CAMERA
        TT::Mat44 uP = TT::Mat44::perspectiveY(1.0, tickContext.resolution.x / tickContext.resolution.y, zoom * 0.1f, zoom * 10.0f);
        TT::Mat44 uV = TT::Mat44::translate(origin) * TT::Mat44::rotate(tilt, orbit, 0.0f, TT::ERotateOrder::YXZ) * TT::Mat44::translate(0.0f, 0.0f, -zoom);
#else
        TT::Mat44 uP = TT::Mat44::perspectiveY(1.0, tickContext.resolution.x / tickContext.resolution.y, cameraNearClip, cameraFarClip);
        TT::Mat44 uV = TT::Mat44::translate(cameraPosition) * TT::Mat44::rotate(cameraRadians, TT::ERotateOrder::YXZ);
#endif
        
        TT::Mat44 uC = uV.inversed();
        // TT::Mat44 uC = TT::Mat44::translate(0.0f, 0.0f, -zoom) * TT::Mat44::rotate(-tilt, -orbit, 0.0f, TT::ERotateOrder::ZXY) * TT::Mat44::translate(origin);

        forwardPassUniforms.set("uVP", uV * uP);
        forwardPassUniforms.setVec3("uCameraPos", uC.col[3].m128_f32);

        context.beginFrame();
        if(tickContext.scene)
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

    float dragStartOrbit = 0.0f;
    float dragStartTilt = 0.0f;
    int dragStartX = 0;
    int dragStartY = 0;

#ifdef ORBIT_CAMERA
    TT::Vec3 origin = {};
    float orbit = 0.0f;
    float tilt = 0.0f;
    float zoom = 1000.0f;
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
#else
    TT::Vec3 cameraPosition = {};
    TT::Vec3 cameraRadians = {};
    float cameraNearClip = 0.1f;
    float cameraFarClip = 10000.0f;

    void tickCamera(double deltaTime) {
        TT::Mat44 uCr = TT::Mat44::rotate(-cameraRadians, TT::ERotateOrder::ZXY);

        TT::Vec3 delta = {};
        if (tickContext.keyStates['A'] != EKeyState::Up)
            delta -= TT::Vec3(uCr.col[0]);
        if (tickContext.keyStates['D'] != EKeyState::Up)
            delta += TT::Vec3(uCr.col[0]);
        if (tickContext.keyStates['E'] != EKeyState::Up)
            delta -= TT::Vec3(uCr.col[1]);
        if (tickContext.keyStates['Q'] != EKeyState::Up)
            delta += TT::Vec3(uCr.col[1]);
        if (tickContext.keyStates['W'] != EKeyState::Up)
            delta -= TT::Vec3(uCr.col[2]);
        if (tickContext.keyStates['S'] != EKeyState::Up)
            delta += TT::Vec3(uCr.col[2]);
        
        float speed = 100.0f;
        if (tickContext.keyStates[VK_SHIFT] != EKeyState::Up)
            speed = 1000.0f;
        if (tickContext.keyStates[VK_CONTROL] != EKeyState::Up)
            speed = 10.0f;

        cameraPosition += delta * (float)deltaTime * -speed;

        float dx = 0.0f;
        float dy = 0.0f;
        if (tickContext.keyStates[VK_LEFT] != EKeyState::Up)
            dx -= 1.0f;
        if (tickContext.keyStates[VK_RIGHT] != EKeyState::Up)
            dx += 1.0f;
        if (tickContext.keyStates[VK_UP] != EKeyState::Up)
            dy -= 1.0f;
        if (tickContext.keyStates[VK_DOWN] != EKeyState::Up)
            dy += 1.0f;
        cameraRadians.y += dx * (float)deltaTime;
        cameraRadians.x += dy * (float)deltaTime;

    }

    void onMouseEvent(const TT::MouseEvent& event) override {
        if (event.type == TT::Event::EType::MouseDown) {
            dragStartOrbit = cameraRadians.y;
            dragStartTilt = cameraRadians.x;
            dragStartX = event.x;
            dragStartY = event.y;
        }

        if (event.type == TT::Event::EType::MouseMove) {
            float dx = (event.x - dragStartX) * 0.001f;
            float dy = (event.y - dragStartY) * 0.001f;
            cameraRadians.y = dragStartOrbit + dx;
            cameraRadians.x = dragStartTilt + dy;
        }
    }
#endif
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
