#ifdef NEXUS_VULKAN_ENABLED

#include "RHI/Vulkan/VulkanDevice.h"
#include "RHI/Vulkan/VulkanResource.h"
#include "RHI/Vulkan/VulkanCommandBuffer.h"
#include "EngineConfig.h"   // NEXUS_DEBUG - without it the validation paths below vanish
#include "Logger.h"

#include "SDLVulkanCompat.h"

#include <algorithm>
#include <cstring>
#include <set>
#include <string>
#include <vector>

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
    , windowHandle_(nullptr)
    , swapChainDesc_{}
    , apiVersion_(VK_API_VERSION_1_0)
    , portabilitySubset_(false) {
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
    if (SDL_Vulkan_CreateSurface(window, instance_, &surface_) != SDL_TRUE) {
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
    // Every handle is cleared as it is destroyed. Shutdown is public *and*
    // called from the destructor, so without this an explicit Shutdown()
    // followed by destruction destroyed every object twice - a use-after-free
    // that only shows up under a validation layer or as a late crash.
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);

        DestroySyncObjects();
        CleanupSwapChain();

        if (commandPool_ != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device_, commandPool_, nullptr);
            commandPool_ = VK_NULL_HANDLE;
        }

        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
        graphicsQueue_ = VK_NULL_HANDLE;
        presentQueue_ = VK_NULL_HANDLE;
    }

    if (surface_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }

    if (debugMessenger_ != VK_NULL_HANDLE) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance_, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance_, debugMessenger_, nullptr);
        }
        debugMessenger_ = VK_NULL_HANDLE;
    }

    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }

    physicalDevice_ = VK_NULL_HANDLE;
}

namespace {

/// True when @p name appears in @p properties, comparing the fixed-size char
/// arrays Vulkan reports rather than assuming they are null-terminated strings
/// of a known length.
template <typename Property, typename NameField>
bool Contains(const std::vector<Property>& properties, NameField Property::*field, const char* name) {
    for (const Property& property : properties) {
        if (std::strncmp(property.*field, name, sizeof(property.*field)) == 0) {
            return true;
        }
    }
    return false;
}

std::vector<VkExtensionProperties> EnumerateInstanceExtensions() {
    uint32_t count = 0;
    if (vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr) != VK_SUCCESS || count == 0) {
        return {};
    }
    std::vector<VkExtensionProperties> extensions(count);
    if (vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data()) != VK_SUCCESS) {
        return {};
    }
    extensions.resize(count);
    return extensions;
}

std::vector<VkLayerProperties> EnumerateInstanceLayers() {
    uint32_t count = 0;
    if (vkEnumerateInstanceLayerProperties(&count, nullptr) != VK_SUCCESS || count == 0) {
        return {};
    }
    std::vector<VkLayerProperties> layers(count);
    if (vkEnumerateInstanceLayerProperties(&count, layers.data()) != VK_SUCCESS) {
        return {};
    }
    layers.resize(count);
    return layers;
}

std::vector<VkExtensionProperties> EnumerateDeviceExtensions(VkPhysicalDevice device) {
    uint32_t count = 0;
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr) != VK_SUCCESS || count == 0) {
        return {};
    }
    std::vector<VkExtensionProperties> extensions(count);
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data()) != VK_SUCCESS) {
        return {};
    }
    extensions.resize(count);
    return extensions;
}

/// The highest instance-level API version this loader supports.
///
/// vkEnumerateInstanceVersion only exists from Vulkan 1.1, so it is resolved
/// dynamically: on a 1.0 loader the symbol is absent and the answer is 1.0.
uint32_t QueryInstanceApiVersion() {
    auto enumerateVersion = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
        vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"));

    if (!enumerateVersion) {
        return VK_API_VERSION_1_0;
    }

    uint32_t version = VK_API_VERSION_1_0;
    if (enumerateVersion(&version) != VK_SUCCESS) {
        return VK_API_VERSION_1_0;
    }
    return version;
}

const char* const kValidationLayerName = "VK_LAYER_KHRONOS_validation";

} // namespace

