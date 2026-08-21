#ifdef NEXUS_VULKAN_ENABLED

#include "RHI/Vulkan/VulkanDevice.h"
#include "RHI/Vulkan/VulkanResource.h"
#include "RHI/Vulkan/VulkanCommandBuffer.h"
#include "Logger.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <algorithm>
#include <set>
#include <string>

namespace Nexus {
namespace RHI {

VkFormat ToVulkanFormat(TextureFormat format) {
    switch (format) {
        case TextureFormat::R8_UNORM: return VK_FORMAT_R8_UNORM;
        case TextureFormat::RG8_UNORM: return VK_FORMAT_R8G8_UNORM;
        case TextureFormat::RGBA8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
        case TextureFormat::RGBA8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
        case TextureFormat::R16_FLOAT: return VK_FORMAT_R16_SFLOAT;
        case TextureFormat::RG16_FLOAT: return VK_FORMAT_R16G16_SFLOAT;
        case TextureFormat::RGBA16_FLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
        case TextureFormat::R32_FLOAT: return VK_FORMAT_R32_SFLOAT;
        case TextureFormat::RG32_FLOAT: return VK_FORMAT_R32G32_SFLOAT;
        case TextureFormat::RGBA32_FLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
        case TextureFormat::D24_UNORM_S8_UINT: return VK_FORMAT_D24_UNORM_S8_UINT;
        case TextureFormat::D32_FLOAT: return VK_FORMAT_D32_SFLOAT;
        case TextureFormat::BC1_UNORM: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        case TextureFormat::BC3_UNORM: return VK_FORMAT_BC3_UNORM_BLOCK;
        case TextureFormat::BC5_UNORM: return VK_FORMAT_BC5_UNORM_BLOCK;
        case TextureFormat::BC7_UNORM: return VK_FORMAT_BC7_UNORM_BLOCK;
        default: return VK_FORMAT_UNDEFINED;
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        Logger::Warning("Vulkan Validation: " + std::string(pCallbackData->pMessage));
    }
    return VK_FALSE;
}

VulkanDevice::VulkanDevice()
    : instance_(VK_NULL_HANDLE)
    , debugMessenger_(VK_NULL_HANDLE)
    , surface_(VK_NULL_HANDLE)
    , physicalDevice_(VK_NULL_HANDLE)
    , device_(VK_NULL_HANDLE)
    , graphicsQueue_(VK_NULL_HANDLE)
    , presentQueue_(VK_NULL_HANDLE)
    , graphicsQueueFamily_(0)
    , presentQueueFamily_(0)
    , swapChain_(VK_NULL_HANDLE)
    , swapChainFormat_(VK_FORMAT_UNDEFINED)
    , commandPool_(VK_NULL_HANDLE)
    , currentFrame_(0)
    , currentImageIndex_(0)
    , deviceLost_(false)
    , windowHandle_(nullptr) {
}

VulkanDevice::~VulkanDevice() {
    Shutdown();
}

bool VulkanDevice::Initialize(const SwapChainDesc& swapChainDesc) {
    Logger::Info("Initializing Vulkan Device...");

    windowHandle_ = swapChainDesc.windowHandle;

    if (!CreateInstance()) {
        Logger::Error("Failed to create Vulkan instance");
        return false;
    }

    SDL_Window* window = static_cast<SDL_Window*>(windowHandle_);
    if (!SDL_Vulkan_CreateSurface(window, instance_, &surface_)) {
        Logger::Error("Failed to create Vulkan surface: " + std::string(SDL_GetError()));
        return false;
    }

    if (!SelectPhysicalDevice()) {
        Logger::Error("Failed to select physical device");
        return false;
    }

    if (!CreateLogicalDevice()) {
        Logger::Error("Failed to create logical device");
        return false;
    }

    if (!CreateSwapChain(swapChainDesc)) {
        Logger::Error("Failed to create swap chain");
        return false;
    }

    if (!CreateCommandPool()) {
        Logger::Error("Failed to create command pool");
        return false;
    }

    if (!CreateSyncObjects()) {
        Logger::Error("Failed to create sync objects");
        return false;
    }

    Logger::Info("Vulkan Device initialized successfully");
    return true;
}

void VulkanDevice::Shutdown() {
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);

        for (auto& fence : inFlightFences_) {
            vkDestroyFence(device_, fence, nullptr);
        }
        for (auto& semaphore : renderFinishedSemaphores_) {
            vkDestroySemaphore(device_, semaphore, nullptr);
        }
        for (auto& semaphore : imageAvailableSemaphores_) {
            vkDestroySemaphore(device_, semaphore, nullptr);
        }

        CleanupSwapChain();

        if (commandPool_ != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device_, commandPool_, nullptr);
        }

