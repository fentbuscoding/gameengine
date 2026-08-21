#pragma once

#ifdef NEXUS_VULKAN_ENABLED

#include "../RHIResource.h"
#include <vulkan/vulkan.h>

namespace Nexus {
namespace RHI {

class VulkanDevice;

class VulkanBuffer : public RHIBuffer {
public:
    VulkanBuffer(VulkanDevice* device, const BufferDesc& desc, const void* initialData);
    ~VulkanBuffer() override;

    void* Map() override;
    void Unmap() override;
    void UpdateData(const void* data, uint64_t size, uint64_t offset = 0) override;

    VkBuffer GetVkBuffer() const { return buffer_; }
    VkDeviceMemory GetVkMemory() const { return memory_; }

private:
    VulkanDevice* device_;
    VkBuffer buffer_;
    VkDeviceMemory memory_;
    void* mappedData_;
};

class VulkanTexture : public RHITexture {
public:
    VulkanTexture(VulkanDevice* device, const TextureDesc& desc, const void* initialData);
    VulkanTexture(VulkanDevice* device, VkImage image, VkImageView imageView, const TextureDesc& desc);
    ~VulkanTexture() override;

    void UpdateData(const void* data, uint32_t mipLevel = 0, uint32_t arraySlice = 0) override;

    VkImage GetVkImage() const { return image_; }
    VkImageView GetVkImageView() const { return imageView_; }

    /// True for the images the swap chain owns, which the device wraps with the
    /// (image, view) constructor. They must end a frame in PRESENT_SRC_KHR,
    /// which is what makes their layout handling differ from a texture the
    /// engine allocated itself.
    bool IsSwapChainImage() const { return !ownsImage_; }

private:
    VulkanDevice* device_;
    VkImage image_;
    VkImageView imageView_;
    VkDeviceMemory memory_;
    bool ownsImage_;
};

class VulkanSampler : public RHISampler {
public:
    VulkanSampler(VulkanDevice* device, const SamplerDesc& desc);
    ~VulkanSampler() override;

    VkSampler GetVkSampler() const { return sampler_; }

private:
    VulkanDevice* device_;
    VkSampler sampler_;
};

class VulkanShader : public RHIShader {
public:
    VulkanShader(VulkanDevice* device, ShaderStage stage, const void* bytecode, size_t size);
    ~VulkanShader() override;

    VkShaderModule GetVkModule() const { return module_; }

private:
    VulkanDevice* device_;
    VkShaderModule module_;
};

class VulkanPipeline : public RHIPipeline {
public:
    VulkanPipeline(VulkanDevice* device, VkPipeline pipeline, VkPipelineLayout layout);
    ~VulkanPipeline() override;

    VkPipeline GetVkPipeline() const { return pipeline_; }
    VkPipelineLayout GetVkPipelineLayout() const { return layout_; }

private:
    VulkanDevice* device_;
    VkPipeline pipeline_;
    VkPipelineLayout layout_;
};

class VulkanRenderPass : public RHIRenderPass {
public:
    VulkanRenderPass(VulkanDevice* device, VkRenderPass renderPass);
    ~VulkanRenderPass() override;

    VkRenderPass GetVkRenderPass() const { return renderPass_; }

private:
    VulkanDevice* device_;
    VkRenderPass renderPass_;
};

class VulkanFramebuffer : public RHIFramebuffer {
public:
    VulkanFramebuffer(VulkanDevice* device, VkFramebuffer framebuffer, uint32_t width, uint32_t height);
    ~VulkanFramebuffer() override;

    VkFramebuffer GetVkFramebuffer() const { return framebuffer_; }
    uint32_t GetWidth() const { return width_; }
    uint32_t GetHeight() const { return height_; }

private:
    VulkanDevice* device_;
    VkFramebuffer framebuffer_;
    uint32_t width_;
    uint32_t height_;
};

} // namespace RHI
} // namespace Nexus

#endif // NEXUS_VULKAN_ENABLED