bool VulkanDevice::CreateInstance() {
    SDL_Window* window = static_cast<SDL_Window*>(windowHandle_);
    if (!window) {
        Logger::Error("Vulkan needs a window: SwapChainDesc::windowHandle was null");
        return false;
    }

    // The engine asked for Vulkan 1.3 unconditionally. vkCreateInstance fails
    // with VK_ERROR_INCOMPATIBLE_DRIVER when the loader supports less, and
    // MoltenVK reports 1.2 - so on macOS this failed before doing anything
    // else. Ask for no more than the loader has.
    const uint32_t loaderVersion = QueryInstanceApiVersion();
    apiVersion_ = std::min(loaderVersion, static_cast<uint32_t>(VK_API_VERSION_1_3));

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Nexus Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Nexus";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = apiVersion_;

    uint32_t sdlExtensionCount = 0;
    if (SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, nullptr) != SDL_TRUE) {
        // Both calls were previously unchecked, so a window created without
        // SDL_WINDOW_VULKAN produced an empty extension list and a surfaceless
        // instance rather than a diagnosable failure.
        Logger::Error("SDL_Vulkan_GetInstanceExtensions failed: " + std::string(SDL_GetError()));
        return false;
    }

    std::vector<const char*> extensions(sdlExtensionCount);
    if (SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, extensions.data()) != SDL_TRUE) {
        Logger::Error("SDL_Vulkan_GetInstanceExtensions failed: " + std::string(SDL_GetError()));
        return false;
    }
    extensions.resize(sdlExtensionCount);

    const std::vector<VkExtensionProperties> available = EnumerateInstanceExtensions();

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

#ifdef VK_KHR_portability_enumeration
    // MoltenVK is not a conformant Vulkan implementation, so from loader 1.3.216
    // onwards it is hidden unless the instance opts in to enumerating portability
    // drivers. Without this, vkCreateInstance succeeds and then
    // vkEnumeratePhysicalDevices reports zero GPUs on every Mac.
    if (Contains(available, &VkExtensionProperties::extensionName,
                 VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

        // VK_KHR_portability_subset, which the device must enable later, depends
        // on this instance extension. It is core from 1.1, so only request it
        // when the instance is 1.0.
        if (apiVersion_ < VK_API_VERSION_1_1 &&
            Contains(available, &VkExtensionProperties::extensionName,
                     VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
            extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        }
    }
#endif

    // Validation layers and the debug messenger were requested unconditionally
    // in debug builds. Neither ships with the runtime loader, so a debug build
    // on a machine without the SDK failed at vkCreateInstance with
    // VK_ERROR_LAYER_NOT_PRESENT. Both are now best-effort.
    std::vector<const char*> layers;
    bool debugUtils = false;

#ifdef NEXUS_DEBUG
    if (Contains(available, &VkExtensionProperties::extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        debugUtils = true;
    }

    if (Contains(EnumerateInstanceLayers(), &VkLayerProperties::layerName, kValidationLayerName)) {
        layers.push_back(kValidationLayerName);
    } else {
        Logger::Warning("Vulkan validation layers requested but not installed - continuing without them");
    }
#endif

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
    createInfo.ppEnabledLayerNames = layers.empty() ? nullptr : layers.data();

    const VkResult result = vkCreateInstance(&createInfo, nullptr, &instance_);
    if (result != VK_SUCCESS) {
        Logger::Error("vkCreateInstance failed (VkResult " + std::to_string(static_cast<int>(result)) + ")");
        return false;
    }

    Logger::Info("Vulkan instance created (API " +
                 std::to_string(VK_VERSION_MAJOR(apiVersion_)) + "." +
                 std::to_string(VK_VERSION_MINOR(apiVersion_)) + ")");

    if (debugUtils) {
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
    }

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

    // Rank candidates rather than taking the first that can present. The old
    // loop picked whatever the driver listed first, which on a laptop with
    // switchable graphics is usually the integrated GPU, and on a system with
    // a software rasteriser installed (lavapipe, common in CI) is that.
    VkPhysicalDevice best = VK_NULL_HANDLE;
    int bestScore = -1;
    uint32_t bestGraphics = UINT32_MAX;
    uint32_t bestPresent = UINT32_MAX;
    bool bestPortability = false;
    std::string bestName;

    for (VkPhysicalDevice device : devices) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);

        const std::vector<VkExtensionProperties> extensions = EnumerateDeviceExtensions(device);

        // Presenting without VK_KHR_swapchain is impossible; this was never
        // checked, so a device missing it was selected and failed later at
        // vkCreateDevice with a much less obvious error.
        if (!Contains(extensions, &VkExtensionProperties::extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
            continue;
        }

        // A device with no surface formats or present modes cannot drive this
        // surface, and CreateSwapChain would read formats[0] out of bounds.
        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr);
        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, nullptr);
        if (formatCount == 0 || presentModeCount == 0) {
            continue;
        }

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        uint32_t graphicsFamily = UINT32_MAX;
        uint32_t presentFamily = UINT32_MAX;
        uint32_t combinedFamily = UINT32_MAX;

        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            const bool graphics = (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;

            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);

            if (graphics && graphicsFamily == UINT32_MAX) {
                graphicsFamily = i;
            }
            if (presentSupport && presentFamily == UINT32_MAX) {
                presentFamily = i;
            }
            if (graphics && presentSupport && combinedFamily == UINT32_MAX) {
                combinedFamily = i;
            }
        }

        if (graphicsFamily == UINT32_MAX || presentFamily == UINT32_MAX) {
            continue;
        }

        // One family that can do both avoids VK_SHARING_MODE_CONCURRENT and the
        // throughput cost that comes with it, so prefer it when it exists.
        if (combinedFamily != UINT32_MAX) {
            graphicsFamily = combinedFamily;
            presentFamily = combinedFamily;
        }

        int score = 0;
        switch (properties.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   score = 400; break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: score = 300; break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    score = 200; break;
            default:                                     score = 100; break;  // CPU / other
        }
        if (graphicsFamily == presentFamily) {
            score += 10;
        }

        if (score > bestScore) {
            bestScore = score;
            best = device;
            bestGraphics = graphicsFamily;
            bestPresent = presentFamily;
            bestName = properties.deviceName;
            bestPortability = Contains(extensions, &VkExtensionProperties::extensionName,
                                       "VK_KHR_portability_subset");
        }
    }

    if (best == VK_NULL_HANDLE) {
        Logger::Error("No Vulkan GPU can present to this surface");
        return false;
    }

    physicalDevice_ = best;
    graphicsQueueFamily_ = bestGraphics;
    presentQueueFamily_ = bestPresent;
    portabilitySubset_ = bestPortability;

    Logger::Info("Selected GPU: " + bestName + (portabilitySubset_ ? " (portability subset)" : ""));
    return true;
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

    // samplerAnisotropy and fillModeNonSolid were requested unconditionally.
    // Requesting a feature the device does not have makes vkCreateDevice fail,
    // and MoltenVK reports fillModeNonSolid as unsupported on some hardware, so
    // they are now requested only where they exist.
    VkPhysicalDeviceFeatures supported{};
    vkGetPhysicalDeviceFeatures(physicalDevice_, &supported);

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = supported.samplerAnisotropy;
    deviceFeatures.fillModeNonSolid = supported.fillModeNonSolid;

    std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    if (portabilitySubset_) {
        // Required by the specification: if the device exposes
        // VK_KHR_portability_subset, it *must* be enabled. Leaving it out is
        // invalid usage that validation layers flag and some loaders reject.
        deviceExtensions.push_back("VK_KHR_portability_subset");
    }

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    const VkResult result = vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_);
    if (result != VK_SUCCESS) {
        Logger::Error("vkCreateDevice failed (VkResult " + std::to_string(static_cast<int>(result)) + ")");
        return false;
    }

    vkGetDeviceQueue(device_, graphicsQueueFamily_, 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, presentQueueFamily_, 0, &presentQueue_);

    return true;
}

