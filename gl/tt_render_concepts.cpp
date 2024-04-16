#include "tt_render_concepts.h"

namespace TT {
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

	GLenum MeshAttrib::glElementType() { return (int)elementType & 0xFFFF; }
	int MeshAttrib::elementSizeInBytes() { return (int)elementType >> 16; }

	void Mesh::alloc(size_t vboSize, void* vboData, GLsizei indexCount, IndexType iboElementType, void* iboData) {
		vbo.alloc(vboSize, GL_ARRAY_BUFFER, vboData);
		this->indexCount = indexCount;
		indexType = iboElementType;
		if (indexCount)
			ibo.alloc(indexCount * ((size_t)iboElementType >> 16), GL_ELEMENT_ARRAY_BUFFER, iboData);

		vao.alloc();
		glBindVertexArray(vao.handle);
		if (indexCount) {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo.handle);
		}

		GLsizei stride = 0;
		for (auto attrib : attribs)
			stride += attrib.elementSizeInBytes() * (GLsizei)attrib.dimensions;
		vertexCount = (GLsizei)(vboSize / stride);

		glBindBuffer(GL_ARRAY_BUFFER, vbo.handle);
		size_t offset = 0;
		for (auto attrib : attribs) {
			glEnableVertexAttribArray((int)attrib.semantic);
			vertexAttribPointer((int)attrib.semantic, (int)attrib.dimensions, attrib.glElementType(), false, stride, (const void*)offset);
			offset += attrib.elementSizeInBytes() * (GLsizei)attrib.dimensions;
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		for (auto instanceBuffer : instanceAttribs) {
			glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer->handle);
			stride = 0;
			for (auto attrib : instanceBuffer->bindings)
				stride += attrib.elementSizeInBytes() * (GLsizei)attrib.dimensions;
			
			offset = 0;
			for (auto attrib : instanceBuffer->bindings) {
				glEnableVertexAttribArray((int)attrib.semantic);
				vertexAttribPointer((int)attrib.semantic, (int)attrib.dimensions, attrib.glElementType(), false, stride, (const void*)offset);
				glVertexAttribDivisor((int)attrib.semantic, 1);
				offset += attrib.elementSizeInBytes() * (GLsizei)attrib.dimensions;
			}
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		glBindVertexArray(0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void Mesh::cleanup() {
		vao.cleanup();
		vbo.cleanup();
		ibo.cleanup();
	}

	void Mesh::draw(GLenum primitive) const {
		glBindVertexArray(vao.handle);
		glDrawArrays(primitive, 0, vertexCount); TT_GL_DBG_ERR;
		glBindVertexArray(0);
	}

	void Mesh::drawIndexed(GLenum primitive) const {
		glBindVertexArray(vao.handle);
		glDrawElements(primitive, indexCount, (int)indexType & 0xFFFF, nullptr); TT_GL_DBG_ERR;
		glBindVertexArray(0);
	}

	void Mesh::drawInstanced(GLenum primitive, GLsizei instanceCount) const {
		glBindVertexArray(vao.handle);
		glDrawArraysInstanced(primitive, 0, vertexCount, instanceCount);
		glBindVertexArray(0);
	}

	void Mesh::drawIndexedInstanced(GLenum primitive, GLsizei instanceCount) const {
		glBindVertexArray(vao.handle);
		glDrawElementsInstanced(primitive, indexCount, (int)indexType & 0xFFFF, nullptr, instanceCount);
		glBindVertexArray(0);
	}

	void UniformValue::init(const void* data, Type type, int sizeOfElement, int dimensions, int count) {
		this->type = type;
		this->dimensions = dimensions;
		this->count = count;
		blob = new char[this->dimensions * this->count * sizeOfElement];
		memcpy(blob, data, this->dimensions * this->count * sizeOfElement);
	}

	UniformValue::UniformValue() {}
	UniformValue::UniformValue(const void* data, Type type, int sizeOfElement, int dimensions, int count) { init(data, type, sizeOfElement, dimensions, count); }
	UniformValue::UniformValue(const float* data, int dimensions, int count) : UniformValue((void*)data, Type::Float, sizeof(float), dimensions, count) {}
	UniformValue::UniformValue(const int* data, int dimensions, int count) : UniformValue((void*)data, Type::Int, sizeof(int), dimensions, count) {}
	UniformValue::UniformValue(const unsigned int* data, int dimensions, int count) : UniformValue((void*)data, Type::UInt, sizeof(unsigned int), dimensions, count) {}
	UniformValue::UniformValue(const Image& image) {
		type = Type::Image;
		dimensions = 1;
		count = 1;
		this->image = &image;
	}
	UniformValue::UniformValue(const float* data, Type matrixType, int count) {
		int dimensions;
		switch (matrixType) {
		case Type::Mat22:
			dimensions = 4;
			break;
		case Type::Mat23:
		case Type::Mat32:
			dimensions = 6;
			dimensions = 16;
			break;
		case Type::Mat33:
			dimensions = 9;
			break;
		case Type::Mat34:
		case Type::Mat43:
			dimensions = 12;
			break;
		case Type::Mat44:
			dimensions = 16;
			break;
		default:
			error("Invalid type when constructing matrix uniform.");
			return;
		}
		init(data, matrixType, sizeof(float), dimensions, count);
	}
	UniformValue::UniformValue(float x) : UniformValue(&x, 1) {}
	UniformValue::UniformValue(float x, float y) { float tmp[2] = { x, y }; init(tmp, Type::Float, sizeof(float), 2, 1); }
	UniformValue::UniformValue(float x, float y, float z) { float tmp[3] = { x, y, z }; init(tmp, Type::Float, sizeof(float), 3, 1); }
	UniformValue::UniformValue(float x, float y, float z, float w) { float tmp[4] = { x, y, z, w }; init(tmp, Type::Float, sizeof(float), 4, 1); }

	UniformValue::UniformValue(int x) : UniformValue(&x, 1) {}
	UniformValue::UniformValue(int x, int y) { int tmp[2] = { x, y }; init(tmp, Type::Int, sizeof(int), 2, 1); }
	UniformValue::UniformValue(int x, int y, int z) { int tmp[3] = { x, y, z }; init(tmp, Type::Int, sizeof(int), 3, 1); }
	UniformValue::UniformValue(int x, int y, int z, int w) { int tmp[4] = { x, y, z, w }; init(tmp, Type::Int, sizeof(int), 4, 1); }

	UniformValue::UniformValue(unsigned int x) : UniformValue(&x, 1) {}
	UniformValue::UniformValue(unsigned int x, unsigned int y) { unsigned int tmp[2] = { x, y }; init(tmp, Type::UInt, sizeof(unsigned int), 2, 1); }
	UniformValue::UniformValue(unsigned int x, unsigned int y, unsigned int z) { unsigned int tmp[3] = { x, y, z }; init(tmp, Type::UInt, sizeof(unsigned int), 3, 1); }
	UniformValue::UniformValue(unsigned int x, unsigned int y, unsigned int z, unsigned int w) { unsigned int tmp[4] = { x, y, z, w }; init(tmp, Type::UInt, sizeof(unsigned int), 4, 1); }

	UniformValue::UniformValue(Vec2 v) : UniformValue(v.m.m128_f32, 2) {}
	UniformValue::UniformValue(Vec3 v) : UniformValue(v.m.m128_f32, 3) {}
	UniformValue::UniformValue(Vec4 v) : UniformValue(v.m.m128_f32, 4) {}
	UniformValue::UniformValue(const Mat44& v) : UniformValue((const float*)v.m, UniformValue::Type::Mat44) {}

	void UniformValue::apply(GLuint location, int& imageCounter) const {
		switch (type) {
		case Type::Float:
			switch (dimensions) {
			case 1:
				glUniform1fv(location, count, f);
				break;
			case 2:
				glUniform2fv(location, count, f);
				break;
			case 3:
				glUniform3fv(location, count, f);
				break;
			case 4:
				glUniform4fv(location, count, f);
				break;
			default:
				error("Invalid dimensions for uniform of type float.");
				return;
			}
			break;
		case Type::Int:
			switch (dimensions) {
			case 1:
				glUniform1iv(location, count, i);
				break;
			case 2:
				glUniform2iv(location, count, i);
				break;
			case 3:
				glUniform3iv(location, count, i);
				break;
			case 4:
				glUniform4iv(location, count, i);
				break;
			default:
				error("Invalid dimensions for uniform of type int.");
				return;
			}
			break;
		case Type::UInt:
			switch (dimensions) {
			case 1:
				glUniform1uiv(location, count, u);
				break;
			case 2:
				glUniform2uiv(location, count, u);
				break;
			case 3:
				glUniform3uiv(location, count, u);
				break;
			case 4:
				glUniform4uiv(location, count, u);
				break;
			default:
				error("Invalid dimensions for uniform of type uint.");
				return;
			}
			break;
		case Type::Mat22:
			if (!assert(dimensions == 4)) return;
			glUniformMatrix2fv(location, count, false, f);
			break;
		case Type::Mat23:
			if (!assert(dimensions == 6)) return;
			glUniformMatrix2x3fv(location, count, false, f);
			break;
		case Type::Mat32:
			if (!assert(dimensions == 6)) return;
			glUniformMatrix3x2fv(location, count, false, f);
			break;
		case Type::Mat33:
			if (!assert(dimensions == 9)) return;
			glUniformMatrix3fv(location, count, false, f);
			break;
		case Type::Mat34:
			if (!assert(dimensions == 12)) return;
			glUniformMatrix3x4fv(location, count, false, f);
			break;
		case Type::Mat43:
			if (!assert(dimensions == 12)) return;
			glUniformMatrix4x3fv(location, count, false, f);
			break;
		case Type::Mat44:
			if (!assert(dimensions == 16)) return;
			glUniformMatrix4fv(location, count, false, f);
			break;
		case Type::Image:
			glActiveTexture(GL_TEXTURE0 + imageCounter);
			image->bind();
			glUniform1i(location, imageCounter);
			++imageCounter;
			break;
		default:
			error("invalid uniform type");
			break;
		}
	}

	void Material::use() {
		if (active == this) {
			imageCounter = imageCounterReset;
			return;
		}
		imageCounter = 0;
		active = this;
		program->use();
		for(auto pair : uniforms)
			pair.second.apply(program->uniform(pair.first.c_str()), imageCounter);
		imageCounterReset = imageCounter;
	}

	void Material::set(const char* n, const UniformValue& v) {
		uniforms[n] = v;
		if (active == this)
			v.apply(program->uniform(n), imageCounter);
	}

	void Material::set(const std::unordered_map<std::string, UniformValue>& uniforms) {
		for (const auto& pair : uniforms)
			set(pair.first.c_str(), pair.second);
	}

	Material* Material::active = nullptr;
}
