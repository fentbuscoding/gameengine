#pragma once

#ifdef NEXUS_VULKAN_ENABLED

#include "../RHIDevice.h"
#include <vulkan/vulkan.h>
#include <cstdint>
#include <string>
#include <vector>

namespace Nexus {
namespace RHI {

/// Maps an engine texture format to its Vulkan equivalent.
///
/// This was defined `static` in VulkanDevice.cpp, giving it internal linkage,
/// while VulkanPipeline.cpp called it - so the Vulkan backend did not compile
/// as soon as both files were built together. Declaring it here gives the
/// backend a single shared definition.
VkFormat ToVulkanFormat(TextureFormat format);

class VulkanDevice : public RHIDevice {
public:
    VulkanDevice();
    ~VulkanDevice() override;

    bool Initialize(const SwapChainDesc& swapChainDesc) override;
    void Shutdown() override;

    GraphicsAPI GetAPI() const override { return GraphicsAPI::Vulkan; }
    const char* GetAPIName() const override { return "Vulkan"; }

    void BeginFrame() override;
    void EndFrame() override;
    void Present() override;

    RHICommandBufferPtr CreateCommandBuffer() override;
    void SubmitCommandBuffer(RHICommandBuffer* commandBuffer) override;
    void WaitIdle() override;

    RHIBufferPtr CreateBuffer(const BufferDesc& desc, const void* initialData = nullptr) override;
    RHITexturePtr CreateTexture(const TextureDesc& desc, const void* initialData = nullptr) override;
    RHISamplerPtr CreateSampler(const SamplerDesc& desc) override;

    RHIShaderPtr CreateShader(ShaderStage stage, const void* bytecode, size_t size) override;
    RHIShaderPtr CreateShaderFromSource(ShaderStage stage, const std::string& source, const std::string& entryPoint = "main") override;

    RHIPipelinePtr CreateGraphicsPipeline(
        const std::vector<RHIShader*>& shaders,
        const BlendStateDesc& blendState,
        const DepthStencilStateDesc& depthStencilState,
        const RasterizerStateDesc& rasterizerState,
        PrimitiveTopology topology
    ) override;

    RHIRenderPassPtr CreateRenderPass(
        const std::vector<TextureFormat>& colorFormats,
        TextureFormat depthFormat = TextureFormat::D24_UNORM_S8_UINT
    ) override;

    RHIFramebufferPtr CreateFramebuffer(
        RHIRenderPass* renderPass,
        const std::vector<RHITexture*>& colorAttachments,
        RHITexture* depthAttachment = nullptr
    ) override;

    RHITexture* GetBackBuffer() override;
    uint32_t GetBackBufferWidth() const override { return swapChainExtent_.width; }
    uint32_t GetBackBufferHeight() const override { return swapChainExtent_.height; }

    void ResizeSwapChain(uint32_t width, uint32_t height) override;

    bool IsDeviceLost() override { return deviceLost_; }
    bool ResetDevice() override;

    VkDevice GetVkDevice() const { return device_; }
    VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice_; }
    VkQueue GetGraphicsQueue() const { return graphicsQueue_; }
    VkCommandPool GetCommandPool() const { return commandPool_; }

private:
    bool CreateInstance();
    bool SelectPhysicalDevice();
    bool CreateLogicalDevice();
    bool CreateSwapChain(const SwapChainDesc& desc);
    bool CreateCommandPool();
    bool CreateSyncObjects();

    void CleanupSwapChain();
    void DestroySyncObjects();

    VkInstance instance_;
    VkDebugUtilsMessengerEXT debugMessenger_;
    VkSurfaceKHR surface_;
    VkPhysicalDevice physicalDevice_;
    VkDevice device_;
    VkQueue graphicsQueue_;
    VkQueue presentQueue_;
    uint32_t graphicsQueueFamily_;
    uint32_t presentQueueFamily_;

    VkSwapchainKHR swapChain_;
    std::vector<VkImage> swapChainImages_;
    std::vector<VkImageView> swapChainImageViews_;
    std::vector<RHITexturePtr> swapChainTextures_;
    VkFormat swapChainFormat_;
    VkExtent2D swapChainExtent_;

    VkCommandPool commandPool_;
    std::vector<VkCommandBuffer> commandBuffers_;

    // Acquire-side sync is per frame-in-flight; present-side sync is per swap
    // chain *image*. Both used to be indexed by currentFrame_, which is wrong
    // whenever the swap chain has more images than frames in flight (three is
    // the common case on Linux and on MoltenVK): the semaphore a present was
    // still waiting on could be re-signalled by the next submit. Sizing the
    // present semaphores to the image count removes the reuse entirely.
    std::vector<VkSemaphore> imageAvailableSemaphores_;
    std::vector<VkSemaphore> renderFinishedSemaphores_;
    std::vector<VkFence> inFlightFences_;

    /// Fence, if any, that the last submission touching each swap chain image
    /// was signalled with. Acquiring an image whose previous frame has not
    /// finished must wait for that fence, not merely for this frame's.
    /// Non-owning: every fence here is also in inFlightFences_.
    std::vector<VkFence> imagesInFlight_;

    uint32_t currentFrame_;
    uint32_t currentImageIndex_;
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    bool deviceLost_;
    void* windowHandle_;

    /// Kept so ResizeSwapChain can preserve the vsync/format/buffer-count the
    /// caller originally asked for instead of silently substituting defaults.
    SwapChainDesc swapChainDesc_;

    /// The instance-level API version actually granted. Requesting 1.3 from a
    /// 1.2 loader - which is what MoltenVK reports - fails instance creation
    /// outright, so this is negotiated rather than assumed.
    uint32_t apiVersion_;

    /// Set when the selected physical device advertises VK_KHR_portability_subset
    /// (i.e. it is MoltenVK). The spec *requires* that extension be enabled at
    /// device creation when it is present; omitting it is invalid usage.
    bool portabilitySubset_;
};

} // namespace RHI
} // namespace Nexus

#endif // NEXUS_VULKAN_ENABLED