        vkDestroyDevice(device_, nullptr);
    }

    if (surface_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
    }

    if (debugMessenger_ != VK_NULL_HANDLE) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance_, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance_, debugMessenger_, nullptr);
        }
    }

    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
    }
}

bool VulkanDevice::CreateInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Nexus Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Nexus";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    SDL_Window* window = static_cast<SDL_Window*>(windowHandle_);
    uint32_t sdlExtensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, nullptr);
    std::vector<const char*> extensions(sdlExtensionCount);
    SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, extensions.data());

#ifdef NEXUS_DEBUG
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

#ifdef NEXUS_DEBUG
    const char* validationLayers[] = { "VK_LAYER_KHRONOS_validation" };
    createInfo.enabledLayerCount = 1;
    createInfo.ppEnabledLayerNames = validationLayers;
#else
    createInfo.enabledLayerCount = 0;
#endif

    if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS) {
        return false;
    }

#ifdef NEXUS_DEBUG
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = DebugCallback;

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance_, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance_, &debugCreateInfo, nullptr, &debugMessenger_);
    }
#endif

    return true;
}

bool VulkanDevice::SelectPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);

    if (deviceCount == 0) {
        Logger::Error("No Vulkan-capable GPUs found");
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

    for (const auto& device : devices) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        uint32_t graphicsFamily = UINT32_MAX;
        uint32_t presentFamily = UINT32_MAX;

        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);
            if (presentSupport) {
                presentFamily = i;
            }

            if (graphicsFamily != UINT32_MAX && presentFamily != UINT32_MAX) {
                break;
            }
        }

        if (graphicsFamily != UINT32_MAX && presentFamily != UINT32_MAX) {
            physicalDevice_ = device;
            graphicsQueueFamily_ = graphicsFamily;
            presentQueueFamily_ = presentFamily;

            Logger::Info("Selected GPU: " + std::string(deviceProperties.deviceName));
            return true;
        }
    }

    return false;
}

bool VulkanDevice::CreateLogicalDevice() {
    std::set<uint32_t> uniqueQueueFamilies = { graphicsQueueFamily_, presentQueueFamily_ };
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.fillModeNonSolid = VK_TRUE;

    const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = deviceExtensions;

    if (vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_) != VK_SUCCESS) {
        return false;
    }

    vkGetDeviceQueue(device_, graphicsQueueFamily_, 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, presentQueueFamily_, 0, &presentQueue_);

    return true;
}

bool VulkanDevice::CreateSwapChain(const SwapChainDesc& desc) {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface_, &capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, formats.data());

    VkSurfaceFormatKHR surfaceFormat = formats[0];
    for (const auto& availableFormat : formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = availableFormat;
            break;
        }
    }

    swapChainExtent_.width = std::clamp(desc.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    swapChainExtent_.height = std::clamp(desc.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    uint32_t imageCount = desc.bufferCount;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface_;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = swapChainExtent_;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    uint32_t queueFamilyIndices[] = { graphicsQueueFamily_, presentQueueFamily_ };

    if (graphicsQueueFamily_ != presentQueueFamily_) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = desc.vsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapChain_) != VK_SUCCESS) {
        return false;
    }

    swapChainFormat_ = surfaceFormat.format;

    vkGetSwapchainImagesKHR(device_, swapChain_, &imageCount, nullptr);
    swapChainImages_.resize(imageCount);
    vkGetSwapchainImagesKHR(device_, swapChain_, &imageCount, swapChainImages_.data());

    swapChainImageViews_.resize(imageCount);
    swapChainTextures_.resize(imageCount);

    for (size_t i = 0; i < imageCount; i++) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = swapChainImages_[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapChainFormat_;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device_, &viewInfo, nullptr, &swapChainImageViews_[i]) != VK_SUCCESS) {
            return false;
        }

        TextureDesc texDesc{};
        texDesc.width = swapChainExtent_.width;
        texDesc.height = swapChainExtent_.height;
        texDesc.depth = 1;
        texDesc.mipLevels = 1;
        texDesc.arraySize = 1;
        texDesc.format = TextureFormat::RGBA8_SRGB;
        texDesc.usage = TextureUsage::RenderTarget;
        texDesc.sampleCount = 1;

        swapChainTextures_[i] = std::make_shared<VulkanTexture>(
            this, swapChainImages_[i], swapChainImageViews_[i], texDesc);
    }

    return true;
}

bool VulkanDevice::CreateCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsQueueFamily_;

    return vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_) == VK_SUCCESS;
}

bool VulkanDevice::CreateSyncObjects() {
    imageAvailableSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences_.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &imageAvailableSemaphores_[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &renderFinishedSemaphores_[i]) != VK_SUCCESS ||
            vkCreateFence(device_, &fenceInfo, nullptr, &inFlightFences_[i]) != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

void VulkanDevice::CleanupSwapChain() {
    swapChainTextures_.clear();

    for (auto imageView : swapChainImageViews_) {
        vkDestroyImageView(device_, imageView, nullptr);
    }
    swapChainImageViews_.clear();

    if (swapChain_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device_, swapChain_, nullptr);
        swapChain_ = VK_NULL_HANDLE;
    }
}

