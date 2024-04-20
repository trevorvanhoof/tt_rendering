#include "tt_rendering.h"

#include <tt_messages.h>
#include <ThirdParty/stb/stb_image.h>

namespace TTRendering {
	HandleBase::HandleBase(size_t identifier) : _identifier(identifier) {}
	size_t HandleBase::identifier() const { return _identifier; }
	
	BufferHandle::BufferHandle(size_t identifier, size_t size) : HandleBase(identifier), _size(size) {}
	size_t BufferHandle::size() const { return _size; }
	
	MeshHandle::MeshHandle(size_t identifier,
		size_t meshLayoutHash,
		BufferHandle vertexBuffer,
		size_t numElements,
		PrimitiveType primitiveType,
		IndexType indexType,
		BufferHandle* indexBuffer,
        size_t numIstances,
        BufferHandle* instanceBuffer) :
		HandleBase(identifier),
        _meshLayoutHash(meshLayoutHash),
		_vertexBuffer(vertexBuffer),
		_numElements(numElements),
		_primitiveType(primitiveType),
		_indexType(indexType),
        _indexBuffer(indexBuffer),
        _numIstances(numIstances),
        _instanceBuffer(instanceBuffer) {}
	const BufferHandle MeshHandle::vertexBuffer() const { return _vertexBuffer; }
	size_t MeshHandle::numElements() const { return _numElements; }
	const BufferHandle* MeshHandle::indexBuffer() const { return _indexBuffer.value(); }
	PrimitiveType MeshHandle::primitiveType() const { return _primitiveType; }
	IndexType MeshHandle::indexType() const { return _indexType; }

	ImageHandle::ImageHandle(size_t identifier, ImageFormat format, ImageInterpolation interpolation, ImageTiling tiling) :
		HandleBase(identifier), _format(format), _interpolation(interpolation), _tiling(tiling) {}

	ImageFormat ImageHandle::format() const { return _format; }
	
	FramebufferHandle::FramebufferHandle(size_t identifier, const std::vector<ImageHandle>& colorAttachments, ImageHandle* depthStencilAttachment) :
		HandleBase(identifier), _colorAttachments(colorAttachments) {
		if (depthStencilAttachment)
			_depthStencilAttachment.set(*depthStencilAttachment);
	}

	ShaderStageHandle::ShaderStageHandle(size_t identifier, ShaderStage stage) : 
		HandleBase(identifier), _stage(stage) {
	}

	ShaderStageHandle::ShaderStage ShaderStageHandle::stage() const {
		return _stage;
	}

	bool UniformInfo::operator==(const UniformInfo& rhs) {
		if (bufferSize != rhs.bufferSize)
			return false;
		if (fields.size() != rhs.fields.size())
			return false;
		for (size_t i = 0; i < fields.size(); ++i) {
			if (fields[i] != rhs.fields[i])
				return false;
		}
		return true;
	}

	bool UniformInfo::Field::operator==(const UniformInfo::Field& rhs) {
		return type == rhs.type && offset == rhs.offset && name == rhs.name;
	}

	const UniformInfo::Field* UniformInfo::find(const char* key) const {
		static std::hash<std::string> stringHasher;
		size_t hash = stringHasher(key);
		auto it = nameHashToFieldIndex.find(hash);
		if (it == nameHashToFieldIndex.end())
			return nullptr;
		return &fields[it->second];
	}

	namespace {
		size_t sizeOfUniformType(UniformType type) {
			switch (type) {
			case UniformType::Float:
			case UniformType::Int:
			case UniformType::UInt:
			case UniformType::Bool:
                // case UniformType::Image:
				return 4;
			case UniformType::Vec2:
			case UniformType::IVec2:
			case UniformType::UVec2:
			case UniformType::BVec2:
				return 4 * 2;
			case UniformType::Vec3:
			case UniformType::IVec3:
			case UniformType::UVec3:
			case UniformType::BVec3:
				return 4 * 3;
			case UniformType::Vec4:
			case UniformType::IVec4:
			case UniformType::UVec4:
			case UniformType::BVec4:
			case UniformType::Mat2:
				return 4 * 4;
			case UniformType::Mat3:
				return 4 * 9;
			case UniformType::Mat4:
				return 4 * 16;
			}
			TT::assert(false);
			return 0;
		}
	}

