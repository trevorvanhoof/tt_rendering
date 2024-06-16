#pragma once

#include "../tt_cpplib/tt_math.h"
#include "../tt_cpplib/tt_messages.h"

#include <vector>
#include <unordered_map>
#include <map>
#include <string>
#include <algorithm>

#ifndef BEFRIEND_CONTEXTS
#define BEFRIEND_CONTEXTS friend class RenderingContext; friend class OpenGLContext; friend class VkContext;
#endif

namespace TTRendering {
	// TODO: Use tt_containers instead of this workaround?
	// The reason these exist is because internally std::map generates std::pair
	// which in turn fails to copy or default construct or whatever. std::vector works fine.
	template<typename T>
	class HandlePool {
		std::unordered_map<size_t, size_t> identifierToIndex;
		std::vector<T> handles;

	public:
		void insert(const T& handle) {
			size_t identifier = handle.identifier();
			auto it = identifierToIndex.find(identifier);
			if (it != identifierToIndex.end())
				return;
			identifierToIndex[identifier] = handles.size();
			handles.push_back(handle);
		}

        void remove(const T& handle) {
            // Note: we do not remove from handles[] here because
            // that would invalidate all indices in identifierToIndex.
            identifierToIndex.erase(handle.identifier());
        }

		const T* find(size_t identifier) const {
			auto it = identifierToIndex.find(identifier);
			if (it != identifierToIndex.end())
				return &handles[it->second];
			return nullptr;
		}

        auto begin() const { return handles.begin(); }
        auto end() const { return handles.end(); }

        auto begin() { return handles.begin(); }
        auto end() { return handles.end(); }
	};

	template<typename K, typename T>
	class HandleDict {
		std::unordered_map<K, size_t> keyToIndex;
		std::vector<T> handles;

	public:
		void insert(const K& key, const T& handle) {
			auto it = keyToIndex.find(key);
			if (it != keyToIndex.end())
				return;
			keyToIndex[key] = handles.size();
			handles.push_back(handle);
		}
        
        void remove(const K& key) {
            // Note: we do not remove from handles[] here because
            // that would invalidate all indices in keyToIndex.
            keyToIndex.erase(key);
        }

        const T* find(const K& key) const {
            auto it = keyToIndex.find(key);
            if (it == keyToIndex.end())
                return nullptr;
            return &handles[it->second];
        }

        T* find(const K& key) {
            auto it = keyToIndex.find(key);
            if (it == keyToIndex.end())
                return nullptr;
            return &handles[it->second];
        }

		auto begin() const { return keyToIndex.begin(); }
		auto end() const { return keyToIndex.end(); }
		const T& handle(size_t index) const { return handles[index]; }
		T& handle(size_t index) { return handles[index]; }

		const T& operator[](const K& key) const {
			return *find(key);
		}
	};

	template<typename T>
	class NullableHandle {
		bool _empty = true;
		unsigned char* _handle;

	public:
		NullableHandle() {
			_handle = new unsigned char[sizeof(T)];
		}

        NullableHandle(const T& handle) {
            _handle = new unsigned char[sizeof(T)];
            set(handle);
        }

        NullableHandle(const T* handle) {
            _handle = new unsigned char[sizeof(T)];
            if(handle != nullptr) set(*handle);
        }

		~NullableHandle() {
			delete _handle;
		}

		NullableHandle(const NullableHandle& rhs) : NullableHandle() {
			if (!rhs.isEmpty())
				set(*rhs.value());
		}

		NullableHandle& operator=(const NullableHandle& rhs) {
			if (!rhs.isEmpty())
				set(*rhs.value());
			else
				clear();
			return *this;
		}

		NullableHandle(NullableHandle&& rhs) = delete;
		NullableHandle& operator=(NullableHandle&& rhs) = delete;

		bool isEmpty() const {
			return _empty;
		}

		void clear() {
			if (!_empty)
				((T*)_handle)->~T();
			_empty = true;
		}

		void set(const T& handle) {
			new(_handle) T(handle);
			_empty = false;
		}

		const T* value() const {
			return _empty ? nullptr : (const T*)_handle;
		}

		T* value() {
			return _empty ? nullptr : (T*)_handle;
		}

