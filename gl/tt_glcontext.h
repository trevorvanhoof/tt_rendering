#include "../tt_rendering.h"

#include <tt_window.h>

namespace TTRendering {
	// TODO: This context does not clean up after itself
	class OpenGLContext final : public RenderingContext {
		HDC__* _windowsGLContext;

		std::unordered_map<int, UniformInfo> getUniformBlocks(const ShaderHandle& shader, const std::vector<ShaderStageHandle>& stages) const override;

	public:
		OpenGLContext(const TT::Window& window);
		void windowResized(unsigned int width, unsigned int height) override;
		void beginFrame() override;
		void endFrame() override;

        BufferHandle createBuffer(size_t size, unsigned char* data = nullptr, BufferMode mode = BufferMode::StaticDraw) override;
		MeshHandle createMesh(const std::vector<MeshAttribute>& attributeLayout, BufferHandle vbo, size_t numElements, BufferHandle* ibo = nullptr) override;
		ShaderStageHandle createShaderStage(const char* glslFilePath) override;
		ShaderHandle createShader(const std::vector<ShaderStageHandle>& stages) override;
		ImageHandle createImage(unsigned int width, unsigned int height, ImageFormat format) override;
        void resizeImage(ImageHandle& image, unsigned int width, unsigned int height) override;
		FramebufferHandle createFramebuffer(const std::vector<ImageHandle>& colorAttachments, ImageHandle* depthStencilAttachment) override;
		void drawPass(RenderPass& pass) override;
	};
}
