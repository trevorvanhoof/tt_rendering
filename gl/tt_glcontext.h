#include "../tt_rendering.h"

#include "../../tt_cpplib/tt_window.h"

namespace TTRendering {
	// TODO: This context does not clean up after itself
	class OpenGLContext final : public RenderingContext {
		HDC__* _windowsGLContext = nullptr;
		std::unordered_map<int, UniformInfo> getUniformBlocks(const ShaderHandle& shader, const std::vector<ShaderStageHandle>& stages) const override;

        void bindAndAllocateMaterialUBO(const UniformInfo* uniformInfo) const;
        void applyMaterialBlendMode(const MaterialHandle& material) const;
        void uploadMaterial(const UniformInfo* uniformInfo, const MaterialHandle& material) const;
        void bindMaterialImages(const MaterialHandle& material, size_t shaderIdentifier) const;
        void bindMaterialSSBOs(const MaterialHandle& material) const;

        const UniformInfo* useAndPrepareShader(const ShaderHandle& handle) const;
        void bindMaterialResources(const UniformInfo* uniformInfo, const MaterialHandle& material, size_t shaderIdentifier) const;

		ShaderStageHandle createShaderStage(const char* glslFilePath) override;
		ShaderHandle createShader(const std::vector<ShaderStageHandle>& stages) override;

	public:
        // This is useful when running in another framework, but it means beginFrame and endFrame do not work.
		OpenGLContext();

		OpenGLContext(const TT::Window& window);

		void beginFrame() override;
		void endFrame() override;

        BufferHandle createBuffer(size_t size, unsigned char* data = nullptr, BufferMode mode = BufferMode::StaticDraw, const ResourcePoolHandle* pool = nullptr) override;
        MeshHandle createMesh(
            size_t numElements, // num vertices if indexData == nullptr, else num indices
            BufferHandle vertexData, 
            const std::vector<MeshAttribute>& attributeLayout,
            BufferHandle* indexData = nullptr,
            PrimitiveType primitiveType = PrimitiveType::Triangle,
            size_t numInstances = 0, // mesh is not instanced if numInstances == 0
            BufferHandle* instanceData = nullptr, // ignored if numInstances == 0
            const std::vector<MeshAttribute>& instanceAttributeLayout = {}, 
            const ResourcePoolHandle* pool = nullptr) override; // ignored if numInstances == 0 or instanceData == nullptr
		ImageHandle createImage(unsigned int width, unsigned int height, ImageFormat format, ImageInterpolation interpolation = ImageInterpolation::Linear, ImageTiling tiling = ImageTiling::Repeat, const unsigned char* data = nullptr, const ResourcePoolHandle* pool = nullptr) override;
        void imageSize(const ImageHandle& image, unsigned int& width, unsigned int& height) const override;
        void framebufferSize(const FramebufferHandle& framebuffer, unsigned int& width, unsigned int& height) const override;
		void resizeImage(const ImageHandle& image, unsigned int width, unsigned int height) override;
		void resizeFramebuffer(const FramebufferHandle& framebuffer, unsigned int width, unsigned int height) override;
		FramebufferHandle createFramebuffer(const std::vector<ImageHandle>& colorAttachments, const ImageHandle* depthStencilAttachment, const ResourcePoolHandle* pool = nullptr) override;
		void drawPass(const RenderPass& pass, unsigned int defaultFramebuffer = 0) override;
        void dispatchCompute(const MaterialHandle& material, unsigned int x = 1, unsigned int y = 1, unsigned int z = 1) override;
        void deleteBuffer(const BufferHandle& buffer) override;
        void deleteMesh(const MeshHandle& mesh) override;
        void deleteShaderStage(const ShaderStageHandle& mesh) override;
        void deleteShader(const ShaderHandle& mesh) override;
        void deleteImage(const ImageHandle& mesh) override;
        void deleteFramebuffer(const FramebufferHandle& material) override;
	};
}