        const T* operator->() const {
            TT::assertFatal(!_empty);
            return (const T*)_handle;
        }

        T* operator->() {
            TT::assertFatal(!_empty);
            return (T*)_handle;
        }

        const T& operator*() const {
            TT::assertFatal(!_empty);
            return *(const T*)_handle;
        }

        T& operator*() {
            TT::assertFatal(!_empty);
            return *(T*)_handle;
        }

        operator T*() {
            return value();
        }

        void operator=(const T& rhs) {
            set(rhs);
        }

        operator bool() const {
            return !_empty;
        }
	};
}

namespace TTRendering {
	struct MeshAttribute {
		enum class Dimensions : unsigned char {
			D1, D2, D3, D4,
		};

		enum class ElementType : unsigned char {
			I8, U8, I16, U16, I32, U32, F16, F32, F64,
		};

		Dimensions dimensions = Dimensions::D3;
		ElementType elementType = ElementType::F32;
		unsigned char location = 0;

		unsigned char sizeInBytes() const {
			using namespace TTRendering;
			int factor = (int)dimensions + 1;
			switch (elementType) {
			case MeshAttribute::ElementType::I8:
			case MeshAttribute::ElementType::U8:
				return factor;
			case MeshAttribute::ElementType::I16:
			case MeshAttribute::ElementType::U16:
			case MeshAttribute::ElementType::F16:
				return factor * 2;
			case MeshAttribute::ElementType::I32:
			case MeshAttribute::ElementType::U32:
			case MeshAttribute::ElementType::F32:
				return factor * 4;
			case MeshAttribute::ElementType::F64:
				return factor * 8;
			}
			TT::assert(false);
			return 0;
		}
	};

    // Note: anything named handle should be copyable!
    // It should not own anything.
    // For this reason e.g. a uniform block will have a pointer
    // into memory owned by the context.
    // Removing a uniform block would invalidate all copies
    // of that uniform block handle (though using it after 
    // the fact is undefined behaviour).
	class HandleBase {
		BEFRIEND_CONTEXTS;

		size_t _identifier;

	protected:
		HandleBase(size_t identifier);

	public:
		size_t identifier() const;
	};

	class BufferHandle final : public HandleBase {
		BEFRIEND_CONTEXTS;

		size_t _size;

	protected:
		BufferHandle(size_t identifier, size_t size);

    public:
		size_t size() const;

        static const BufferHandle Null;
	};

	enum class PrimitiveType {
		Point, Line, Triangle, TriangleStrip, TriangleFan
	};

	enum class IndexType {
		None, U8, U16, U32
	};

	class MeshHandle : public HandleBase {
		BEFRIEND_CONTEXTS;

		friend class RenderPass;

		size_t _meshLayoutHash;
		BufferHandle _vertexBuffer;
		size_t _numElements; // num indices if ibo != nullptr, else num vertices
		PrimitiveType _primitiveType;
		IndexType _indexType;
		NullableHandle<BufferHandle> _indexBuffer;
        size_t _numInstances;
		NullableHandle<BufferHandle> _instanceBuffer;

		MeshHandle(
            size_t identifier,
			size_t meshLayoutHash,
			BufferHandle vertexBuffer,
			size_t numElements,
			PrimitiveType primitiveType,
			IndexType indexType,
			BufferHandle* indexBuffer = nullptr,
            size_t numIstances = 0,
            BufferHandle* instanceBuffer = nullptr);

	public:
		const BufferHandle vertexBuffer() const;
		size_t numElements() const;
		const BufferHandle* indexBuffer() const;
		PrimitiveType primitiveType() const;
		IndexType indexType() const;

        static const MeshHandle Null;
	};

	enum class ImageFormat {
		Depth32F,
		RGBA32F,
		R8, RG8, RGB8, RGBA8,
	};

	enum class ImageInterpolation {
		Linear,
		Nearest,
	};

	enum class ImageTiling {
		Repeat,
		Clamp,
	};

	class ImageHandle final : public HandleBase {
		BEFRIEND_CONTEXTS;

		ImageFormat _format;
		ImageInterpolation _interpolation;
		ImageTiling _tiling;

