#include "tt_rendering.h"

#include "../../tt_cpplib/tt_messages.h"
#include "stb/stb_image.h"
#include "tt_meshloader.h"

#include <filesystem>

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
		BufferHandle* indexBuffer,
        size_t numInstances,
        BufferHandle* instanceBuffer) :
		HandleBase(identifier),
        _meshLayoutHash(meshLayoutHash),
		_vertexBuffer(vertexBuffer),
		_numElements(numElements),
		_primitiveType(primitiveType),
        _numInstances(numInstances) {

        if (indexBuffer) _indexBuffer = *indexBuffer;
        if (instanceBuffer) _instanceBuffer = *instanceBuffer;

        _indexType = IndexType::None;
        if(indexBuffer) {
            size_t indexElementSize = indexBuffer->size() / numElements;
            switch(indexElementSize) {
            case 1:
                _indexType = IndexType::U8;
                break;
            case 2:
                _indexType = IndexType::U16;
                break;
            case 4:
                _indexType = IndexType::U32;
                break;
            default:
                TT::assert(false);
                break;
            }
        }
    }
	const BufferHandle MeshHandle::vertexBuffer() const { return _vertexBuffer; }
	size_t MeshHandle::numElements() const { return _numElements; }
    const BufferHandle* MeshHandle::indexBuffer() const { return _indexBuffer == BufferHandle::Null ? nullptr : &_indexBuffer; }
	PrimitiveType MeshHandle::primitiveType() const { return _primitiveType; }
	IndexType MeshHandle::indexType() const { return _indexType; }

	ImageHandle::ImageHandle(size_t identifier, ImageFormat format, ImageInterpolation interpolation, ImageTiling tiling) :
		HandleBase(identifier), _format(format), _interpolation(interpolation), _tiling(tiling) {}

	FramebufferHandle::FramebufferHandle(size_t identifier, const std::vector<ImageHandle>& colorAttachments, const ImageHandle* depthStencilAttachment) :
		HandleBase(identifier), _colorAttachments(colorAttachments) {
		if (depthStencilAttachment)
			_depthStencilAttachment = *depthStencilAttachment;
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
		return type == rhs.type && offset == rhs.offset && name == rhs.name && arraySize == rhs.arraySize;
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
		memcpy(_resources->uniformBuffer + info->offset, src, sizeOfUniformType(srcType));
		return true;
	}

	UniformBlockHandle::UniformBlockHandle(const UniformInfo& uniformInfo, UniformResources* resources) :
		_uniformInfo(&uniformInfo), _resources(resources) {
	}

	UniformBlockHandle::UniformBlockHandle(UniformResources* resources) :
		_uniformInfo(nullptr), _resources(resources) {
	}

	size_t UniformBlockHandle::size() const { return _uniformInfo ? _uniformInfo->bufferSize : 0; }
    unsigned char* UniformBlockHandle::cpuBuffer() const { if (!_resources) return nullptr; return _resources->uniformBuffer; }

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
        if (!_resources) return false;
        _resources->images.insert(key, image);
        return true;
    }

    bool UniformBlockHandle::set(size_t binding, const BufferHandle& buffer) {
        if (!_resources) return false;
        _resources->ssbos.insert(binding, buffer);
        return true;
    }

	MaterialHandle::MaterialHandle(const ShaderHandle& shader, const UniformInfo& uniformInfo, UniformResources* resources, MaterialBlendMode blendMode) :
		UniformBlockHandle(uniformInfo, resources), _shader(shader), _blendMode(blendMode) {
	}

	MaterialHandle::MaterialHandle(const ShaderHandle& shader, UniformResources* resources, MaterialBlendMode blendMode) : UniformBlockHandle(resources), _shader(shader), _blendMode(blendMode) {}

	const ShaderHandle& MaterialHandle::shader() const { return _shader; }
	
	MaterialBlendMode MaterialHandle::blendMode() const { return _blendMode; }

    MeshQueue& MaterialQueue::fetch(const MaterialHandle& key, size_t& index) {
		// Ensure we have a queue with this material instance
		auto it = materialIdentifierToQueueIndex.find((size_t)&key);
		if (it == materialIdentifierToQueueIndex.end()) {
			materialIdentifierToQueueIndex[(size_t)&key] = queues.size();
			keys.push_back(key); // store a copy of all handle info because materials are not managed by the context
            index = queues.size();
			return queues.emplace_back();
		} else {
            index = it->second;
			return queues[it->second];
        }
	}

	MaterialQueue& ShaderQueue::fetch(const ShaderHandle& key, size_t& index) {
		// Ensure we have a queue with this shader
		auto it = shaderIdentifierToQueueIndex.find(key.identifier());
		if (it == shaderIdentifierToQueueIndex.end()) {
			shaderIdentifierToQueueIndex[key.identifier()] = queues.size();
			keys.push_back(key);
            index = queues.size();
			return queues.emplace_back();
		} else {
            index = it->second;
			return queues[it->second];
        }
	}

	ShaderQueue& DrawQueue::fetch(size_t key, size_t& index) {
		// Ensure we have a queue with this mesh layout
		auto it = meshLayoutHashToQueueIndex.find(key);
		if (it == meshLayoutHashToQueueIndex.end()) {
			meshLayoutHashToQueueIndex[key] = queues.size();
			keys.push_back(key);
            index = queues.size();
			return queues.emplace_back();
		} else {
            index = it->second;
			return queues[it->second];
        }
	}

	void RenderPass::setPassUniforms(UniformBlockHandle handle) {
		passUniforms = handle;
		modified = true;
	}

	void RenderPass::clearPassUniforms() {
        passUniforms = UniformBlockHandle::Null;
		modified = true;
	}

	void RenderPass::setFramebuffer(FramebufferHandle handle) {
		_framebuffer = handle;
		modified = true;
	}

	void RenderPass::clearFramebuffer() {
        _framebuffer = FramebufferHandle::Null;
		modified = true;
	}

    RenderEntry RenderPass::addToDrawQueue(const MeshHandle& mesh, const MaterialHandle& material, const PushConstants* pushConstants, size_t instanceCount) {
        RenderEntry result;
        auto& queue = _drawQueue
            .fetch(mesh._meshLayoutHash, result.meshLayoutQueueIndex)
            .fetch(material._shader, result.shaderQueueIndex)
            .fetch(material, result.materialQueueIndex);
        result.meshIndex = queue.next++;
        DrawInfo& e = queue.orderedQueue[result.meshIndex];
        e.meshIdentifier = mesh.identifier();
        e.instanceCount = instanceCount;
        e.pushConstants = pushConstants;
        modified = true;
        return result;
    }

    void RenderPass::removeFromDrawQueue(const RenderEntry& entry) {
        _drawQueue.queues[entry.meshLayoutQueueIndex].queues[entry.shaderQueueIndex].queues[entry.materialQueueIndex].orderedQueue.erase(entry.meshIndex);
        modified = true;
    }

    void RenderPass::emptyQueue() {
        _drawQueue.meshLayoutHashToQueueIndex.clear();
        _drawQueue.keys.clear();
        _drawQueue.queues.clear();
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

    void RenderingContext::deregisterMesh(const MeshHandle& handle) {
        meshes.remove(handle);
    }
    
    void RenderingContext::deregisterShaderStage(const ShaderStageHandle& handle) {
        shaderStagePool.removeValue(handle);
    }
    
    void RenderingContext::deregisterShader(const ShaderHandle& handle) {
        shaderPool.removeValue(handle);
        shaderUniformInfo.erase(handle.identifier());
    }

    const UniformInfo* RenderingContext::materialUniformInfo(const ShaderHandle& handle) const {
        // Get material uniform block info for the given shader
        TT::assert(shaderUniformInfo.find(handle.identifier()) != shaderUniformInfo.end());
        const auto& uniformBlocks = shaderUniformInfo.find(handle.identifier())->second;
        auto it = uniformBlocks.find((int)UniformBlockSemantics::Material);
        const UniformInfo* uniformInfo = nullptr;
        if (it != uniformBlocks.end())
            uniformInfo = &it->second;
        return uniformInfo;
    }

	MaterialHandle RenderingContext::createMaterial(const ShaderHandle& shader, MaterialBlendMode blendMode) {
		TT::assert(shaderUniformInfo.contains(shader.identifier()));
		const std::unordered_map<int, UniformInfo>& info = shaderUniformInfo.find(shader.identifier())->second;
		auto it = info.find((int)UniformBlockSemantics::Material);
		if (it != info.end()) {
            materialResources.push_back(new UniformResources {new unsigned char[it->second.bufferSize], {}, {} });
			MaterialHandle handle(shader, it->second, materialResources.back(), blendMode);
            return handle;
		}
        materialResources.push_back(new UniformResources {nullptr, {}, {} });
		MaterialHandle handle(shader, materialResources.back(), blendMode);
        return handle;
	}

	UniformBlockHandle RenderingContext::createUniformBuffer(const ShaderHandle& shader, const UniformBlockSemantics& semantic) {
		// Will find info for the given block type in the shader and allocate a buffer that is large enough to hold the uniforms for that block.
		// May return an invalid block if the shader has no such block type.
		TT::assert(shaderUniformInfo.contains(shader.identifier()));
		const std::unordered_map<int, UniformInfo>& info = shaderUniformInfo.find(shader.identifier())->second;
		auto it = info.find((int)semantic);
		if (it != info.end()) {
            materialResources.push_back(new UniformResources { new unsigned char[it->second.bufferSize], {}, {} });
			return UniformBlockHandle(it->second, materialResources.back());
		}
        materialResources.push_back(new UniformResources {nullptr, {}, {} });
		return UniformBlockHandle(materialResources.back());
	}

    void RenderingContext::deleteMaterial(const MaterialHandle& material) {
        auto it = std::find(materialResources.begin(), materialResources.end(), material._resources);
        TT::assert(it != materialResources.end());
        if(it != materialResources.end())
            materialResources.erase(it);
    }

    void RenderingContext::deleteUniformBuffer(const UniformBlockHandle& uniformBuffer) {
        auto it = std::find(materialResources.begin(), materialResources.end(), uniformBuffer._resources);
        TT::assert(it != materialResources.end());
        if(it != materialResources.end())
            materialResources.erase(it);
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

	ImageHandle RenderingContext::loadImage(const char* filePath, ImageInterpolation interpolation, ImageTiling tiling) {
		int width, height, channels;
		unsigned char* data = stbi_load(filePath, &width, &height, &channels, 0);
		ImageFormat format;
		switch (channels) {
		case 1: format = ImageFormat::R8; break;
		case 2: format = ImageFormat::RG8; break;
		case 3: format = ImageFormat::RGB8; break;
		case 4: format = ImageFormat::RGBA8; break;
        default: TT::error("Invalid image: %s", filePath); return ImageHandle::Null;
		}
		ImageHandle r = createImage(width, height, format, interpolation, tiling, data);
		stbi_image_free(data);
		return { r };
	}

    MeshFileInfo RenderingContext::loadMesh(const char* fbxFilePath) {
        TT::FbxExtractor scene(fbxFilePath);

        // Bidirectional LUT to go from mesh handle to material and back
        MeshFileInfo result;
        std::vector<std::vector<MeshHandle>> subMeshesByMaterialId;
        // To save space we push material names into this vector
        std::vector<std::string> materialNames;

        
        // To speed up the internal check of known material names we can use a map
        std::unordered_map<std::string, size_t> materialNameToId;
        // To redirect transform mesh indices to (valid) meshes we need this internal map
        std::unordered_map<size_t, std::vector<std::pair<size_t, size_t>>> multiMeshToSubMeshIndices;

        // Upload meshes
        for (size_t j = 0; j < scene.meshes().size(); ++j) {
            const auto& mesh = scene.meshes()[j];
            if (!mesh.attributeCount || !mesh.meshCount) continue;

            std::vector<MeshAttribute> layout;
            unsigned int vertexStride = 0;
            for(unsigned int i = 0; i < mesh.attributeCount;++i) {
                if (mesh.attributeLayout[i].numElements == NumElements::Invalid) continue;
                MeshAttribute entry;
                entry.location = (unsigned char)mesh.attributeLayout[i].semantic;
                entry.dimensions = (MeshAttribute::Dimensions)((unsigned char)mesh.attributeLayout[i].numElements - 1);
                switch(mesh.attributeLayout[i].elementType) {
                case ElementType::UInt32:
                    entry.elementType = MeshAttribute::ElementType::U32;
                    break;
                case ElementType::Float:
                    entry.elementType = MeshAttribute::ElementType::F32;
                    break;
                default:
                    TT::assert(false);
                    continue;
                }
                layout.push_back(entry);
                vertexStride += 4 * (unsigned int)mesh.attributeLayout[i].numElements;
            }
            if(vertexStride == 0) {
                TT::assert(false);
                continue;
            }
            
            std::vector<std::pair<size_t, size_t>> uploadedMeshIndices;
            for(unsigned int i = 0; i < mesh.meshCount; ++i) {
                const auto& subMesh = mesh.meshes[i];
                if(subMesh.materialId >= mesh.materialNameCount) {
                    TT::assert(false);
                    continue;
                }
                auto vbo = createBuffer(subMesh.vertexDataSizeInBytes, subMesh.vertexDataBlob);
                MeshHandle gpuMesh = MeshHandle::Null;
                if(subMesh.indexDataSizeInBytes) {
                    auto ibo = createBuffer(subMesh.indexDataSizeInBytes, subMesh.indexDataBlob);
                    gpuMesh = createMesh(subMesh.indexDataSizeInBytes / mesh.indexElementSizeInBytes, vbo, layout, &ibo);
                } else {
                    gpuMesh = createMesh(subMesh.vertexDataSizeInBytes / vertexStride, vbo, layout);
                }

                std::string materialName(mesh.materialNames[subMesh.materialId].buffer, mesh.materialNames[subMesh.materialId].length);
                const auto& it = materialNameToId.find(materialName);
                size_t materialId;
                if (it == materialNameToId.end()) {
                    materialNameToId[materialName] = materialNames.size();
                    materialId = materialNames.size();
                    materialNames.push_back(materialName);
                    subMeshesByMaterialId.emplace_back();
                } else {
                    materialId = it->second;
                }

                uploadedMeshIndices.push_back({materialId, subMeshesByMaterialId[materialId].size()});
                subMeshesByMaterialId[materialId].push_back(gpuMesh);
            }

            if (uploadedMeshIndices.size() == 0) continue;
            multiMeshToSubMeshIndices[j] = uploadedMeshIndices;
        }

        // Convert nodes
        const auto& nodes = scene.nodes();
        std::vector<TransformInfo> transforms(nodes.size());
        for(size_t i = 0 ;i < nodes.size(); ++i) {
            const auto& node = nodes[i];
            
            if (node.parentIndex != -1)
                transforms[i].parent = &transforms[node.parentIndex];
            else
                transforms[i].parent = nullptr;
            
            const auto& it = multiMeshToSubMeshIndices.find(nodes[i].meshIndex);
            if (it != multiMeshToSubMeshIndices.end())
                transforms[i].meshIndices = it->second;
            else
                transforms[i].meshIndices = {};

            transforms[i].translate.x = (float)node.translateX;
            transforms[i].translate.y = (float)node.translateY;
            transforms[i].translate.z = (float)node.translateZ;
            transforms[i].radians.x = (float)node.rotateX;
            transforms[i].radians.y = (float)node.rotateY;
            transforms[i].radians.z = (float)node.rotateZ;
            transforms[i].scale.x = (float)node.scaleX;
            transforms[i].scale.y = (float)node.scaleY;
            transforms[i].scale.z = (float)node.scaleZ;

            // TODO: Verify this is not reversed, i.e. my XYZ is not their ZYX. Since I already remap during extraction we can easily fix that in the rotateOrderInts table in the extractor.
            transforms[i].rotateOrder = (TT::ERotateOrder)node.rotateOrder;
        }

        return { subMeshesByMaterialId, transforms, materialNames };
    }

    const BufferHandle BufferHandle::Null(0, 0);
    const MeshHandle MeshHandle::Null(0, 0, BufferHandle::Null, 0, PrimitiveType::Line);
    const ImageHandle ImageHandle::Null(0, TTRendering::ImageFormat::RGBA32F, TTRendering::ImageInterpolation::Linear, TTRendering::ImageTiling::Clamp);
    const FramebufferHandle FramebufferHandle::Null(0, {}, nullptr);
    const ShaderStageHandle ShaderStageHandle::Null(0, ShaderStageHandle::ShaderStage::Compute);
    const ShaderHandle ShaderHandle::Null(0);
    const MaterialHandle MaterialHandle::Null(ShaderHandle::Null, nullptr);
    const UniformBlockHandle UniformBlockHandle::Null(nullptr);
}