	bool UniformBlockHandle::_setUniform(const char* key, void* src, UniformType srcType, unsigned int count) {
		if (!_uniformInfo)
			return false;
		const UniformInfo::Field* info = _uniformInfo->find(key);
		if (!info) return false;
		if (info->type != srcType)
			return false;
        if(info->arraySize != count)
            return false;
		memcpy(_uniformBuffer + info->offset, src, sizeOfUniformType(srcType));
		return true;
	}

	UniformBlockHandle::UniformBlockHandle(const UniformInfo& uniformInfo, unsigned char*& uniformBuffer) :
		_uniformInfo(&uniformInfo), _uniformBuffer(uniformBuffer) {
	}

	UniformBlockHandle::UniformBlockHandle() :
		_uniformInfo(nullptr), _uniformBuffer(nullptr) {
	}

	size_t UniformBlockHandle::size() const { return _uniformInfo ? _uniformInfo->bufferSize : 0; }
	unsigned char* UniformBlockHandle::cpuBuffer() const { return _uniformBuffer; }

	bool UniformBlockHandle::hasUniformBlock() const { return _uniformInfo != nullptr; }

	bool UniformBlockHandle::set(const char* key, float x) { return _setUniform(key, &x, UniformType::Float); }
	bool UniformBlockHandle::set(const char* key, float x, float y) { float vec[] = { x, y }; return _setUniform(key, &vec, UniformType::Vec2); }
	bool UniformBlockHandle::set(const char* key, float x, float y, float z) { float vec[] = { x, y, z }; return _setUniform(key, &vec, UniformType::Vec3); }
	bool UniformBlockHandle::set(const char* key, float x, float y, float z, float w) {
		float vec[] = { x, y, z, w };
		// Could be either vec4 or mat2
		if (!_setUniform(key, &vec, UniformType::Vec4))
			return _setUniform(key, &vec, UniformType::Mat2);
		return true;
	}
    
	bool UniformBlockHandle::set(const char* key, TT::Mat22 m) { return _setUniform(key, &m, UniformType::Mat2); }
	bool UniformBlockHandle::set(const char* key, TT::Mat33 m) { return _setUniform(key, &m, UniformType::Mat3); }
	bool UniformBlockHandle::set(const char* key, TT::Mat44 m) { return _setUniform(key, &m, UniformType::Mat4); }
	bool UniformBlockHandle::set(const char* key, TT::Vec2 vec) { return _setUniform(key, &vec, UniformType::Vec2); }
	bool UniformBlockHandle::set(const char* key, TT::Vec3 vec) { return _setUniform(key, &vec, UniformType::Vec3); }
	bool UniformBlockHandle::set(const char* key, TT::Vec4 vec) { return _setUniform(key, &vec, UniformType::Vec4); }

	bool UniformBlockHandle::set(const char* key, int x) { return _setUniform(key, &x, UniformType::Int); }
	bool UniformBlockHandle::set(const char* key, int x, int y) { int vec[] = { x, y }; return _setUniform(key, &vec, UniformType::IVec2); }
	bool UniformBlockHandle::set(const char* key, int x, int y, int z) { int vec[] = { x, y, z }; return _setUniform(key, &vec, UniformType::IVec3); }
	bool UniformBlockHandle::set(const char* key, int x, int y, int z, int w) { int vec[] = { x, y, z, w }; return _setUniform(key, &vec, UniformType::IVec4); }

	bool UniformBlockHandle::set(const char* key, unsigned int x) { return _setUniform(key, &x, UniformType::UInt); }
	bool UniformBlockHandle::set(const char* key, unsigned int x, unsigned int y) { unsigned int vec[] = { x, y }; return _setUniform(key, &vec, UniformType::UVec2); }
	bool UniformBlockHandle::set(const char* key, unsigned int x, unsigned int y, unsigned int z) { unsigned int vec[] = { x, y, z }; return _setUniform(key, &vec, UniformType::UVec3); }
	bool UniformBlockHandle::set(const char* key, unsigned int x, unsigned int y, unsigned int z, unsigned int w) { unsigned int vec[] = { x, y, z, w }; return _setUniform(key, &vec, UniformType::UVec4); }