		ImageHandle(size_t identifier, ImageFormat format, ImageInterpolation interpolation, ImageTiling tiling);

	public:
		ImageFormat format() const;
		ImageInterpolation interpolation() const;
		ImageTiling tiling() const;
		// TODO: generate mip maps?

        static const ImageHandle Null;
        bool operator==(const ImageHandle& rhs) const { return identifier() == rhs.identifier(); }
        bool operator!=(const ImageHandle& rhs) const { return !operator==(rhs); }
	};

	class FramebufferHandle final : public HandleBase {
		BEFRIEND_CONTEXTS;

		std::vector<ImageHandle> _colorAttachments;
		NullableHandle<ImageHandle> _depthStencilAttachment;

		FramebufferHandle(size_t identifier, const std::vector<ImageHandle>& colorAttachments, const ImageHandle* depthStencilAttachment);

    public:
        const std::vector<ImageHandle>& colorAttachments() const { return _colorAttachments; }
        const ImageHandle* depthStencilAttachment() const { return _depthStencilAttachment.value(); }

        static const FramebufferHandle Null;
        bool operator==(const FramebufferHandle& rhs) const { return identifier() == rhs.identifier(); }
        bool operator!=(const FramebufferHandle& rhs) const { return !operator==(rhs); }
	};

	class ShaderStageHandle final : public HandleBase {
	public:
		enum ShaderStage {
			Vert,
			Frag,
			Geom,
			Compute,
		};

	private:
		BEFRIEND_CONTEXTS;

		ShaderStage _stage;

        // TODO: Do not want public.
    public:
		ShaderStageHandle(size_t identifier, ShaderStage stage);

	public:
		ShaderStage stage() const;
	};

	class ShaderHandle final : public HandleBase { 
        using HandleBase::HandleBase;

    public: 
        static const ShaderHandle Null;
        bool operator==(const ShaderHandle& rhs) const { return identifier() == rhs.identifier(); }
        bool operator!=(const ShaderHandle& rhs) const { return !operator==(rhs); }
    };

	enum class UniformType {
		Unknown,
		Float, Vec2, Vec3, Vec4,
		Int, IVec2, IVec3, IVec4,
		UInt, UVec2, UVec3, UVec4,
		Bool, BVec2, BVec3, BVec4,
		Mat2, Mat3, Mat4, Image,
	};

	struct UniformInfo {
		struct Field {
			std::string name;
			UniformType type;
			size_t offset;
			unsigned int arraySize; // non-arrays are just 1

			bool operator==(const Field& rhs);
		};

		std::unordered_map<size_t, size_t> nameHashToFieldIndex;
		std::vector<Field> fields;
		size_t bufferSize;

		bool operator==(const UniformInfo& rhs);
		const Field* find(const char* key) const;
	};

    namespace {
        struct UniformResources {
            unsigned char* uniformBuffer;
            HandleDict<std::string, ImageHandle> images;
            HandleDict<size_t, BufferHandle> ssbos;
        };
    }

	class UniformBlockHandle {
		BEFRIEND_CONTEXTS;

		friend class RenderPass;

        const UniformInfo* _uniformInfo;

	protected:
        // The context owns the resources, so handles can safely be copied around.
        UniformResources* _resources = nullptr;
        virtual bool isMaterialBlockHandle() const { return false; }

		bool _setUniform(const char* key, void* src, UniformType srcType, unsigned int count = 1);
		UniformBlockHandle(const UniformInfo& uniformInfo, UniformResources*& resources);
		UniformBlockHandle(UniformResources*& resources);

	public:
        const HandleDict<std::string, ImageHandle>& images() const { return _resources->images; }

		size_t size() const;
		unsigned char* cpuBuffer() const;

		bool hasUniformBlock() const;

		bool set(const char* key, float x);
		bool set(const char* key, float x, float y);
		bool set(const char* key, float x, float y, float z);
		bool set(const char* key, float x, float y, float z, float w);

		bool set(const char* key, TT::Mat22 m);
		bool set(const char* key, TT::Mat33 m);
		bool set(const char* key, TT::Mat44 m);
		bool set(const char* key, TT::Vec2 vec);
		bool set(const char* key, TT::Vec3 vec);
		bool set(const char* key, TT::Vec4 vec);

