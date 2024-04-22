#include "tt_glcontext.h"

#include "tt_gl.h"
#include <tt_window.h>
#include <tt_files.h>

#include <unordered_set>
#include <string>

namespace {
	GLint _getProgrami(GLuint program, GLenum query) {
		GLint v;
		glGetProgramiv(program, query, &v);
		return v;
	}

	std::string _getProgramInfoLog(GLuint program) {
		GLsizei infoLogSize = _getProgrami(program, GL_INFO_LOG_LENGTH);
		GLchar* infoLog = new GLchar[infoLogSize];
		glGetProgramInfoLog(program, infoLogSize, &infoLogSize, infoLog);
		std::string result(infoLog, infoLogSize);
		delete[] infoLog;
		return result;
	}

	GLint _getShaderi(GLuint shader, GLenum query) {
		GLint v;
		glGetShaderiv(shader, query, &v);
		return v;
	}

	std::string _getShaderInfoLog(GLuint shader) {
		GLsizei infoLogSize = _getShaderi(shader, GL_INFO_LOG_LENGTH);
		GLchar* infoLog = new GLchar[infoLogSize];
		glGetShaderInfoLog(shader, infoLogSize, &infoLogSize, infoLog);
		std::string result(infoLog, infoLogSize);
		delete[] infoLog;
		return result;
	}

	GLuint compileShader(const std::string& code, GLenum mode) {
		GLuint shader = glCreateShader(mode);
		const char* codeAptr = code.data();
		GLsizei length = (GLsizei)code.size();
		glShaderSource(shader, 1, &codeAptr, &length);
		glCompileShader(shader);
		if (_getShaderi(shader, GL_COMPILE_STATUS) == GL_FALSE) {
			std::string r = _getShaderInfoLog(shader);
			TT::error("%s\n", r.data());
		}
		return shader;
	}

	GLenum glElementType(TTRendering::MeshAttribute::ElementType type) {
		using namespace TTRendering;
		switch (type) {
		case MeshAttribute::ElementType::I8:
			return GL_UNSIGNED_BYTE;
		case MeshAttribute::ElementType::U8:
			return GL_BYTE;
		case MeshAttribute::ElementType::I16:
			return GL_SHORT;
		case MeshAttribute::ElementType::U16:
			return GL_UNSIGNED_SHORT;
		case MeshAttribute::ElementType::F16:
			return GL_HALF_FLOAT;
		case MeshAttribute::ElementType::I32:
			return GL_INT;
		case MeshAttribute::ElementType::U32:
			return GL_UNSIGNED_INT;
		case MeshAttribute::ElementType::F32:
			return GL_FLOAT;
		case MeshAttribute::ElementType::F64:
			return GL_DOUBLE;
		}
		TT::assert(false);
		return 0;
	}

	TTRendering::UniformType _mapType(GLenum glType) {
		using namespace TTRendering;
		// Table from: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGetActiveUniform.xhtml
		if (glType >= GL_SAMPLER_1D && glType <= GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY)
			return UniformType::Image;
		switch (glType) {
		case GL_FLOAT:
			return UniformType::Float;
		case GL_FLOAT_VEC2:
			return UniformType::Vec2;
		case GL_FLOAT_VEC3:
			return UniformType::Vec3;
		case GL_FLOAT_VEC4:
			return UniformType::Vec4;
		case GL_INT:
			return UniformType::Int;
		case GL_INT_VEC2:
			return UniformType::IVec2;
		case GL_INT_VEC3:
			return UniformType::IVec3;
		case GL_INT_VEC4:
			return UniformType::IVec4;
		case GL_UNSIGNED_INT:
			return UniformType::UInt;
		case GL_UNSIGNED_INT_VEC2:
			return UniformType::UVec2;
		case GL_UNSIGNED_INT_VEC3:
			return UniformType::UVec3;
		case GL_UNSIGNED_INT_VEC4:
			return UniformType::UVec4;
		case GL_BOOL:
			return UniformType::Bool;
		case GL_BOOL_VEC2:
			return UniformType::BVec2;
		case GL_BOOL_VEC3:
			return UniformType::BVec3;
		case GL_BOOL_VEC4:
			return UniformType::BVec4;
		case GL_FLOAT_MAT2:
			return UniformType::Mat2;
		case GL_FLOAT_MAT3:
			return UniformType::Mat3;
		case GL_FLOAT_MAT4:
			return UniformType::Mat4;
		default:
			TT::assert(false);
			return UniformType::Unknown;
		}
	}