bool VulkanDevice::CreateSwapChain(const SwapChainDesc& desc) {
    swapChainDesc_ = desc;

    VkSurfaceCapabilitiesKHR capabilities{};
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface_, &capabilities) != VK_SUCCESS) {
        Logger::Error("Failed to query surface capabilities");
        return false;
    }

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, nullptr);
    if (formatCount == 0) {
        // The old code indexed formats[0] regardless, so an empty list was a
        // read past the end of an empty vector.
        Logger::Error("Surface reports no supported formats");
        return false;
    }
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

    // When currentExtent is not the "surface size is determined by the swap
    // chain" sentinel, the specification requires the swap chain to match it
    // exactly. Clamping the caller's requested size, as this used to do, gives
    // a mismatched swap chain on every platform that reports a fixed extent -
    // which is most of them, and all of macOS.
    if (capabilities.currentExtent.width != UINT32_MAX) {
        swapChainExtent_ = capabilities.currentExtent;
    } else {
        swapChainExtent_.width = std::clamp(desc.width,
                                            capabilities.minImageExtent.width,
                                            capabilities.maxImageExtent.width);
        swapChainExtent_.height = std::clamp(desc.height,
                                             capabilities.minImageExtent.height,
                                             capabilities.maxImageExtent.height);
    }

    if (swapChainExtent_.width == 0 || swapChainExtent_.height == 0) {
        // Minimised window. Creating a zero-extent swap chain is invalid usage;
        // the caller is expected to retry when the window is restored.
        Logger::Warning("Swap chain not created: window has a zero-sized drawable");
        return false;
    }

    uint32_t imageCount = std::max(desc.bufferCount, capabilities.minImageCount);
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    // Only FIFO is guaranteed to exist. Requesting IMMEDIATE unconditionally for
    // vsync-off - as before - fails swap chain creation on drivers that lack it,
    // MoltenVK among them. MAILBOX is the better tear-free alternative when the
    // caller wants vsync off, so try that first.
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    if (!desc.vsync) {
        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface_, &presentModeCount, nullptr);
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface_, &presentModeCount, presentModes.data());

        const auto supports = [&presentModes](VkPresentModeKHR mode) {
            return std::find(presentModes.begin(), presentModes.end(), mode) != presentModes.end();
        };

        if (supports(VK_PRESENT_MODE_MAILBOX_KHR)) {
            presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        } else if (supports(VK_PRESENT_MODE_IMMEDIATE_KHR)) {
            presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface_;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = swapChainExtent_;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // TRANSFER_DST is what ClearRenderTarget needs, but it is not universally
    // supported for swap chain images; adding it unconditionally can fail
    // creation outright. Add it only where the surface allows it.
    if (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
        createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    uint32_t queueFamilyIndices[] = { graphicsQueueFamily_, presentQueueFamily_ };

    if (graphicsQueueFamily_ != presentQueueFamily_) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = capabilities.currentTransform;

    // OPAQUE is not always available - Wayland compositors commonly offer only
    // the pre-multiplied or inherit modes - so pick from what the surface says
    // it supports rather than assuming.
    const VkCompositeAlphaFlagBitsKHR compositeAlphaPreference[] = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    for (VkCompositeAlphaFlagBitsKHR mode : compositeAlphaPreference) {
        if (capabilities.supportedCompositeAlpha & mode) {
            createInfo.compositeAlpha = mode;
            break;
        }
    }

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    const VkResult result = vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapChain_);
    if (result != VK_SUCCESS) {
        Logger::Error("vkCreateSwapchainKHR failed (VkResult " + std::to_string(static_cast<int>(result)) + ")");
        swapChain_ = VK_NULL_HANDLE;
        return false;
    }

    swapChainFormat_ = surfaceFormat.format;

    vkGetSwapchainImagesKHR(device_, swapChain_, &imageCount, nullptr);
    swapChainImages_.resize(imageCount);
    vkGetSwapchainImagesKHR(device_, swapChain_, &imageCount, swapChainImages_.data());

    swapChainImageViews_.resize(imageCount);
    swapChainTextures_.resize(imageCount);

    // The driver may hand back more images than were asked for, and the count
    // can change across a resize, so the per-image tracking is sized here.
    imagesInFlight_.assign(imageCount, VK_NULL_HANDLE);

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
            Logger::Error("Failed to create swap chain image view");
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

    // One present-completion semaphore per image, created alongside the images
    // they belong to so a resize that changes the image count keeps them in
    // step. See the comment on renderFinishedSemaphores_ in the header.
    renderFinishedSemaphores_.resize(imageCount);
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (uint32_t i = 0; i < imageCount; ++i) {
        if (vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &renderFinishedSemaphores_[i]) != VK_SUCCESS) {
            Logger::Error("Failed to create present semaphore");
            return false;
        }
    }

    Logger::Info("Swap chain: " + std::to_string(swapChainExtent_.width) + "x" +
                 std::to_string(swapChainExtent_.height) + ", " +
                 std::to_string(imageCount) + " images");
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
    // Acquire semaphores and submission fences are per frame-in-flight. The
    // present semaphores are per swap chain image and are created with the swap
    // chain instead, because their count follows the image count.
    imageAvailableSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences_.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &imageAvailableSemaphores_[i]) != VK_SUCCESS ||
            vkCreateFence(device_, &fenceInfo, nullptr, &inFlightFences_[i]) != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

