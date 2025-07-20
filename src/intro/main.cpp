#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include <set>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

constexpr std::uint32_t WIDTH = 800;
constexpr std::uint32_t HEIGHT = 600;

constexpr std::array VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };

#ifdef NDEBUG
constexpr bool ENABLE_VALIDATION_LAYERS = false;
#else
constexpr bool ENABLE_VALIDATION_LAYERS = true;
#endif

struct QueueFamilyIndices
{
  std::uint32_t graphicsFamily = UINT32_MAX;
  std::uint32_t presentFamily = UINT32_MAX;

  [[nodiscard]] bool IsComplete() const { return graphicsFamily != UINT32_MAX && presentFamily != UINT32_MAX; }

  [[nodiscard]] std::set<std::uint32_t> GetUniqueIndices() const { return { graphicsFamily, presentFamily }; }
};

struct SwapChainSupportDetails
{
  vk::SurfaceCapabilitiesKHR capabilities;
  std::vector<vk::SurfaceFormatKHR> formats;
  std::vector<vk::PresentModeKHR> presentModes;
};

class HelloVulkan
{
public:
  void Run()
  {
    InitWindow();
    InitVulkan();
    MainLoop();
    Cleanup();
  }

private:
  void InitWindow()
  {
    if (glfwInit() == GLFW_FALSE) {
      glfwTerminate();
      throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Hello Vulkan", nullptr, nullptr);
    if (window == nullptr) {
      glfwTerminate();
      throw std::runtime_error("Failed to create GLFW window");
    }
  }

  void CreateInstance()
  {
    vk::ApplicationInfo appInfo{};
    appInfo.pApplicationName = "Hello Vulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = vk::ApiVersion14;

    auto requiredLayers = GetRequiredLayers();
    auto requiredExtensions = GetRequiredExtensions();

    ValidateLayerSupport(requiredLayers);
    ValidateExtensionSupport(requiredExtensions);

    vk::InstanceCreateInfo createInfo{};
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = static_cast<std::uint32_t>(requiredLayers.size());
    createInfo.ppEnabledLayerNames = requiredLayers.data();
    createInfo.enabledExtensionCount = static_cast<std::uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    instance = vk::raii::Instance(context, createInfo);
  }

  [[nodiscard]] static std::vector<const char *> GetRequiredLayers()
  {
    std::vector<const char *> layers;
    if (ENABLE_VALIDATION_LAYERS) { layers.assign(VALIDATION_LAYERS.begin(), VALIDATION_LAYERS.end()); }
    return layers;
  }

  [[nodiscard]] static std::vector<const char *> GetRequiredExtensions()
  {
    std::uint32_t glfwExtensionCount = 0;
    auto const *glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char *> extensions;
    extensions.reserve(glfwExtensionCount + (ENABLE_VALIDATION_LAYERS ? 1U : 0U));

    const std::span<const char *const> extensionSpan(glfwExtensions, glfwExtensionCount);
    std::ranges::copy(extensionSpan, std::back_inserter(extensions));

    if (ENABLE_VALIDATION_LAYERS) { extensions.push_back(vk::EXTDebugUtilsExtensionName); }

    return extensions;
  }

  void ValidateLayerSupport(const std::vector<const char *> &requiredLayers) const
  {
    if (requiredLayers.empty()) { return; }

    auto layerProperties = context.enumerateInstanceLayerProperties();
    auto unsupportedLayer = std::ranges::find_if(requiredLayers, [&layerProperties](auto const &requiredLayer) {
      return std::ranges::none_of(layerProperties,
        [requiredLayer](auto const &layerProperty) { return strcmp(layerProperty.layerName, requiredLayer) == 0; });
    });

    if (unsupportedLayer != requiredLayers.end()) {
      throw std::runtime_error("Required layer not supported: " + std::string(*unsupportedLayer));
    }
  }

  void ValidateExtensionSupport(const std::vector<const char *> &requiredExtensions) const
  {
    auto extensionProperties = context.enumerateInstanceExtensionProperties();
    auto unsupportedExtension =
      std::ranges::find_if(requiredExtensions, [&extensionProperties](auto const &requiredExtension) {
        return std::ranges::none_of(extensionProperties, [requiredExtension](auto const &extensionProperty) {
          return strcmp(extensionProperty.extensionName, requiredExtension) == 0;
        });
      });

    if (unsupportedExtension != requiredExtensions.end()) {
      throw std::runtime_error("Required extension not supported: " + std::string(*unsupportedExtension));
    }
  }

  void SetupDebugMessenger()
  {
    if (!ENABLE_VALIDATION_LAYERS) { return; }

    const vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
                                                              | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
                                                              | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);

    const vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                                                             | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
                                                             | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{};
    debugUtilsMessengerCreateInfoEXT.messageSeverity = severityFlags;
    debugUtilsMessengerCreateInfoEXT.messageType = messageTypeFlags;
    debugUtilsMessengerCreateInfoEXT.pfnUserCallback = &DebugCallback;

    debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
  }

