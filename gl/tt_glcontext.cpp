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
		glGenBuffers(1, &passUbo);
		glGenBuffers(1, &materialUbo);
		glGenBuffers(1, &pushConstantsUbo);
		glBindBuffer(GL_UNIFORM_BUFFER, pushConstantsUbo);
		// glBufferData(GL_UNIFORM_BUFFER, sizeof(PushConstants), nullptr, GL_DYNAMIC_DRAW);
		glBindBufferRange(GL_UNIFORM_BUFFER, (GLuint)UniformBlockSemantics::PushConstants, pushConstantsUbo, 0, sizeof(PushConstants));
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	void OpenGLContext::windowResized(unsigned int width, unsigned int height) {
		glViewport(0, 0, width, height); TT_GL_DBG_ERR;
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

	MeshHandle OpenGLContext::createMesh(const std::vector<MeshAttribute>& attributeLayout, BufferHandle vbo, size_t numElements, BufferHandle* ibo) {
		GLuint glHandle;
		glGenVertexArrays(1, &glHandle);

		glBindVertexArray(glHandle);

		// indices
		if (ibo) {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (GLuint)ibo->identifier());
		}

		// vertices
		glBindBuffer(GL_ARRAY_BUFFER, (GLuint)vbo.identifier());

		GLuint stride = 0;
		for (auto attribute : attributeLayout) stride += attribute.sizeInBytes();

		size_t offset = 0;
		for (auto attribute : attributeLayout) {
			glEnableVertexAttribArray(attribute.location);
			vertexAttribPointer(attribute.location, ((int)attribute.dimensions + 1), glElementType(attribute.elementType), false, stride, (const void*)offset);
			offset += attribute.sizeInBytes();
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		/*
		// instancing
		for (auto instanceBuffer : instanceAttribs) {
			glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer->handle);
			GLsizei stride = 0;
			for (auto attrib : instanceBuffer->bindings)
				stride += attrib.elementSizeInBytes() * ((GLsizei)attrib.dimensions + 1);

			offset = 0;
			for (auto attrib : instanceBuffer->bindings) {
				glEnableVertexAttribArray((int)attrib.semantic);
				vertexAttribPointer((int)attrib.semantic, ((int)attrib.dimensions + 1), attrib.glElementType(), false, stride, (const void*)offset);
				glVertexAttribDivisor((int)attrib.semantic, 1);
				offset += attrib.elementSizeInBytes() * ((GLsizei)attrib.dimensions + 1);
			}
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
		*/

		glBindVertexArray(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		return registerMesh(MeshHandle(glHandle, hashMeshLayout(attributeLayout), vbo, numElements, PrimitiveType::Triangle, IndexType::U32, ibo));
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

	ImageHandle OpenGLContext::createImage(unsigned int width, unsigned int height, ImageFormat format) {
		GLuint glHandle;
		glGenTextures(1, &glHandle); TT_GL_DBG_ERR;
		glBindTexture(GL_TEXTURE_2D, glHandle); TT_GL_DBG_ERR;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); TT_GL_DBG_ERR;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); TT_GL_DBG_ERR;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); TT_GL_DBG_ERR;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); TT_GL_DBG_ERR;
		GLenum internalFormat, channels, elementType;
		glFormatInfo(format, internalFormat, channels, elementType); TT_GL_DBG_ERR;
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, channels, elementType, nullptr); TT_GL_DBG_ERR;
		glBindTexture(GL_TEXTURE_2D, 0); TT_GL_DBG_ERR;
		return ImageHandle(glHandle, width, height, format);
	}
    
    void OpenGLContext::resizeImage(ImageHandle& image, unsigned int width, unsigned int height) {
        GLuint glHandle = (GLuint)image.identifier();
        glBindTexture(GL_TEXTURE_2D, glHandle); TT_GL_DBG_ERR;
        GLenum internalFormat, channels, elementType;
        glFormatInfo(image.format(), internalFormat, channels, elementType); TT_GL_DBG_ERR;
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, channels, elementType, nullptr); TT_GL_DBG_ERR;
        glBindTexture(GL_TEXTURE_2D, 0); TT_GL_DBG_ERR;
        image.resized(width, height);
    }

	FramebufferHandle OpenGLContext::createFramebuffer(const std::vector<ImageHandle>& colorAttachments, ImageHandle* depthStencilAttachment) {
		// Verify we have more than 0 attachments
		TT::assert(colorAttachments.size() > 0 || depthStencilAttachment != nullptr);

		// Fetch the resolution of the first available attachment, we'll ensure all buffers have the same size
		unsigned int width, height;
		if (colorAttachments.size() > 0) {
			width = colorAttachments[0].width();
			height = colorAttachments[0].height();
		} else {
			width = depthStencilAttachment->width();
			height = depthStencilAttachment->height();
		}

		// Resolution check
		for (const auto& colorAttachment : colorAttachments) {
			TT::assert(width == colorAttachment.width() && height == colorAttachment.height());
		}

		// Verify the depth attachment is of a valid format
		GLenum depthStencilMode;
		if (depthStencilAttachment) {
			// Resolution check
			TT::assert(width == depthStencilAttachment->width() && height == depthStencilAttachment->height());

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

	void OpenGLContext::drawPass(RenderPass& pass) {
		if (pass.framebuffer.isEmpty()) {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		} else {
			glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)pass.framebuffer.value()->identifier());
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

		// TODO: How do we know the layout for this?
		//       Should we generate the UniformInfo from source and
		//       then use a Material-esque system to assign named values instead?
		// pass.passUniforms;
		// std::unordered_map<const MeshLayout&, std::unordered_map<const Program&, std::unordered_map<const Material&, std::vector<const Mesh&>>>> drawQueue;
		for (size_t meshLayoutIndex = 0; meshLayoutIndex < pass.drawQueue.keys.size(); ++meshLayoutIndex) {
			size_t meshLayoutHash = pass.drawQueue.keys[meshLayoutIndex];
			const auto& shaderQueue = pass.drawQueue.queues[meshLayoutIndex];
			for (size_t shaderIndex = 0; shaderIndex < shaderQueue.keys.size(); ++shaderIndex) {
				size_t shaderIdentifier = shaderQueue.keys[shaderIndex];

				glUseProgram((GLuint)shaderIdentifier);

				// Get material uniform block info for this shader
				TT::assert(shaderUniformInfo.find(shaderIdentifier) != shaderUniformInfo.end());
				const auto& uniformBlocks = shaderUniformInfo.find(shaderIdentifier)->second;
				auto it = uniformBlocks.find((int)UniformBlockSemantics::Material);
				const UniformInfo* uniformInfo = nullptr;
				if (it != uniformBlocks.end())
					uniformInfo = &it->second;

				// Allocate enough space
				if (uniformInfo) {
					glBindBuffer(GL_UNIFORM_BUFFER, materialUbo);
					size_t requiredBufferSize = uniformInfo->bufferSize;
					if (materialUboSize < requiredBufferSize) {
						glBufferData(GL_UNIFORM_BUFFER, requiredBufferSize, nullptr, GL_DYNAMIC_DRAW);
						materialUboSize = requiredBufferSize;
					}
				}

				const auto& materialQueue = shaderQueue.queues[shaderIndex];
				for (size_t materialIndex = 0; materialIndex < materialQueue.keys.size(); ++materialIndex) {
					const MaterialHandle& material = materialQueue.keys[shaderIndex];

					if (uniformInfo) {
						TT::assert(material._uniformBuffer != nullptr);
						// Copy the data into the buffer
						glBufferSubData(GL_UNIFORM_BUFFER, 0, uniformInfo->bufferSize, material._uniformBuffer);
						// Map only the used portion of the buffer
						glBindBufferRange(GL_UNIFORM_BUFFER, (GLuint)UniformBlockSemantics::Material, materialUbo, 0, uniformInfo->bufferSize);
					}

                    unsigned int activeImage = 0;
                    for(const auto& pair : material._images) {
                        glActiveTexture(GL_TEXTURE0 + activeImage);
                        glBindTexture(GL_TEXTURE_2D, (GLuint)material._images.handle(pair.second).identifier());
                        glUniform1i(glGetUniformLocation((GLuint)shaderIdentifier, pair.first.data()), activeImage);
                        ++activeImage;
                    }

					const auto& meshQueue = materialQueue.queues[shaderIndex];
					for (const auto& [meshIdentifier, pushConstants] : meshQueue) {
						const MeshHandle* meshH = meshes.find(meshIdentifier);
						TT::assert(meshH != nullptr);
						const MeshHandle& mesh = *meshH;
						glBindVertexArray((GLuint)mesh.identifier());

						glBindBuffer(GL_UNIFORM_BUFFER, pushConstantsUbo);
						glBufferData(GL_UNIFORM_BUFFER, sizeof(PushConstants), &pushConstants, GL_DYNAMIC_DRAW);

						if (mesh.indexBuffer() != nullptr) {
							glDrawElements(glPrimitiveType(mesh.primitiveType()), (GLsizei)mesh.numElements(), glIndexType(mesh.indexType()), nullptr);
							TT_GL_DBG_ERR;
						}
						else {
							glDrawArrays(glPrimitiveType(mesh.primitiveType()), 0, (GLsizei)mesh.numElements());
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
	}
}
