#ifdef NEXUS_VULKAN_ENABLED

#include "RHI/Vulkan/VulkanResource.h"
#include "RHI/Vulkan/VulkanDevice.h"
#include "Logger.h"

#include <cstring>   // std::memcpy

namespace Nexus {
namespace RHI {

static uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    return 0;
}

VulkanBuffer::VulkanBuffer(VulkanDevice* device, const BufferDesc& desc, const void* initialData)
    : device_(device)
    , buffer_(VK_NULL_HANDLE)
    , memory_(VK_NULL_HANDLE)
    , mappedData_(nullptr) {

    desc_ = desc;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = desc.size;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    switch (desc.usage) {
        case BufferUsage::Vertex:
            bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            break;
        case BufferUsage::Index:
            bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            break;
        case BufferUsage::Uniform:
            bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            break;
        case BufferUsage::Storage:
            bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            break;
    }

    if (vkCreateBuffer(device_->GetVkDevice(), &bufferInfo, nullptr, &buffer_) != VK_SUCCESS) {
        Logger::Error("Failed to create Vulkan buffer");
        return;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device_->GetVkDevice(), buffer_, &memRequirements);

    VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    if (desc.accessMode == BufferAccessMode::Static) {
        memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(device_->GetPhysicalDevice(), memRequirements.memoryTypeBits, memoryProperties);

    if (vkAllocateMemory(device_->GetVkDevice(), &allocInfo, nullptr, &memory_) != VK_SUCCESS) {
        Logger::Error("Failed to allocate Vulkan buffer memory");
        return;
    }

    vkBindBufferMemory(device_->GetVkDevice(), buffer_, memory_, 0);

    if (initialData) {
        UpdateData(initialData, desc.size, 0);
    }
}

VulkanBuffer::~VulkanBuffer() {
    if (mappedData_) {
        Unmap();
    }
    if (buffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_->GetVkDevice(), buffer_, nullptr);
    }
    if (memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_->GetVkDevice(), memory_, nullptr);
    }
}

void* VulkanBuffer::Map() {
    if (!mappedData_) {
        vkMapMemory(device_->GetVkDevice(), memory_, 0, desc_.size, 0, &mappedData_);
    }
    return mappedData_;
}

void VulkanBuffer::Unmap() {
    if (mappedData_) {
        vkUnmapMemory(device_->GetVkDevice(), memory_);
        mappedData_ = nullptr;
    }
}

void VulkanBuffer::UpdateData(const void* data, uint64_t size, uint64_t offset) {
    void* mapped = Map();
    if (mapped) {
        memcpy(static_cast<char*>(mapped) + offset, data, size);
        Unmap();
    }
}

VulkanTexture::VulkanTexture(VulkanDevice* device, const TextureDesc& desc, const void* initialData)
    : device_(device)
    , image_(VK_NULL_HANDLE)
    , imageView_(VK_NULL_HANDLE)
    , memory_(VK_NULL_HANDLE)
    , ownsImage_(true) {

    desc_ = desc;
    Logger::Warning("VulkanTexture creation from data not yet fully implemented");
}

VulkanTexture::VulkanTexture(VulkanDevice* device, VkImage image, VkImageView imageView, const TextureDesc& desc)
    : device_(device)
    , image_(image)
    , imageView_(imageView)
    , memory_(VK_NULL_HANDLE)
    , ownsImage_(false) {

    desc_ = desc;
}

VulkanTexture::~VulkanTexture() {
    if (ownsImage_) {
        if (imageView_ != VK_NULL_HANDLE) {
            vkDestroyImageView(device_->GetVkDevice(), imageView_, nullptr);
        }
        if (image_ != VK_NULL_HANDLE) {
            vkDestroyImage(device_->GetVkDevice(), image_, nullptr);
        }
        if (memory_ != VK_NULL_HANDLE) {
            vkFreeMemory(device_->GetVkDevice(), memory_, nullptr);
        }
    }
}