	bool UniformBlockHandle::set(const char* key, bool x) { return _setUniform(key, &x, UniformType::Bool); }
	bool UniformBlockHandle::set(const char* key, bool x, bool y) { int vec[] = { x, y }; return _setUniform(key, &vec, UniformType::BVec2); }
	bool UniformBlockHandle::set(const char* key, bool x, bool y, bool z) { int vec[] = { x, y, z }; return _setUniform(key, &vec, UniformType::BVec3); }
	bool UniformBlockHandle::set(const char* key, bool x, bool y, bool z, bool w) { int vec[] = { x, y, z, w }; return _setUniform(key, &vec, UniformType::BVec4); }

	bool UniformBlockHandle::setFloat(const char* key, float* value, unsigned int count) { return _setUniform(key, value, UniformType::Float, count); }
	bool UniformBlockHandle::setVec2(const char* key, float* value, unsigned int count) { return _setUniform(key, value, UniformType::Vec2, count); }
	bool UniformBlockHandle::setVec3(const char* key, float* value, unsigned int count) { return _setUniform(key, value, UniformType::Vec3, count); }
	bool UniformBlockHandle::setVec4(const char* key, float* value, unsigned int count) { return _setUniform(key, value, UniformType::Vec4, count); }
	bool UniformBlockHandle::setMat2(const char* key, float* value, unsigned int count) { return _setUniform(key, value, UniformType::Mat2, count); }
	bool UniformBlockHandle::setMat3(const char* key, float* value, unsigned int count) { return _setUniform(key, value, UniformType::Mat3, count); }
	bool UniformBlockHandle::setMat4(const char* key, float* value, unsigned int count) { return _setUniform(key, value, UniformType::Mat4, count); }

	bool UniformBlockHandle::setInt(const char* key, int* value, unsigned int count) { return _setUniform(key, value, UniformType::Int, count); }
	bool UniformBlockHandle::setIVec2(const char* key, int* value, unsigned int count) { return _setUniform(key, value, UniformType::IVec2, count); }
	bool UniformBlockHandle::setIVec3(const char* key, int* value, unsigned int count) { return _setUniform(key, value, UniformType::IVec3, count); }
	bool UniformBlockHandle::setIVec4(const char* key, int* value, unsigned int count) { return _setUniform(key, value, UniformType::IVec4, count); }

	bool UniformBlockHandle::setUInt(const char* key, unsigned int* value, unsigned int count) { return _setUniform(key, value, UniformType::UInt, count); }
	bool UniformBlockHandle::setUVec2(const char* key, unsigned int* value, unsigned int count) { return _setUniform(key, value, UniformType::UVec2, count); }
	bool UniformBlockHandle::setUVec3(const char* key, unsigned int* value, unsigned int count) { return _setUniform(key, value, UniformType::UVec3, count); }
	bool UniformBlockHandle::setUVec4(const char* key, unsigned int* value, unsigned int count) { return _setUniform(key, value, UniformType::UVec4, count); }

	bool UniformBlockHandle::setBool(const char* key, int* value, unsigned int count) { return _setUniform(key, value, UniformType::Bool, count); }
	bool UniformBlockHandle::setBVec2(const char* key, int* value, unsigned int count) { return _setUniform(key, value, UniformType::BVec2, count); }
	bool UniformBlockHandle::setBVec3(const char* key, int* value, unsigned int count) { return _setUniform(key, value, UniformType::BVec3, count); }
	bool UniformBlockHandle::setBVec4(const char* key, int* value, unsigned int count) { return _setUniform(key, value, UniformType::BVec4, count); }

    bool UniformBlockHandle::set(const char* key, const ImageHandle& image) { 
        _images.insert(key, image);
        return true;
    }