void VulkanDevice::DestroySyncObjects() {
    for (VkFence fence : inFlightFences_) {
        if (fence != VK_NULL_HANDLE) {
            vkDestroyFence(device_, fence, nullptr);
        }
    }
    inFlightFences_.clear();

    for (VkSemaphore semaphore : imageAvailableSemaphores_) {
        if (semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(device_, semaphore, nullptr);
        }
    }
    imageAvailableSemaphores_.clear();
}

void VulkanDevice::CleanupSwapChain() {
    swapChainTextures_.clear();

    for (VkSemaphore semaphore : renderFinishedSemaphores_) {
        if (semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(device_, semaphore, nullptr);
        }
    }
    renderFinishedSemaphores_.clear();

    for (auto imageView : swapChainImageViews_) {
        vkDestroyImageView(device_, imageView, nullptr);
    }
    swapChainImageViews_.clear();
    swapChainImages_.clear();
    imagesInFlight_.clear();

    if (swapChain_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device_, swapChain_, nullptr);
        swapChain_ = VK_NULL_HANDLE;
    }
}

void VulkanDevice::BeginFrame() {
    if (swapChain_ == VK_NULL_HANDLE) {
        deviceLost_ = true;
        return;
    }

    vkWaitForFences(device_, 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);

    VkResult result = vkAcquireNextImageKHR(device_, swapChain_, UINT64_MAX,
        imageAvailableSemaphores_[currentFrame_], VK_NULL_HANDLE, &currentImageIndex_);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Do not reset the fence: nothing will be submitted this frame, and a
        // reset fence that is never signalled deadlocks the next BeginFrame.
        deviceLost_ = true;
        return;
    }

    if (result != VK_SUCCESS) {
        Logger::Error("vkAcquireNextImageKHR failed (VkResult " +
                      std::to_string(static_cast<int>(result)) + ")");
        deviceLost_ = true;
        return;
    }

    // The image we just acquired may still be in use by an earlier frame that
    // has not retired - the acquire semaphore says the *presentation engine* is
    // done with it, not the GPU. Without this wait, a swap chain with more
    // images than frames in flight can have two submissions writing the same
    // image at once.
    if (currentImageIndex_ < imagesInFlight_.size() &&
        imagesInFlight_[currentImageIndex_] != VK_NULL_HANDLE) {
        vkWaitForFences(device_, 1, &imagesInFlight_[currentImageIndex_], VK_TRUE, UINT64_MAX);
    }
    if (currentImageIndex_ < imagesInFlight_.size()) {
        imagesInFlight_[currentImageIndex_] = inFlightFences_[currentFrame_];
    }

    vkResetFences(device_, 1, &inFlightFences_[currentFrame_]);
}