void VulkanTexture::UpdateData(const void* data, uint32_t mipLevel, uint32_t arraySlice) {
    Logger::Warning("VulkanTexture::UpdateData not yet implemented");
}

VulkanSampler::VulkanSampler(VulkanDevice* device, const SamplerDesc& desc)
    : device_(device)
    , sampler_(VK_NULL_HANDLE) {

    desc_ = desc;

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = desc.magFilter == SamplerFilter::Point ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
    samplerInfo.minFilter = desc.minFilter == SamplerFilter::Point ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = desc.mipFilter == SamplerFilter::Point ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;

    auto ConvertAddressMode = [](SamplerAddressMode mode) {
        switch (mode) {
            case SamplerAddressMode::Wrap: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            case SamplerAddressMode::Clamp: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case SamplerAddressMode::Mirror: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            case SamplerAddressMode::Border: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            default: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }
    };

    samplerInfo.addressModeU = ConvertAddressMode(desc.addressU);
    samplerInfo.addressModeV = ConvertAddressMode(desc.addressV);
    samplerInfo.addressModeW = ConvertAddressMode(desc.addressW);

    samplerInfo.anisotropyEnable = desc.maxAnisotropy > 1 ? VK_TRUE : VK_FALSE;
    samplerInfo.maxAnisotropy = static_cast<float>(desc.maxAnisotropy);
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipLodBias = desc.mipLodBias;
    samplerInfo.minLod = desc.minLod;
    samplerInfo.maxLod = desc.maxLod;

    if (vkCreateSampler(device_->GetVkDevice(), &samplerInfo, nullptr, &sampler_) != VK_SUCCESS) {
        Logger::Error("Failed to create Vulkan sampler");
    }
}

VulkanSampler::~VulkanSampler() {
    if (sampler_ != VK_NULL_HANDLE) {
        vkDestroySampler(device_->GetVkDevice(), sampler_, nullptr);
    }
}

VulkanShader::VulkanShader(VulkanDevice* device, ShaderStage stage, const void* bytecode, size_t size)
    : device_(device)
    , module_(VK_NULL_HANDLE) {

    stage_ = stage;

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = size;
    createInfo.pCode = reinterpret_cast<const uint32_t*>(bytecode);

    if (vkCreateShaderModule(device_->GetVkDevice(), &createInfo, nullptr, &module_) != VK_SUCCESS) {
        Logger::Error("Failed to create Vulkan shader module");
    }
}

VulkanShader::~VulkanShader() {
    if (module_ != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device_->GetVkDevice(), module_, nullptr);
    }
}

VulkanPipeline::VulkanPipeline(VulkanDevice* device, VkPipeline pipeline, VkPipelineLayout layout)
    : device_(device)
    , pipeline_(pipeline)
    , layout_(layout) {
}

VulkanPipeline::~VulkanPipeline() {
    if (pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_->GetVkDevice(), pipeline_, nullptr);
    }
    if (layout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_->GetVkDevice(), layout_, nullptr);
    }
}

VulkanRenderPass::VulkanRenderPass(VulkanDevice* device, VkRenderPass renderPass)
    : device_(device)
    , renderPass_(renderPass) {
}

VulkanRenderPass::~VulkanRenderPass() {
    if (renderPass_ != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device_->GetVkDevice(), renderPass_, nullptr);
    }
}

VulkanFramebuffer::VulkanFramebuffer(VulkanDevice* device, VkFramebuffer framebuffer, uint32_t width, uint32_t height)
    : device_(device)
    , framebuffer_(framebuffer)
    , width_(width)
    , height_(height) {
}

VulkanFramebuffer::~VulkanFramebuffer() {
    if (framebuffer_ != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(device_->GetVkDevice(), framebuffer_, nullptr);
    }
}

} // namespace RHI
} // namespace Nexus

#endif // NEXUS_VULKAN_ENABLED