		bool set(const char* key, int x);
		bool set(const char* key, int x, int y);
		bool set(const char* key, int x, int y, int z);
		bool set(const char* key, int x, int y, int z, int w);

		bool set(const char* key, unsigned int x);
		bool set(const char* key, unsigned int x, unsigned int y);
		bool set(const char* key, unsigned int x, unsigned int y, unsigned int z);
		bool set(const char* key, unsigned int x, unsigned int y, unsigned int z, unsigned int w);

		bool set(const char* key, bool x);
		bool set(const char* key, bool x, bool y);
		bool set(const char* key, bool x, bool y, bool z);
		bool set(const char* key, bool x, bool y, bool z, bool w);

		bool setFloat(const char* key, float* value, unsigned int count = 1);
		bool setVec2(const char* key, float* value, unsigned int count = 1);
		bool setVec3(const char* key, float* value, unsigned int count = 1);
		bool setVec4(const char* key, float* value, unsigned int count = 1);
		bool setMat2(const char* key, float* value, unsigned int count = 1);
		bool setMat3(const char* key, float* value, unsigned int count = 1);
		bool setMat4(const char* key, float* value, unsigned int count = 1);

		bool setInt(const char* key, int* value, unsigned int count = 1);
		bool setIVec2(const char* key, int* value, unsigned int count = 1);
		bool setIVec3(const char* key, int* value, unsigned int count = 1);
		bool setIVec4(const char* key, int* value, unsigned int count = 1);

		bool setUInt(const char* key, unsigned int* value, unsigned int count = 1);
		bool setUVec2(const char* key, unsigned int* value, unsigned int count = 1);
		bool setUVec3(const char* key, unsigned int* value, unsigned int count = 1);
		bool setUVec4(const char* key, unsigned int* value, unsigned int count = 1);

		bool setBool(const char* key, int* value, unsigned int count = 1);
		bool setBVec2(const char* key, int* value, unsigned int count = 1);
		bool setBVec3(const char* key, int* value, unsigned int count = 1);
		bool setBVec4(const char* key, int* value, unsigned int count = 1);

		bool set(const char* key, const ImageHandle& image);
		bool set(size_t binding, const BufferHandle& buffer);

        bool operator==(const UniformBlockHandle& rhs) const { return _resources == rhs._resources && isMaterialBlockHandle() == rhs.isMaterialBlockHandle(); }
        bool operator!=(const UniformBlockHandle& rhs) const { return !operator==(rhs); }
	};

	enum class MaterialBlendMode {
		Opaque,
		AlphaTest,
		Alpha,
		PremultipliedAlpha,
		Additive,
	};

	class MaterialHandle : public UniformBlockHandle {
		BEFRIEND_CONTEXTS;

		friend class RenderPass;
        friend struct std::hash<TTRendering::MaterialHandle>;

		ShaderHandle _shader;
		MaterialBlendMode _blendMode;

		MaterialHandle(const ShaderHandle& shader, const UniformInfo& uniformInfo, UniformResources*& resources, MaterialBlendMode blendMode = MaterialBlendMode::Opaque);
		MaterialHandle(const ShaderHandle& shader, UniformResources*& resources, MaterialBlendMode blendMode = MaterialBlendMode::Opaque);

        virtual bool isMaterialBlockHandle() const { return true; }

	public:
		const ShaderHandle& shader() const;
		MaterialBlendMode blendMode() const;

        static const MaterialHandle Null;
        bool operator==(const MaterialHandle& rhs) const { return _shader.identifier() == rhs._shader.identifier() && UniformBlockHandle::operator==(rhs); }
        bool operator!=(const MaterialHandle& rhs) const { return !operator==(rhs); }
	};

	struct PushConstants {
		// In OpenGL this gets uploaded to uModelMatrix and uExtraData by name.
		TT::Mat44 modelMatrix = TT::MAT44_IDENTITY;
		TT::Mat44 extraData = TT::MAT44_IDENTITY;
	};

    struct RenderEntry {
        size_t shaderQueueIndex = -1;
        size_t materialQueueIndex = -1;
        size_t meshLayoutQueueIndex = -1;
        size_t meshIndex = -1;