	void glFormatInfo(TTRendering::ImageFormat format, GLenum& internalFormat, GLenum& channels, GLenum& elementType) {
		using namespace TTRendering;
		switch (format) {
		case ImageFormat::Depth32F:
			internalFormat = GL_DEPTH_COMPONENT32F;
			channels = GL_DEPTH_COMPONENT;
			elementType = GL_FLOAT;
			return;
        case ImageFormat::RGBA32F:
            internalFormat = GL_RGBA32F;
            channels = GL_RGBA;
            elementType = GL_FLOAT;
            return;
		case ImageFormat::R8:
			internalFormat = GL_R8;
			channels = GL_RED;
			elementType = GL_UNSIGNED_BYTE;
			return;
		case ImageFormat::RG8:
			internalFormat = GL_RG8;
			channels = GL_RG;
			elementType = GL_UNSIGNED_BYTE;
			return;
		case ImageFormat::RGB8:
			internalFormat = GL_RGB8;
			channels = GL_RGB;
			elementType = GL_UNSIGNED_BYTE;
			return;
		case ImageFormat::RGBA8:
			internalFormat = GL_RGBA8;
			channels = GL_RGBA;
			elementType = GL_UNSIGNED_BYTE;
			return;
		default:
			// unhandled format
			TT::assert(false);
			return;;
		}
	}

	GLenum glPrimitiveType(TTRendering::PrimitiveType primitiveType) {
		using namespace TTRendering;
		switch (primitiveType) {
		case PrimitiveType::Point:
			return GL_POINTS;
		case PrimitiveType::Line:
			return GL_LINES;
		case PrimitiveType::Triangle:
			return GL_TRIANGLES;
		case PrimitiveType::TriangleFan:
			return GL_TRIANGLE_FAN;
		case PrimitiveType::TriangleStrip:
			return GL_TRIANGLE_STRIP;
		}
		TT::assert(false);
		return GL_TRIANGLES;
	}

	GLenum glIndexType(TTRendering::IndexType indexType) {
		using namespace TTRendering;
		switch (indexType) {
		case IndexType::U8:
			return GL_UNSIGNED_BYTE;
		case IndexType::U16:
			return GL_UNSIGNED_SHORT;
		case IndexType::U32:
			return GL_UNSIGNED_INT;
		}
		TT::assert(false);
		return GL_UNSIGNED_INT;
	}

	void vertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) {
		if (type == GL_DOUBLE) {
			glVertexAttribLPointer(index, size, type, stride, pointer);
		} else if (type == GL_BYTE
			|| type == GL_UNSIGNED_BYTE
			|| type == GL_SHORT
			|| type == GL_UNSIGNED_SHORT
			|| type == GL_INT
			|| type == GL_UNSIGNED_INT) {
			glVertexAttribIPointer(index, size, type, stride, pointer);
		} else {
			glVertexAttribPointer(index, size, type, normalized, stride, pointer);
		}
	}
}

namespace TTRendering {
	static_assert(sizeof(GLint) == sizeof(int));
	static_assert(sizeof(GLuint) == sizeof(unsigned int));

	namespace {
		UniformInfo::Field getUniformInfoAt(GLint program, GLuint index) {
			char name[256];
			memset(name, 0, sizeof(name));
			GLint size;
			GLenum type;
			glGetActiveUniform(program, index, sizeof(name) - 1, nullptr, &size, &type, name);
			TT::assert(size >= 0, "Negative array size.");
			return { name, _mapType(type), 0, (unsigned int)size };
		}

