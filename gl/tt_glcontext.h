#include "../tt_rendering.h"

#include <tt_window.h>

namespace TTRendering {
	// TODO: This context does not clean up after itself
	class OpenGLContext final : public RenderingContext {
		HDC__* _windowsGLContext;
		unsigned int screenWidth = 32;
		unsigned int screenHeight = 32;
		std::unordered_map<int, UniformInfo> getUniformBlocks(const ShaderHandle& shader, const std::vector<ShaderStageHandle>& stages) const override;

        const UniformInfo* materialUniformInfo(size_t shaderIdentifier) const;
        void bindAndAllocateMaterialUBO(const UniformInfo* uniformInfo) const;
        void applyMaterialBlendMode(const MaterialHandle& material) const;
        void bindMaterialUBO(const UniformInfo* uniformInfo, const MaterialHandle& material) const;
        void bindMaterialImages(const MaterialHandle& material, size_t shaderIdentifier) const;
        void bindMaterialSSBOs(const MaterialHandle& material, size_t shaderIdentifier) const;

        const UniformInfo* useAndPrepareShader(size_t shaderIdentifier) const;
        void bindMaterialResources(const UniformInfo* uniformInfo, const MaterialHandle& material, size_t shaderIdentifier) const;

	public:
		OpenGLContext(const TT::Window& window);
		void windowResized(unsigned int width, unsigned int height) override;
		void beginFrame() override;
		void endFrame() override;

        BufferHandle createBuffer(size_t size, unsigned char* data = nullptr, BufferMode mode = BufferMode::StaticDraw) override;
        MeshHandle createMesh(
            size_t numElements, // num vertices if indexData == nullptr, else num indices
            BufferHandle vertexData, 
            const std::vector<MeshAttribute>& attributeLayout,
            BufferHandle* indexData = nullptr,
            PrimitiveType primitiveType = PrimitiveType::Triangle,
            size_t numInstances = 0, // mesh is not instanced if numInstances == 0
            BufferHandle* instanceData = nullptr, // ignored if numInstances == 0
            const std::vector<MeshAttribute>& instanceAttributeLayout = {}) override; // ignored if numInstances == 0 or instanceData == nullptr
		ShaderStageHandle createShaderStage(const char* glslFilePath) override;
		ShaderHandle createShader(const std::vector<ShaderStageHandle>& stages) override;
		ImageHandle createImage(unsigned int width, unsigned int height, ImageFormat format, ImageInterpolation interpolation = ImageInterpolation::Linear, ImageTiling tiling = ImageTiling::Repeat, const unsigned char* data = nullptr) override;
        void imageSize(const ImageHandle& image, unsigned int& width, unsigned int& height) const override;
		void resizeImage(const ImageHandle& image, unsigned int width, unsigned int height) override;
		void resizeFbo(const FramebufferHandle& image, unsigned int width, unsigned int height) override;
		FramebufferHandle createFramebuffer(const std::vector<ImageHandle>& colorAttachments, ImageHandle* depthStencilAttachment) override;
		void drawPass(RenderPass& pass) override;
        void dispatchCompute(const MaterialHandle& material, unsigned int x = 1, unsigned int y = 1, unsigned int z = 1) override;
	};
}