		bool isNull() {
			// We dive into a tree to build this so we only need to check the last one.
			return meshIndex == -1;
		}
    };

    namespace {
        struct DrawInfo {
            size_t meshIdentifier = 0;
            size_t instanceCount = 0;
            const PushConstants* pushConstants = nullptr;
        };

        struct MeshQueue {
            size_t next = 0;
            std::map<size_t, DrawInfo> orderedQueue = {};
            auto begin() const { return orderedQueue.begin();}
            auto end() const { return orderedQueue.end();}
        };

        struct MaterialQueue {
            std::unordered_map<size_t, size_t> materialIdentifierToQueueIndex;

            std::vector<MaterialHandle> keys;
            std::vector<MeshQueue> queues;

            MeshQueue& fetch(const MaterialHandle& key, size_t& index);
        };

        struct ShaderQueue {

            std::unordered_map<size_t, size_t> shaderIdentifierToQueueIndex;
            std::vector<size_t> keys;
            std::vector<MaterialQueue> queues;

            MaterialQueue& fetch(size_t key, size_t& index);
        };

        struct DrawQueue {
            std::unordered_map<size_t, size_t> meshLayoutHashToQueueIndex;
            std::vector<size_t> keys;
            std::vector<ShaderQueue> queues;

            ShaderQueue& fetch(size_t key, size_t& index);
        };
    }

	class RenderPass {
		BEFRIEND_CONTEXTS;

		bool modified = true;

		DrawQueue _drawQueue;

		NullableHandle<UniformBlockHandle> passUniforms; // empty means we have no global uniforms to forward to the pipeline
		NullableHandle<FramebufferHandle> _framebuffer; // empty means we draw to screen

	public:
        const DrawQueue& drawQueue() const { return _drawQueue; }
        const FramebufferHandle* framebuffer() const { return _framebuffer.value(); }

		void setPassUniforms(UniformBlockHandle handle);
		void clearPassUniforms();

		void setFramebuffer(FramebufferHandle handle);
		void clearFramebuffer();

		TT::Vec4 clearColor;
		float clearDepthValue = 1.0f;

        RenderEntry addToDrawQueue(const MeshHandle& mesh, const MaterialHandle& material, const PushConstants* pushConstants = nullptr, size_t instanceCount = 0);
        void removeFromDrawQueue(const RenderEntry& entry);
		void emptyQueue();
	};

	enum class UniformBlockSemantics {
		PushConstants = 0,
		Pass = 1,
		Material = 2,
	};

	enum class BufferMode {
		StaticDraw = 0,
		DynamicDraw = 1,
	};

	// TODO: There is no way to free VRAM at present
	// It may be neat to cache all allocated resources in the context for deletion when the context gets destroyed
	// UNLESS a resource group is provided to push it into instead; then that group can be freed in one go
	// and we don't have to go remove elements from the context's own delete queue.
	class RenderingContext {
	protected:
        unsigned int screenWidth = 32;
        unsigned int screenHeight = 32;

		HandlePool<MeshHandle> meshes; // allocated meshes, used during drawPass but may be redundant
		HandleDict<std::string, ShaderStageHandle> shaderStagePool; // file path or source code to shader stage map
		HandleDict<size_t, ShaderHandle> shaderPool; // hash of the shader stages used by the shader to shader map
		std::unordered_map<size_t, std::unordered_map<int, UniformInfo>> shaderUniformInfo; // shader identifier to uniform info map

		const MeshHandle& registerMesh(const MeshHandle& handle);
		const ShaderStageHandle& registerShaderStage(const char* glslFilePath, const ShaderStageHandle& handle);
		const ShaderHandle& registerShader(size_t hash, const ShaderHandle& handle, const std::unordered_map<int, UniformInfo>& uniformBlocks);

		virtual std::unordered_map<int, UniformInfo> getUniformBlocks(const ShaderHandle& shader, const std::vector<ShaderStageHandle>& stages) const = 0;
		virtual ShaderStageHandle createShaderStage(const char* glslFilePath) = 0;
		virtual ShaderHandle createShader(const std::vector<ShaderStageHandle>& stages) = 0;