		GLuint passUbo;
		size_t passUboSize = 0;
		GLuint materialUbo;
		size_t materialUboSize = 0;
		GLuint pushConstantsUbo;
	}

	std::unordered_map<int, UniformInfo> OpenGLContext::getUniformBlocks(const ShaderHandle& shader, const std::vector<ShaderStageHandle>& stages) const {
		static std::hash<std::string> stringHasher;
		std::unordered_map<int, UniformInfo> result;

		GLuint program = (GLuint)shader.identifier();
		GLint numUniformBlocks = 0;
		glGetProgramInterfaceiv(program, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &numUniformBlocks);
		static const GLenum blockProperties[3] = { GL_NAME_LENGTH, GL_NUM_ACTIVE_VARIABLES, GL_BUFFER_DATA_SIZE };
		GLint blockData[3];
		for (GLint blockIx = 0; blockIx < numUniformBlocks; ++blockIx) {
			UniformInfo blockInfo;

			glGetProgramResourceiv(program, GL_UNIFORM_BLOCK, blockIx, 3, blockProperties, 3, nullptr, blockData);
			//Retrieve block name
			//char* blockName = new char[blockData[0]];
			//glGetProgramResourceName(program, GL_UNIFORM_BLOCK, blockIx, blockName.size() + 1, nullptr, blockName.data());
			//delete[] blockName;

			GLint binding;
			glGetActiveUniformBlockiv(program, blockIx, GL_UNIFORM_BLOCK_BINDING, &binding);

			//Retrieve indices of uniforms that are a member of this block.
			std::vector<GLint> uniformIxs(blockData[1]);
			GLenum member = GL_ACTIVE_VARIABLES;
			glGetProgramResourceiv(program, GL_UNIFORM_BLOCK, blockIx, 1, &member, (GLsizei)uniformIxs.size(), nullptr, uniformIxs.data());

			GLenum offs = GL_OFFSET;
			std::vector<GLint> uniformOffsets(blockData[1]);
			glGetProgramResourceiv(program, GL_UNIFORM, blockIx, 1, &offs, (GLsizei)uniformOffsets.size(), nullptr, uniformOffsets.data());

			int j = 0;
			for (GLint i : uniformIxs) {
				UniformInfo::Field field = getUniformInfoAt(program, i);
				blockInfo.nameHashToFieldIndex[stringHasher(field.name)] = blockInfo.fields.size();
				blockInfo.fields.push_back(field);

				GLint uniformOffset;
				glGetProgramResourceiv(program, GL_UNIFORM, i, 1, &offs, 1, nullptr, &uniformOffset);

				blockInfo.fields.back().offset = uniformOffset; // uniformOffsets[j];
				j++;
			}

			//We already retrieved the size.
			blockInfo.bufferSize = blockData[2];

			result[binding] = blockInfo;
		}

		return result;
	}