    bool UniformBlockHandle::set(size_t binding, const BufferHandle& buffer) {
        _ssbos.insert(binding, buffer);
        return true;
    }

	MaterialHandle::MaterialHandle(const ShaderHandle& shader, const UniformInfo& uniformInfo, unsigned char*& uniformBuffer, MaterialBlendMode blendMode) :
		UniformBlockHandle(uniformInfo, uniformBuffer), _shader(shader), _blendMode(blendMode) {
	}

	MaterialHandle::MaterialHandle(const ShaderHandle& shader, MaterialBlendMode blendMode) : UniformBlockHandle(), _shader(shader), _blendMode(blendMode) {}

	const ShaderHandle& MaterialHandle::shader() const { return _shader; }
	
	MaterialBlendMode MaterialHandle::blendMode() const { return _blendMode; }

	std::vector<RenderPass::DrawInfo>& RenderPass::DrawQueue::ShaderQueue::MaterialQueue::fetch(const MaterialHandle& key) {
		// Ensure we have a queue with this material instance
		auto it = materialIdentifierToQueueIndex.find((size_t)&key);
		if (it == materialIdentifierToQueueIndex.end()) {
			materialIdentifierToQueueIndex[(size_t)&key] = queues.size();
			keys.push_back(key); // store a copy of all handle info because materials are not managed by the context
			return queues.emplace_back();
		}
		else
			return queues[it->second];
	}

	RenderPass::DrawQueue::ShaderQueue::MaterialQueue& RenderPass::DrawQueue::ShaderQueue::fetch(size_t key) {
		// Ensure we have a queue with this shader
		auto it = shaderIdentifierToQueueIndex.find(key);
		if (it == shaderIdentifierToQueueIndex.end()) {
			shaderIdentifierToQueueIndex[key] = queues.size();
			keys.push_back(key);
			return queues.emplace_back();
		}
		else
			return queues[it->second];
	}

	RenderPass::DrawQueue::ShaderQueue& RenderPass::DrawQueue::fetch(size_t key) {
		// Ensure we have a queue with this mesh layout
		auto it = meshLayoutHashToQueueIndex.find(key);
		if (it == meshLayoutHashToQueueIndex.end()) {
			meshLayoutHashToQueueIndex[key] = queues.size();
			keys.push_back(key);
			return queues.emplace_back();
		}
		else
			return queues[it->second];
	}

	void RenderPass::setPassUniforms(UniformBlockHandle handle) {
		passUniforms.set(handle);
		modified = true;
	}

	void RenderPass::clearPassUniforms() {
		passUniforms.clear();
		modified = true;
	}

	void RenderPass::setFramebuffer(FramebufferHandle handle) {
		framebuffer.set(handle);
		modified = true;
	}

	void RenderPass::clearFramebuffer() {
		framebuffer.clear();
		modified = true;
	}

    PushConstants* RenderPass::addToDrawQueue(const MeshHandle& mesh, const MaterialHandle& material, const PushConstants& pushConstants, size_t instanceCount) {
        auto& queue = drawQueue.fetch(mesh._meshLayoutHash).fetch(material._shader.identifier()).fetch(material);
        queue.push_back(DrawInfo(mesh.identifier(), pushConstants, instanceCount));
        modified = true;
        return &queue.back().pushConstants;
    }

    void RenderPass::emptyQueue() {
        drawQueue.meshLayoutHashToQueueIndex.clear();
        drawQueue.keys.clear();
        drawQueue.queues.clear();
        modified = true;
    }

	namespace {
		unsigned short hashMeshAttribute(const MeshAttribute& attribute) {
			return ((unsigned short)attribute.location << 8u) | ((unsigned short)attribute.elementType << 2u) | (unsigned short)attribute.dimensions;
		}
    }

    size_t RenderingContext::hashMeshLayout(const std::vector<MeshAttribute>& attributes) {
        static_assert(sizeof(size_t) == 8);
        size_t finalHash = 0;
        size_t localHash = 0;
        size_t index = 0;
        for (const MeshAttribute& attribute : attributes) {
            localHash <<= 16;
            localHash |= hashMeshAttribute(attribute);
            index++;
            if (index % 4 == 0) {
                finalHash = TT::hashCombine(finalHash, localHash);
                localHash = 0;
            }
        }
        if (index % 4 != 0) {
            finalHash = TT::hashCombine(finalHash, localHash);
            localHash = 0;
        }
        return finalHash;
    }