void VulkanDevice::EndFrame() {
}

void VulkanDevice::Present() {
    if (swapChain_ == VK_NULL_HANDLE || renderFinishedSemaphores_.empty()) {
        deviceLost_ = true;
        return;
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphores_[currentImageIndex_];
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

    // Signalled per image, not per frame: Present waits on the semaphore
    // belonging to the image it is presenting, and that image may still be
    // queued for display when the next frame submits.
    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores_[currentImageIndex_] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    const VkResult result = vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFences_[currentFrame_]);
    if (result != VK_SUCCESS) {
        Logger::Error("vkQueueSubmit failed (VkResult " + std::to_string(static_cast<int>(result)) + ")");
        deviceLost_ = true;
    }
}

void VulkanDevice::WaitIdle() {
    vkDeviceWaitIdle(device_);
}

RHITexture* VulkanDevice::GetBackBuffer() {
    return swapChainTextures_[currentImageIndex_].get();
}

void VulkanDevice::ResizeSwapChain(uint32_t width, uint32_t height) {
    if (device_ == VK_NULL_HANDLE) {
        return;
    }

    vkDeviceWaitIdle(device_);
    CleanupSwapChain();

    // Reuse the description the swap chain was originally created from and
    // change only the size. The old code substituted hard-coded defaults here,
    // so the first resize silently forced vsync on and the buffer count to two,
    // whatever the application had asked for.
    SwapChainDesc desc = swapChainDesc_;
    desc.windowHandle = windowHandle_;
    desc.width = width;
    desc.height = height;
    if (desc.bufferCount == 0) {
        desc.bufferCount = 2;
    }

    // A failed recreate - a minimised window is the usual cause - leaves the
    // device marked lost so the caller retries rather than presenting to a
    // swap chain that does not exist.
    deviceLost_ = !CreateSwapChain(desc);

    // Restart the frame counter: the fences it indexes were all waited on by
    // vkDeviceWaitIdle above, and the image count may have changed.
    currentFrame_ = 0;
    currentImageIndex_ = 0;
}

bool VulkanDevice::ResetDevice() {
    SDL_Window* window = static_cast<SDL_Window*>(windowHandle_);
    if (!window) {
        return false;
    }

    // Drawable size, not window size: on a HiDPI display - every Retina Mac -
    // they differ by the display scale, and a swap chain built from the logical
    // size renders at a fraction of the window's real resolution.
    int width = 0;
    int height = 0;
    SDL_Vulkan_GetDrawableSize(window, &width, &height);
    if (width <= 0 || height <= 0) {
        return false;
    }

    ResizeSwapChain(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
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