	OpenGLContext::OpenGLContext(const TT::Window& window) {
		_windowsGLContext = TTRendering::createGLContext(window);
        TTRendering::loadGLFunctions();
        glEnable(GL_DEPTH_TEST); TT_GL_DBG_ERR;
		glGenBuffers(1, &passUbo);
		glGenBuffers(1, &materialUbo);
		glGenBuffers(1, &pushConstantsUbo);
		glBindBuffer(GL_UNIFORM_BUFFER, pushConstantsUbo);
		// glBufferData(GL_UNIFORM_BUFFER, sizeof(PushConstants), nullptr, GL_DYNAMIC_DRAW);
		glBindBufferRange(GL_UNIFORM_BUFFER, (GLuint)UniformBlockSemantics::PushConstants, pushConstantsUbo, 0, sizeof(PushConstants));
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	void OpenGLContext::windowResized(unsigned int width, unsigned int height) {
		screenWidth = width;
		screenHeight = height;
	}

	void OpenGLContext::beginFrame() {
	}

	void OpenGLContext::endFrame() {
		SwapBuffers(_windowsGLContext);
	}
    
    BufferHandle OpenGLContext::createBuffer(size_t size, unsigned char* data, BufferMode mode) {
        if (size == 0)
            return BufferHandle(0, size);
        GLuint glHandle;
        glGenBuffers(1, &glHandle);
        glBindBuffer(GL_ARRAY_BUFFER, glHandle);
        glBufferData(GL_ARRAY_BUFFER, size, data, mode == BufferMode::StaticDraw ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        return BufferHandle(glHandle, size);
    }


    MeshHandle OpenGLContext::createMesh(
        size_t numElements, // num vertices if indexData == nullptr, else num indices
        BufferHandle vertexData, 
        const std::vector<MeshAttribute>& attributeLayout,
        BufferHandle* indexData,
        PrimitiveType primitiveType,
        size_t numInstances, // mesh is not instanced if numInstances == 0
        BufferHandle* instanceData, // ignored if numInstances == 0
        const std::vector<MeshAttribute>& instanceAttributeLayout) { // ignored if numInstances == 0 or instanceData == nullptr
		GLuint glHandle;
		glGenVertexArrays(1, &glHandle);

		glBindVertexArray(glHandle);

		// indices
		if (indexData) {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (GLuint)indexData->identifier());
		}

		// vertices
        {
		    glBindBuffer(GL_ARRAY_BUFFER, (GLuint)vertexData.identifier());

		    GLuint stride = 0;
		    for (const auto& attribute : attributeLayout) stride += attribute.sizeInBytes();

		    size_t offset = 0;
		    for (const auto& attribute : attributeLayout) {
			    glEnableVertexAttribArray(attribute.location);
			    vertexAttribPointer(attribute.location, ((int)attribute.dimensions + 1), glElementType(attribute.elementType), false, stride, (const void*)offset);
			    offset += attribute.sizeInBytes();
		    }

		    glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

		// instancing
        if(numInstances && instanceData) {
            glBindBuffer(GL_ARRAY_BUFFER, (GLuint)instanceData->identifier());

            GLuint stride = 0;
            for (const auto& attribute : instanceAttributeLayout) stride += attribute.sizeInBytes();

            size_t offset = 0;
		    for (const MeshAttribute& attribute : instanceAttributeLayout) {
				glEnableVertexAttribArray(attribute.location);
				vertexAttribPointer(attribute.location, ((int)attribute.dimensions + 1), glElementType(attribute.elementType), false, stride, (const void*)offset);
				glVertexAttribDivisor(attribute.location, 1);
				offset += attribute.sizeInBytes();
		    }

            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

		glBindVertexArray(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        size_t attributeLayoutHash = TT::hashCombine(hashMeshLayout(attributeLayout), hashMeshLayout(instanceAttributeLayout));
		return registerMesh(MeshHandle(glHandle, attributeLayoutHash, vertexData, numElements, primitiveType, IndexType::U32, indexData, numInstances, instanceData));
	}

	ShaderStageHandle OpenGLContext::createShaderStage(const char* glslFilePath) {
		std::unordered_set<std::string> dependencies;
		std::string shaderCode = TT::readWithIncludes(glslFilePath);
		std::vector<std::string> parts = TT::split(glslFilePath, ".");
		std::string identifier = parts[parts.size() - 2];
		GLenum mode;
		ShaderStageHandle::ShaderStage stage;
		if (identifier == "vert") {
			mode = GL_VERTEX_SHADER;
			stage = ShaderStageHandle::Vert;
		} else if (identifier == "frag") {
			mode = GL_FRAGMENT_SHADER;
			stage = ShaderStageHandle::Frag;
		} else if (identifier == "geom") {
			mode = GL_GEOMETRY_SHADER;
			stage = ShaderStageHandle::Geom;
		} else {
			mode = GL_COMPUTE_SHADER;
			stage = ShaderStageHandle::Compute;
		}

		return ShaderStageHandle(compileShader(shaderCode, mode), stage);
	}

	ShaderHandle OpenGLContext::createShader(const std::vector<ShaderStageHandle>& stages) {
		GLuint glHandle = glCreateProgram();
		for (const ShaderStageHandle& stage : stages) {
			glAttachShader(glHandle, (GLuint)stage.identifier());
		}
		glLinkProgram(glHandle);
		if (_getProgrami(glHandle, GL_LINK_STATUS) == GL_FALSE) {
			auto b = _getProgramInfoLog(glHandle);
			TT::error("%s\n", b.data());
		}
		glValidateProgram(glHandle);
		if (_getProgrami(glHandle, GL_VALIDATE_STATUS) == GL_FALSE) {
			auto b = _getProgramInfoLog(glHandle);
			TT::error("%s\n", b.data());
		}
		return ShaderHandle(glHandle);
	}

	ImageHandle OpenGLContext::createImage(unsigned int width, unsigned int height, ImageFormat format, ImageInterpolation interpolation, ImageTiling tiling, const unsigned char* data) {
		GLuint glHandle;
		glGenTextures(1, &glHandle); TT_GL_DBG_ERR;
		glBindTexture(GL_TEXTURE_2D, glHandle); TT_GL_DBG_ERR;
		GLenum repeatMode = (tiling == ImageTiling::Clamp) ? GL_CLAMP_TO_EDGE : GL_REPEAT;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeatMode); TT_GL_DBG_ERR;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeatMode); TT_GL_DBG_ERR;
		GLenum interpMode = (interpolation == ImageInterpolation::Linear) ? GL_LINEAR : GL_NEAREST;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpMode); TT_GL_DBG_ERR;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpMode); TT_GL_DBG_ERR;
		GLenum internalFormat, channels, elementType;
		glFormatInfo(format, internalFormat, channels, elementType); TT_GL_DBG_ERR;
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, channels, elementType, data); TT_GL_DBG_ERR;
		glBindTexture(GL_TEXTURE_2D, 0); TT_GL_DBG_ERR;
		return ImageHandle(glHandle, format, interpolation, tiling);
	}

    void OpenGLContext::imageSize(const ImageHandle& image, unsigned int& width, unsigned int& height) const {
        glBindTexture(GL_TEXTURE_2D, (GLuint)image.identifier()); TT_GL_DBG_ERR;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, (int*)&width); TT_GL_DBG_ERR;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, (int*)&height); TT_GL_DBG_ERR;
        glBindTexture(GL_TEXTURE_2D, 0); TT_GL_DBG_ERR;
    }

    void OpenGLContext::resizeImage(const ImageHandle& image, unsigned int width, unsigned int height) {
        GLuint glHandle = (GLuint)image.identifier();
        glBindTexture(GL_TEXTURE_2D, glHandle); TT_GL_DBG_ERR;
        GLenum internalFormat, channels, elementType;
        glFormatInfo(image.format(), internalFormat, channels, elementType); TT_GL_DBG_ERR;
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, channels, elementType, nullptr); TT_GL_DBG_ERR;
        glBindTexture(GL_TEXTURE_2D, 0); TT_GL_DBG_ERR;
    }

    void OpenGLContext::resizeFramebuffer(const FramebufferHandle& framebuffer, unsigned int width, unsigned int height) {
        for(auto& image : framebuffer._colorAttachments)
            resizeImage(image, width, height);
        if(framebuffer._depthStencilAttachment)
            resizeImage(*framebuffer._depthStencilAttachment, width, height);
    }

	FramebufferHandle OpenGLContext::createFramebuffer(const std::vector<ImageHandle>& colorAttachments, ImageHandle* depthStencilAttachment) {
		// Verify we have more than 0 attachments
		TT::assert(colorAttachments.size() > 0 || depthStencilAttachment != nullptr);

		// Fetch the resolution of the first available attachment, we'll ensure all buffers have the same size
		unsigned int width, height;
        if(depthStencilAttachment)
            imageSize(*depthStencilAttachment, width, height);
        else
            imageSize(colorAttachments[0], width, height);

		// Resolution check
		for (const auto& colorAttachment : colorAttachments) {
            unsigned int w, h;
            imageSize(colorAttachment, w, h);
			TT::assert(width == w && height == h);
		}

		// Verify the depth attachment is of a valid format
		GLenum depthStencilMode;
		if (depthStencilAttachment) {
			switch (depthStencilAttachment->format()) {
			case ImageFormat::Depth32F:
				depthStencilMode = GL_DEPTH_ATTACHMENT;
				break;
			default:
				// non depth-stencil formats are not supported
				TT::assertFatal(false);
				depthStencilAttachment = nullptr;
				break;
			}
		}

		GLuint glHandle;
		glGenFramebuffers(1, &glHandle);
		glBindFramebuffer(GL_FRAMEBUFFER, glHandle);

		unsigned int i = 0;
		for (const auto& colorAttachment : colorAttachments) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i++, GL_TEXTURE_2D, (GLuint)colorAttachment.identifier(), 0);
		}
		if (depthStencilAttachment) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, depthStencilMode, GL_TEXTURE_2D, (GLuint)depthStencilAttachment->identifier(), 0);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return FramebufferHandle(glHandle, colorAttachments, depthStencilAttachment);
	}

    const UniformInfo* OpenGLContext::materialUniformInfo(size_t shaderIdentifier) const {
        // Get material uniform block info for this shader
        TT::assert(shaderUniformInfo.find(shaderIdentifier) != shaderUniformInfo.end());
        const auto& uniformBlocks = shaderUniformInfo.find(shaderIdentifier)->second;
        auto it = uniformBlocks.find((int)UniformBlockSemantics::Material);
        const UniformInfo* uniformInfo = nullptr;
        if (it != uniformBlocks.end())
            uniformInfo = &it->second;
        return uniformInfo;
    }

    void OpenGLContext::bindAndAllocateMaterialUBO(const UniformInfo* uniformInfo) const {
        if (uniformInfo) {
            glBindBuffer(GL_UNIFORM_BUFFER, materialUbo);
            size_t requiredBufferSize = uniformInfo->bufferSize;
            if (materialUboSize < requiredBufferSize) {
                glBufferData(GL_UNIFORM_BUFFER, requiredBufferSize, nullptr, GL_DYNAMIC_DRAW);
                materialUboSize = requiredBufferSize;
            }
        }
    }

    const UniformInfo* OpenGLContext::useAndPrepareShader(size_t shaderIdentifier) const {
        // Enable shader
        glUseProgram((GLuint)shaderIdentifier);
        // Get material uniform block info for this shader
        const UniformInfo* uniformInfo = materialUniformInfo(shaderIdentifier);
        // Allocate enough space
        bindAndAllocateMaterialUBO(uniformInfo);
        return uniformInfo;
    }
    
    void OpenGLContext::applyMaterialBlendMode(const MaterialHandle& material) const {
        auto blendMode = material.blendMode();
        switch (blendMode) {
        case TTRendering::MaterialBlendMode::Opaque:
        case TTRendering::MaterialBlendMode::AlphaTest:
            glDisable(GL_BLEND); TT_GL_DBG_ERR;
            glDepthMask(true); TT_GL_DBG_ERR;
            // glBlendFunc(GL_ONE, GL_ZERO);
            break;
        case TTRendering::MaterialBlendMode::Alpha:
            glEnable(GL_BLEND); TT_GL_DBG_ERR;
            glDepthMask(false); TT_GL_DBG_ERR;
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); TT_GL_DBG_ERR;
            break;
        case TTRendering::MaterialBlendMode::PremultipliedAlpha:
            glEnable(GL_BLEND); TT_GL_DBG_ERR;
            glDepthMask(false); TT_GL_DBG_ERR;
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); TT_GL_DBG_ERR;
            break;
        case TTRendering::MaterialBlendMode::Additive:
            glEnable(GL_BLEND); TT_GL_DBG_ERR;
            glDepthMask(false); TT_GL_DBG_ERR;
            glBlendFunc(GL_ONE, GL_ONE); TT_GL_DBG_ERR;
            break;
        default:
            TT::assertFatal(false);
        }
    }

    void OpenGLContext::uploadMaterial(const UniformInfo* uniformInfo, const MaterialHandle& material) const {
        if (uniformInfo) {
            // Copy the data into the buffer
            glBindBuffer(GL_UNIFORM_BUFFER, materialUbo);
            TT::assert(material._uniformBuffer != nullptr);
            TT::assert(uniformInfo->bufferSize <= materialUboSize);
            glBufferSubData(GL_UNIFORM_BUFFER, 0, uniformInfo->bufferSize, material._uniformBuffer);
            // Map only the used portion of the buffer
            glBindBufferRange(GL_UNIFORM_BUFFER, (GLuint)UniformBlockSemantics::Material, materialUbo, 0, uniformInfo->bufferSize);
        }
    }

    void OpenGLContext::bindMaterialImages(const MaterialHandle& material, size_t shaderIdentifier) const {
        unsigned int activeImage = 0;
        for(const auto& pair : material._images) {
            glActiveTexture(GL_TEXTURE0 + activeImage);
            glBindTexture(GL_TEXTURE_2D, (GLuint)material._images.handle(pair.second).identifier()); TT_GL_DBG_ERR;
            GLint loc = glGetUniformLocation((GLuint)shaderIdentifier, pair.first.data());
            glUniform1i(loc, activeImage);
            ++activeImage;
        }
    }

    void OpenGLContext::bindMaterialSSBOs(const MaterialHandle& material, size_t shaderIdentifier) const {
        for(const auto& pair : material._ssbos) {
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, (GLuint)pair.first, (GLuint)material._ssbos.handle(pair.second).identifier());
        }
    }

    void OpenGLContext::bindMaterialResources(const UniformInfo* uniformInfo, const MaterialHandle& material, size_t shaderIdentifier) const {
        applyMaterialBlendMode(material);
        uploadMaterial(uniformInfo, material);
        bindMaterialImages(material, shaderIdentifier);
        bindMaterialSSBOs(material, shaderIdentifier);
    }

    void OpenGLContext::framebufferSize(const FramebufferHandle& framebuffer, unsigned int& width, unsigned int& height) const {
        if(framebuffer._depthStencilAttachment)
            return imageSize(*framebuffer._depthStencilAttachment, width, height);
        TT::assert(framebuffer._colorAttachments.size() > 0);
        return imageSize(framebuffer._colorAttachments[0], width, height);
    }