		// CPU, 1 created per requested uniform block / material
		std::vector<UniformResources*> materialResources;

		static size_t hashMeshLayout(const std::vector<MeshAttribute>& attributes);

        template<typename T> static size_t hashHandles(const T* handles, size_t count) {
            size_t seed = count;
            for (size_t i = 0; i < count; ++i)
                seed = TT::hashCombine(seed, handles[i].identifier());
            return seed;
        }

	public:
		virtual void windowResized(unsigned int width, unsigned int height) { screenWidth = width; screenHeight = height; }
        void resolution(unsigned int& width, unsigned int& height) const { width = screenWidth; height = screenHeight; }

		virtual void beginFrame() = 0;
		virtual void endFrame() = 0;
		virtual void drawPass(const RenderPass& pass, unsigned int defaultFramebuffer = 0) = 0;

		virtual BufferHandle createBuffer(size_t size, unsigned char* data = nullptr, BufferMode mode = BufferMode::StaticDraw) = 0;
		virtual MeshHandle createMesh(
            size_t numElements, // num vertices if indexData == nullptr, else num indices
            BufferHandle vertexData, 
            const std::vector<MeshAttribute>& attributeLayout,
            BufferHandle* indexData = nullptr,
            PrimitiveType primitiveType = PrimitiveType::Triangle,
            size_t numInstances = 0, // mesh is not instanced if numInstances == 0
            BufferHandle* instanceData = nullptr, // ignored if numInstances == 0
            const std::vector<MeshAttribute>& instanceAttributeLayout = {}) = 0; // ignored if numInstances == 0 or instanceData == nullptr
		virtual ShaderStageHandle fetchShaderStage(const char* glslFilePath);
		virtual ShaderHandle fetchShader(const std::vector<ShaderStageHandle>& stages);
		virtual UniformBlockHandle createUniformBuffer(const ShaderHandle& shader, const UniformBlockSemantics& semantic);
		virtual MaterialHandle createMaterial(const ShaderHandle& shader, MaterialBlendMode blendMode = MaterialBlendMode::Opaque);
		virtual ImageHandle createImage(unsigned int width, unsigned int height, ImageFormat format, ImageInterpolation interpolation = ImageInterpolation::Linear, ImageTiling tiling = ImageTiling::Repeat, const unsigned char* data = nullptr) = 0;
		NullableHandle<ImageHandle> loadImage(const char* filePath, ImageInterpolation interpolation = ImageInterpolation::Linear, ImageTiling tiling = ImageTiling::Repeat);
		virtual void imageSize(const ImageHandle& image, unsigned int& width, unsigned int& height) const = 0;
		virtual void framebufferSize(const FramebufferHandle& framebuffer, unsigned int& width, unsigned int& height) const = 0;
		virtual void resizeImage(const ImageHandle& image, unsigned int width, unsigned int height) = 0;
        virtual void resizeFramebuffer(const FramebufferHandle& framebuffer, unsigned int width, unsigned int height) = 0;
		virtual FramebufferHandle createFramebuffer(const std::vector<ImageHandle>& colorAttachments, const ImageHandle* depthStencilAttachment = nullptr) = 0;
        virtual void dispatchCompute(const MaterialHandle& material, unsigned int x, unsigned int y, unsigned int z) = 0;

		virtual void deleteBuffer(const BufferHandle& buffer) = 0;
		virtual void deleteMesh(const MeshHandle& mesh) = 0;
		// virtual void deleteShaderStage(const ShaderStageHandle& mesh) = 0;
		virtual void deleteShader(const ShaderHandle& mesh) = 0;
		virtual void deleteImage(const ImageHandle& mesh) = 0;
		virtual void deleteMaterial(const MaterialHandle& material) = 0;
		virtual void deleteUniformBuffer(const UniformBlockHandle& material) = 0;
		virtual void deleteFramebuffer(const FramebufferHandle& material) = 0;
	};
}

template<> struct std::hash<TTRendering::MaterialHandle> {
    size_t operator()(const TTRendering::MaterialHandle& s) const noexcept {
        return TT::hashCombine(s._shader.identifier(), (size_t)s._resources);
    }
};

#undef BEFRIEND_CONTEXTS