void VulkanDevice::BeginFrame() {
    vkWaitForFences(device_, 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);

    VkResult result = vkAcquireNextImageKHR(device_, swapChain_, UINT64_MAX,
        imageAvailableSemaphores_[currentFrame_], VK_NULL_HANDLE, &currentImageIndex_);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        deviceLost_ = true;
        return;
    }

    vkResetFences(device_, 1, &inFlightFences_[currentFrame_]);
}

void VulkanDevice::EndFrame() {
}

void VulkanDevice::Present() {
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphores_[currentFrame_];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain_;
    presentInfo.pImageIndices = &currentImageIndex_;

    VkResult result = vkQueuePresentKHR(presentQueue_, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        deviceLost_ = true;
    }

    currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

RHICommandBufferPtr VulkanDevice::CreateCommandBuffer() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    if (vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer) != VK_SUCCESS) {
        return nullptr;
    }

    return std::make_unique<VulkanCommandBuffer>(this, commandBuffer);
}

void VulkanDevice::SubmitCommandBuffer(RHICommandBuffer* commandBuffer) {
    VulkanCommandBuffer* vkCmdBuf = static_cast<VulkanCommandBuffer*>(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores_[currentFrame_] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    VkCommandBuffer cmdBuf = vkCmdBuf->GetVkCommandBuffer();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuf;

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores_[currentFrame_] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFences_[currentFrame_]);
}

void VulkanDevice::WaitIdle() {
    vkDeviceWaitIdle(device_);
}

RHITexture* VulkanDevice::GetBackBuffer() {
    return swapChainTextures_[currentImageIndex_].get();
}

void VulkanDevice::ResizeSwapChain(uint32_t width, uint32_t height) {
    vkDeviceWaitIdle(device_);

    CleanupSwapChain();

    SwapChainDesc desc{};
    desc.windowHandle = windowHandle_;
    desc.width = width;
    desc.height = height;
    desc.bufferCount = 2;
    desc.format = TextureFormat::RGBA8_SRGB;
    desc.vsync = true;
    desc.fullscreen = false;

    CreateSwapChain(desc);
    deviceLost_ = false;
}

bool VulkanDevice::ResetDevice() {
    SDL_Window* window = static_cast<SDL_Window*>(windowHandle_);
    int width, height;
    SDL_GetWindowSize(window, &width, &height);

    ResizeSwapChain(width, height);
    return !deviceLost_;
}

RHIBufferPtr VulkanDevice::CreateBuffer(const BufferDesc& desc, const void* initialData) {
    return std::make_shared<VulkanBuffer>(this, desc, initialData);
}

RHITexturePtr VulkanDevice::CreateTexture(const TextureDesc& desc, const void* initialData) {
    return std::make_shared<VulkanTexture>(this, desc, initialData);
}

RHISamplerPtr VulkanDevice::CreateSampler(const SamplerDesc& desc) {
    return std::make_shared<VulkanSampler>(this, desc);
}

RHIShaderPtr VulkanDevice::CreateShader(ShaderStage stage, const void* bytecode, size_t size) {
    return std::make_shared<VulkanShader>(this, stage, bytecode, size);
}

RHIShaderPtr VulkanDevice::CreateShaderFromSource(ShaderStage stage, const std::string& source, const std::string& entryPoint) {
    Logger::Warning("CreateShaderFromSource not yet implemented for Vulkan - use SPIR-V bytecode");
    return nullptr;
}

RHIPipelinePtr VulkanDevice::CreateGraphicsPipeline(
    const std::vector<RHIShader*>& shaders,
    const BlendStateDesc& blendState,
    const DepthStencilStateDesc& depthStencilState,
    const RasterizerStateDesc& rasterizerState,
    PrimitiveTopology topology) {

    Logger::Warning("CreateGraphicsPipeline not yet fully implemented for Vulkan");
    return nullptr;
}

RHIRenderPassPtr VulkanDevice::CreateRenderPass(
    const std::vector<TextureFormat>& colorFormats,
    TextureFormat depthFormat) {

    Logger::Warning("CreateRenderPass not yet fully implemented for Vulkan");
    return nullptr;
}

RHIFramebufferPtr VulkanDevice::CreateFramebuffer(
    RHIRenderPass* renderPass,
    const std::vector<RHITexture*>& colorAttachments,
    RHITexture* depthAttachment) {

    Logger::Warning("CreateFramebuffer not yet fully implemented for Vulkan");
    return nullptr;
}

} // namespace RHI
} // namespace Nexus

#endif // NEXUS_VULKAN_ENABLED