  static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    vk::DebugUtilsMessageTypeFlagsEXT /*messageType*/,
    const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void * /*pUserData*/)
  {
    if (severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
      std::cerr << "validation layer: " << pCallbackData->pMessage << '\n';
    }

    return vk::False;
  }

  void CreateSurface()
  {
    VkSurfaceKHR rawSurface = VK_NULL_HANDLE;
    if (glfwCreateWindowSurface(*instance, window, nullptr, &rawSurface) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create window surface");
    }
    surface = vk::raii::SurfaceKHR(instance, rawSurface);
  }

  void PickPhysicalDevice()
  {
    std::vector<vk::raii::PhysicalDevice> devices = instance.enumeratePhysicalDevices();
    if (devices.empty()) { throw std::runtime_error("Failed to find GPUs with Vulkan support"); }

    auto deviceIter = std::ranges::find_if(
      devices, [this](const vk::raii::PhysicalDevice &physDev) { return IsDeviceSuitable(physDev); });

    if (deviceIter == devices.end()) { throw std::runtime_error("Failed to find a suitable GPU"); }

    physicalDevice = *deviceIter;
  }

  [[nodiscard]] bool IsDeviceSuitable(const vk::raii::PhysicalDevice &phyDevice) const
  {
    const auto properties = phyDevice.getProperties();
    if (properties.apiVersion < VK_API_VERSION_1_3) { return false; }

    if (!FindQueueFamilies(phyDevice).IsComplete()) { return false; }

    if (!CheckDeviceExtensionSupport(phyDevice)) { return false; }

    auto swapChainSupport = QuerySwapChainSupport(phyDevice);
    if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty()) { return false; }

    return CheckDeviceFeatureSupport(phyDevice);
  }

  [[nodiscard]] QueueFamilyIndices FindQueueFamilies(const vk::raii::PhysicalDevice &phyDevice) const
  {
    QueueFamilyIndices indices;
    const auto queueFamilyProperties = phyDevice.getQueueFamilyProperties();

    for (std::uint32_t i = 0; i < queueFamilyProperties.size(); ++i) {
      if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) != vk::QueueFlags{}) {
        indices.graphicsFamily = i;
      }

      if (phyDevice.getSurfaceSupportKHR(i, *surface) != 0U) { indices.presentFamily = i; }

      if (indices.IsComplete()) { break; }
    }

    return indices;
  }

  [[nodiscard]] bool CheckDeviceExtensionSupport(const vk::raii::PhysicalDevice &phyDevice) const
  {
    const auto availableExtensions = phyDevice.enumerateDeviceExtensionProperties();

    return std::ranges::all_of(requiredDeviceExtension, [&](const char *ext) {
      return std::ranges::any_of(
        availableExtensions, [ext](const auto &prop) { return strcmp(prop.extensionName, ext) == 0; });
    });
  }

  [[nodiscard]] static bool CheckDeviceFeatureSupport(const vk::raii::PhysicalDevice &device)
  {
    const auto features = device.getFeatures2<vk::PhysicalDeviceFeatures2,
      vk::PhysicalDeviceVulkan13Features,
      vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();

    return (features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering != 0U)
           && (features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState != 0U);
  }

  [[nodiscard]] SwapChainSupportDetails QuerySwapChainSupport(const vk::raii::PhysicalDevice &phyDevice) const
  {
    SwapChainSupportDetails details;
    details.capabilities = phyDevice.getSurfaceCapabilitiesKHR(*surface);
    details.formats = phyDevice.getSurfaceFormatsKHR(*surface);
    details.presentModes = phyDevice.getSurfacePresentModesKHR(*surface);
    return details;
  }

  void CreateLogicalDevice()
  {
    const QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    const auto uniqueQueueFamilies = indices.GetUniqueIndices();

    constexpr float queuePriority = 1.0F;
    for (const std::uint32_t queueFamily : uniqueQueueFamilies) {
      vk::DeviceQueueCreateInfo queueCreateInfo{};
      queueCreateInfo.queueFamilyIndex = queueFamily;
      queueCreateInfo.queueCount = 1;
      queueCreateInfo.pQueuePriorities = &queuePriority;
      queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::PhysicalDeviceVulkan13Features vulkan13Features{};
    vulkan13Features.dynamicRendering = VK_TRUE;

    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures{};
    extendedDynamicStateFeatures.pNext = &vulkan13Features;
    extendedDynamicStateFeatures.extendedDynamicState = VK_TRUE;

    vk::PhysicalDeviceFeatures2 deviceFeatures{};
    deviceFeatures.pNext = &extendedDynamicStateFeatures;

    vk::DeviceCreateInfo createInfo{};
    createInfo.pNext = &deviceFeatures;
    createInfo.queueCreateInfoCount = static_cast<std::uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = static_cast<std::uint32_t>(requiredDeviceExtension.size());
    createInfo.ppEnabledExtensionNames = requiredDeviceExtension.data();

    device = vk::raii::Device(physicalDevice, createInfo);

    graphicsQueue = vk::raii::Queue(device, indices.graphicsFamily, 0);
    presentQueue = vk::raii::Queue(device, indices.presentFamily, 0);
  }

  void CreateSwapChain()
  {
    auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
    swapChainImageFormat = ChooseSwapSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(*surface)).format;
    swapChainExtent = ChooseSwapExtent(surfaceCapabilities);

    auto minImageCount = std::max(3U, surfaceCapabilities.minImageCount);
    minImageCount = (surfaceCapabilities.maxImageCount > 0 && minImageCount > surfaceCapabilities.maxImageCount)
                      ? surfaceCapabilities.maxImageCount
                      : minImageCount;

    vk::SwapchainCreateInfoKHR swapChainCreateInfo{};
    swapChainCreateInfo.surface = *surface;
    swapChainCreateInfo.minImageCount = minImageCount;
    swapChainCreateInfo.imageFormat = swapChainImageFormat;
    swapChainCreateInfo.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
    swapChainCreateInfo.imageExtent = swapChainExtent;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

    const QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);
    std::array<std::uint32_t, 2> queueFamilyIndices = { indices.graphicsFamily, indices.presentFamily };

    if (indices.graphicsFamily != indices.presentFamily) {
      swapChainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
      swapChainCreateInfo.queueFamilyIndexCount = 2;
      swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    } else {
      swapChainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
      swapChainCreateInfo.queueFamilyIndexCount = 0;
      swapChainCreateInfo.pQueueFamilyIndices = nullptr;
    }

    swapChainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    swapChainCreateInfo.presentMode = ChooseSwapPresentMode(physicalDevice.getSurfacePresentModesKHR(*surface));
    swapChainCreateInfo.clipped = VK_TRUE;
    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    swapChain = vk::raii::SwapchainKHR(device, swapChainCreateInfo);
    swapChainImages = swapChain.getImages();
  }

  [[nodiscard]] static vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR> &availableFormats)
  {
    auto it = std::ranges::find_if(availableFormats, [](const auto &format) {
      return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
    });

    return (it != availableFormats.end()) ? *it : availableFormats[0];
  }

  [[nodiscard]] static vk::PresentModeKHR ChooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR> &availablePresentModes)
  {
    auto it = std::ranges::find_if(
      availablePresentModes, [](const auto &mode) { return mode == vk::PresentModeKHR::eMailbox; });

    return (it != availablePresentModes.end()) ? *it : vk::PresentModeKHR::eFifo;
  }

  [[nodiscard]] vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) const
  {
    if (capabilities.currentExtent.width != UINT32_MAX) { return capabilities.currentExtent; }

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);

    vk::Extent2D actualExtent = { static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height) };

    actualExtent.width =
      std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height =
      std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
  }

  void CreateImageViews()
  {
    swapChainImageViews.clear();

    vk::ImageViewCreateInfo imageViewCreateInfo{};
    imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
    imageViewCreateInfo.format = swapChainImageFormat;
    imageViewCreateInfo.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

    for (auto image : swapChainImages) {
      imageViewCreateInfo.image = image;
      swapChainImageViews.emplace_back(device, imageViewCreateInfo);
    }
  }

  void InitVulkan()
  {
    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapChain();
    CreateImageViews();
  }

  void MainLoop() const
  {
    while (glfwWindowShouldClose(window) == GLFW_FALSE) { glfwPollEvents(); }
  }

  void Cleanup() const
  {
    if (window != nullptr) { glfwDestroyWindow(window); }
    glfwTerminate();
  }

  GLFWwindow *window = nullptr;

  vk::raii::Context context;
  vk::raii::Instance instance = nullptr;
  vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
  vk::raii::SurfaceKHR surface = nullptr;

  vk::raii::PhysicalDevice physicalDevice = nullptr;
  vk::raii::Device device = nullptr;

  vk::raii::Queue graphicsQueue = nullptr;
  vk::raii::Queue presentQueue = nullptr;

  vk::raii::SwapchainKHR swapChain = nullptr;
  std::vector<vk::Image> swapChainImages;
  vk::Format swapChainImageFormat = vk::Format::eUndefined;
  vk::Extent2D swapChainExtent;

  std::vector<vk::raii::ImageView> swapChainImageViews;

  std::vector<const char *> requiredDeviceExtension = { vk::KHRSwapchainExtensionName,
    vk::KHRSpirv14ExtensionName,
    vk::KHRSynchronization2ExtensionName,
    vk::KHRCreateRenderpass2ExtensionName };
};

int main()
{
  HelloVulkan app{};

  try {
    app.Run();
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << '\n';
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}