    size_t RenderingContext::hashHandles(const HandleBase* handles, size_t count) {
        if (!count)
            return 0;
        size_t hash = handles[0].identifier();
        for (size_t i = 1; i < count; ++i)
            hash = TT::hashCombine(hash, handles[i].identifier());
        return hash;
    }

	const ShaderStageHandle& RenderingContext::registerShaderStage(const char* glslFilePath, const ShaderStageHandle& handle) {
		shaderStagePool.insert(glslFilePath, handle);
		return handle;
	}

	const ShaderHandle& RenderingContext::registerShader(size_t hash, const ShaderHandle& handle, const std::unordered_map<int, UniformInfo>& uniformBlocks) {
		shaderPool.insert(hash, handle);
		shaderUniformInfo[handle.identifier()] = uniformBlocks;
		return handle;
	}

	const MeshHandle& RenderingContext::registerMesh(const MeshHandle& handle) {
		meshes.insert(handle);
		return handle;
	}

	MaterialHandle RenderingContext::createMaterial(const ShaderHandle& shader, MaterialBlendMode blendMode) {
		TT::assert(shaderUniformInfo.contains(shader.identifier()));
		const std::unordered_map<int, UniformInfo>& info = shaderUniformInfo.find(shader.identifier())->second;
		auto it = info.find((int)UniformBlockSemantics::Material);
		size_t bufferSize = 0;
		if (it != info.end()) {
			materialUniformBuffers.push_back(new unsigned char[it->second.bufferSize]);
			return MaterialHandle(shader, it->second, materialUniformBuffers.back(), blendMode);
		}
		return MaterialHandle(shader, blendMode);
	}

	UniformBlockHandle RenderingContext::createUniformBuffer(const ShaderHandle& shader, const UniformBlockSemantics& semantic) {
		// Will find info for the given block type in the shader and allocate a buffer that is large enough to hold the uniforms for that block.
		// May return an invalid block if the shader has no such block type.
		TT::assert(shaderUniformInfo.contains(shader.identifier()));
		const std::unordered_map<int, UniformInfo>& info = shaderUniformInfo.find(shader.identifier())->second;
		auto it = info.find((int)semantic);
		if (it != info.end()) {
			materialUniformBuffers.push_back(new unsigned char[it->second.bufferSize]);
			return UniformBlockHandle(it->second, materialUniformBuffers.back());
		}
		return UniformBlockHandle();
	}

	ShaderStageHandle RenderingContext::fetchShaderStage(const char* glslFilePath) {
		if (const ShaderStageHandle* existing = shaderStagePool.find(glslFilePath))
			return *existing;
		return registerShaderStage(glslFilePath, createShaderStage(glslFilePath));
	}

	ShaderHandle RenderingContext::fetchShader(const std::vector<ShaderStageHandle>& stages) {
		size_t hash = hashHandles(stages.data(), stages.size());
		if (const ShaderHandle* existing = shaderPool.find(hash))
			return *existing;
		ShaderHandle shader = createShader(stages);
		return registerShader(hash, shader, getUniformBlocks(shader, stages));
	}

	NullableHandle<ImageHandle> RenderingContext::loadImage(const char* filePath, ImageInterpolation interpolation, ImageTiling tiling) {
		int width, height, channels;
		unsigned char* data = stbi_load(filePath, &width, &height, &channels, 0);
		ImageFormat format;
		switch (channels) {
		case 1: format = ImageFormat::R8; break;
		case 2: format = ImageFormat::RG8; break;
		case 3: format = ImageFormat::RGB8; break;
		case 4: format = ImageFormat::RGBA8; break;
		default: TT::error("Invalid image: %s", filePath); return {};
		}
		ImageHandle r = createImage(width, height, format, interpolation, tiling, data);
		stbi_image_free(data);
		return { r };
	}
}
