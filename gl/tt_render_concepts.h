#pragma once

#include <vector>
#include "tt_gl_handles.h"
#include "tt_program_manager.h"
#include "tt_cgmath.h"

namespace TT {
	void vertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);

	struct MeshAttrib {
		enum class Semantic {
			POSITION = 0,
			NORMAL = 1,
			TANGENT = 2,
			BINORMAL = 4,
			COLOR = 4,
			UV = 5,
			STYLE_ID = 6,
		};

		enum class Dimensions {
			D1 = 1,
			D2 = 2,
			D3 = 3,
			D4 = 4,
		};

		// GLenum only uses up 2 bytes, so we can pack the element size in bytes in the next byte
		enum class ElementType {
			I8 = GL_BYTE | 0x010000,
			U8 = GL_UNSIGNED_BYTE | 0x010000,
			I16 = GL_SHORT | 0x020000,
			U16 = GL_UNSIGNED_SHORT | 0x020000,
			I32 = GL_INT | 0x040000,
			U32 = GL_UNSIGNED_INT | 0x040000,
			F16 = GL_HALF_FLOAT | 0x020000,
			F32 = GL_FLOAT | 0x040000,
			F64 = GL_DOUBLE | 0x080000,
			// = GL_FIXED, 
			I_2_10_10_REV = GL_INT_2_10_10_10_REV | 0x040000,
			U_2_10_10_REV = GL_UNSIGNED_INT_2_10_10_10_REV | 0x040000,
			U_10F_11F_11F_REV = GL_UNSIGNED_INT_10F_11F_11F_REV | 0x040000,
		};

		Semantic semantic = Semantic::POSITION;
		Dimensions dimensions = Dimensions::D3;
		ElementType elementType = ElementType::F32;

		GLenum glElementType();
		int elementSizeInBytes();
	};

	struct InstanceBuffer : public Buffer {
		std::vector<MeshAttrib> bindings;
	};

	struct Mesh {
		// GLenum only uses up 2 bytes, so we can pack the element size in bytes in the next byte
		enum class IndexType {
			U8 = GL_UNSIGNED_BYTE | 0x010000,
			U16 = GL_UNSIGNED_SHORT | 0x020000,
			U32 = GL_UNSIGNED_INT | 0x040000,
		};

		std::vector<MeshAttrib> attribs;
		std::vector<InstanceBuffer*> instanceAttribs;
		VAO vao;
		Buffer vbo;
		Buffer ibo;
		GLsizei vertexCount;
		GLsizei indexCount;
		IndexType indexType;

		void alloc(size_t vboSize, void* vboData = nullptr, GLsizei indexCount = 0, IndexType iboElementType = IndexType::U32, void* iboData = nullptr);
		void cleanup();
		void draw(GLenum primitive) const;
		void drawIndexed(GLenum primitive) const;
		void drawInstanced(GLenum primitive, GLsizei instanceCount) const;
		void drawIndexedInstanced(GLenum primitive, GLsizei instanceCount) const;
	};

	struct UniformValue {
		enum class Type {
			Invalid,
			Float,
			Int,
			UInt,
			Image,

			Mat22,
			Mat23,
			Mat32,
			Mat33,
			Mat34,
			Mat43,
			Mat44,
		};
		
		Type type = Type::Invalid;
		int dimensions = 0;

		union {
			float* f;
			int* i;
			unsigned int* u;
			const Image* image;
			char* blob;
		};
		int count = 0;

		void init(const void* data, Type type, int sizeOfElement, int dimensions, int count = 1);
		UniformValue();
		UniformValue(const void* data, Type type, int sizeOfElement, int dimensions, int count = 1);
		UniformValue(const float* data, int dimensions, int count = 1);
		UniformValue(const int* data, int dimensions, int count = 1);
		UniformValue(const unsigned int* data, int dimensions, int count = 1);
		UniformValue(const Image& image);
		UniformValue(const float* data, Type matrixType = Type::Mat44, int count = 1);
		UniformValue(float x);
		UniformValue(float x, float y);
		UniformValue(float x, float y, float z);
		UniformValue(float x, float y, float z, float w);
		UniformValue(int x);
		UniformValue(int x, int y);
		UniformValue(int x, int y, int z);
		UniformValue(int x, int y, int z, int w);
		UniformValue(unsigned int x);
		UniformValue(unsigned int x, unsigned int y);
		UniformValue(unsigned int x, unsigned int y, unsigned int z);
		UniformValue(unsigned int x, unsigned int y, unsigned int z, unsigned int w);
		UniformValue(Vec2 v);
		UniformValue(Vec3 v);
		UniformValue(Vec4 v);
		UniformValue(const Mat44& v);
		void apply(GLuint location, int& imageCounter) const;
	};

	struct Material {
	private:
		int imageCounter = 0;
		int imageCounterReset = 0;
		static Material* active;
		std::unordered_map<std::string, UniformValue> uniforms;

	public:
		Program* program;

		void use();
		void set(const char* n, const UniformValue& v);
		void set(const std::unordered_map<std::string, UniformValue>& uniforms);
	};
}