#if 0
    void OpenGLContext::bindFramebuffer(const TTRendering::FramebufferHandle* framebuffer) {
        if (!framebuffer) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, screenWidth, screenHeight); TT_GL_DBG_ERR;
        } else {
            glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)framebuffer->identifier());
            unsigned int width, height;
            framebufferSize(*framebuffer, width, height);
            glViewport(0, 0, width, height);
        }
    }
#endif

	void OpenGLContext::drawPass(RenderPass& pass) {
        // bindFramebuffer((const TTRendering::FramebufferHandle*)pass.framebuffer);
        if (!pass.framebuffer) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, screenWidth, screenHeight); TT_GL_DBG_ERR;
        } else {
            glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)pass.framebuffer->identifier());
            unsigned int width, height;
            framebufferSize(*pass.framebuffer, width, height);
            glViewport(0, 0, width, height); TT_GL_DBG_ERR;
        }

		GLenum clearFlags = 0;

		clearFlags |= GL_COLOR_BUFFER_BIT;
		glClearColor(pass.clearColor.x, pass.clearColor.y, pass.clearColor.z, pass.clearColor.w); TT_GL_DBG_ERR;

		clearFlags |= GL_DEPTH_BUFFER_BIT;
		glClearDepth(pass.clearDepthValue); TT_GL_DBG_ERR;

		glClear(clearFlags); TT_GL_DBG_ERR;

		if (!pass.passUniforms.isEmpty()) {
			glBindBuffer(GL_UNIFORM_BUFFER, passUbo);
			size_t requiredBufferSize = pass.passUniforms.value()->size();
			if (passUboSize < requiredBufferSize) {
				glBufferData(GL_UNIFORM_BUFFER, requiredBufferSize, nullptr, GL_DYNAMIC_DRAW);
				passUboSize = requiredBufferSize;
			}
			glBufferSubData(GL_UNIFORM_BUFFER, 0, requiredBufferSize, pass.passUniforms.value()->cpuBuffer());
			glBindBufferRange(GL_UNIFORM_BUFFER, (GLuint)UniformBlockSemantics::Pass, passUbo, 0, requiredBufferSize);
		}

		for (size_t meshLayoutIndex = 0; meshLayoutIndex < pass.drawQueue.keys.size(); ++meshLayoutIndex) {
			size_t meshLayoutHash = pass.drawQueue.keys[meshLayoutIndex];
			const auto& shaderQueue = pass.drawQueue.queues[meshLayoutIndex];
			for (size_t shaderIndex = 0; shaderIndex < shaderQueue.keys.size(); ++shaderIndex) {
				size_t shaderIdentifier = shaderQueue.keys[shaderIndex];

                const UniformInfo* uniformInfo = useAndPrepareShader((GLuint)shaderIdentifier);

				const auto& materialQueue = shaderQueue.queues[shaderIndex];
				for (size_t materialIndex = 0; materialIndex < materialQueue.keys.size(); ++materialIndex) {
					const MaterialHandle& material = materialQueue.keys[shaderIndex];

                    bindMaterialResources(uniformInfo, material, shaderIdentifier);

                    glBindBuffer(GL_UNIFORM_BUFFER, pushConstantsUbo);

					const auto& meshQueue = materialQueue.queues[shaderIndex];
					for (const auto& pair : meshQueue) {
                        const auto& [meshIdentifier, instanceCount, pushConstants] = pair.second;
						const MeshHandle* meshH = meshes.find(meshIdentifier);
						TT::assertFatal(meshH != nullptr);
						const MeshHandle& mesh = *meshH;
						glBindVertexArray((GLuint)mesh.identifier());

						glBufferData(GL_UNIFORM_BUFFER, sizeof(PushConstants), pushConstants, GL_DYNAMIC_DRAW);

						if (mesh.indexBuffer() != nullptr) {
                            if(instanceCount > 0) {
                                glDrawElementsInstanced(glPrimitiveType(mesh.primitiveType()), (GLsizei)mesh.numElements(), glIndexType(mesh.indexType()), nullptr, (GLsizei)instanceCount);
                            } else {
							    glDrawElements(glPrimitiveType(mesh.primitiveType()), (GLsizei)mesh.numElements(), glIndexType(mesh.indexType()), nullptr);
                            }
							TT_GL_DBG_ERR;
						}
						else {
                            if(instanceCount > 0) {
							    glDrawArraysInstanced(glPrimitiveType(mesh.primitiveType()), 0, (GLsizei)mesh.numElements(), (GLsizei)instanceCount);
                            } else {
							    glDrawArrays(glPrimitiveType(mesh.primitiveType()), 0, (GLsizei)mesh.numElements());
                            }
							TT_GL_DBG_ERR;
						}
					}
				}
			}
		}

		glBindVertexArray(0);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		glUseProgram(0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_BLEND);
        glDepthMask(true);
	}

    void OpenGLContext::dispatchCompute(const MaterialHandle& material, unsigned int x, unsigned int y, unsigned int z) {
        size_t shaderIdentifier = material.shader().identifier();
        const UniformInfo* uniformInfo = useAndPrepareShader((GLuint)shaderIdentifier);
        bindMaterialResources(uniformInfo, material, shaderIdentifier);
        glDispatchCompute(x, y, z);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
        // Remnant to verify written data:
        // TT::Vec3* points = (TT::Vec3*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    }
}
