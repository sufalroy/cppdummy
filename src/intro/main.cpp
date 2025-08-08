#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
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

#include "vec.hpp"


constexpr std::uint32_t WIDTH = 800;
constexpr std::uint32_t HEIGHT = 600;

constexpr std::array VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

#ifdef NDEBUG
constexpr bool ENABLE_VALIDATION_LAYERS = false;
#else
constexpr bool ENABLE_VALIDATION_LAYERS = true;
#endif

struct QueueFamilyIndices
{
  std::uint32_t graphicsFamily = std::numeric_limits<uint32_t>::max();
  std::uint32_t presentFamily = std::numeric_limits<uint32_t>::max();

  [[nodiscard]] bool IsComplete() const
  {
    return graphicsFamily != std::numeric_limits<uint32_t>::max()
           && presentFamily != std::numeric_limits<uint32_t>::max();
  }

  [[nodiscard]] std::set<std::uint32_t> GetUniqueIndices() const { return { graphicsFamily, presentFamily }; }
};

struct SwapChainSupportDetails
{
  vk::SurfaceCapabilitiesKHR capabilities;
  std::vector<vk::SurfaceFormatKHR> formats;
  std::vector<vk::PresentModeKHR> presentModes;
};

struct ImageLayoutTransition
{
  vk::ImageLayout oldLayout;
  vk::ImageLayout newLayout;
  vk::AccessFlags2 srcAccessMask;
  vk::AccessFlags2 dstAccessMask;
  vk::PipelineStageFlags2 srcStageMask;
  vk::PipelineStageFlags2 dstStageMask;
};

struct FrameData
{
  vk::raii::Fence inFlightFence;
  vk::raii::CommandBuffer commandBuffer;

  FrameData(const vk::raii::Device &device, vk::raii::CommandBuffer &&cmdBuffer)
        : inFlightFence(device, vk::FenceCreateInfo{ vk::FenceCreateFlagBits::eSignaled }),
          commandBuffer(std::move(cmdBuffer))
    {}
};

struct Vertex 
{
  vec2 pos;
  vec3 color;

  static vk::VertexInputBindingDescription GetBindingDescription()
  {
    vk::VertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = vk::VertexInputRate::eVertex;
    return bindingDescription;
  }

  static std::array<vk::VertexInputAttributeDescription, 2> GetAttributeDescriptions()
  {
    std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = vk::Format::eR32G32Sfloat;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    return attributeDescriptions;
  }
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
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Hello Vulkan", nullptr, nullptr);
    if (window == nullptr) {
      glfwTerminate();
      throw std::runtime_error("Failed to create GLFW window");
    }

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);
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

    const auto &vulkan13Features = features.get<vk::PhysicalDeviceVulkan13Features>();

    return (vulkan13Features.dynamicRendering != 0U) && (vulkan13Features.synchronization2 != 0U)
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
    vulkan13Features.synchronization2 = VK_TRUE;

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
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) { return capabilities.currentExtent; }

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

  void CreateGraphicsPipeline()
  {
    auto shaderCode = ReadFile("/home/dev/Laboratory/cppdummy/src/intro/shaders/slang.spv");

    const vk::raii::ShaderModule shaderModule = CreateShaderModule(shaderCode);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = shaderModule;
    vertShaderStageInfo.pName = "vertMain";

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = shaderModule;
    fragShaderStageInfo.pName = "fragMain";

    std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = { vertShaderStageInfo, fragShaderStageInfo };

    auto bindingDescription = Vertex::GetBindingDescription();
    auto attributeDescriptions = Vertex::GetAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;

    vk::PipelineViewportStateCreateInfo viewportState{};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eClockwise;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasSlopeFactor = 1.0F;
    rasterizer.lineWidth = 1.0F;

    vk::PipelineMultisampleStateCreateInfo multisampling{};
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisampling.sampleShadingEnable = VK_FALSE;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                                          | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = VK_FALSE;

    vk::PipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = vk::LogicOp::eCopy;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::vector dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

    vk::PipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.dynamicStateCount = static_cast<std::uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    const vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
    pipelineRenderingCreateInfo.colorAttachmentCount = 1;
    pipelineRenderingCreateInfo.pColorAttachmentFormats = &swapChainImageFormat;

    vk::GraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.pNext = &pipelineRenderingCreateInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = *pipelineLayout;
    pipelineInfo.renderPass = nullptr;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
  }

  [[nodiscard]] vk::raii::ShaderModule CreateShaderModule(const std::vector<char> &code) const
  {
    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const std::uint32_t *>(code.data());
    return { device, createInfo };
  }

  static std::vector<char> ReadFile(const std::string_view &filename)
  {
    std::ifstream file(std::string(filename), std::ios::ate | std::ios::binary);
    if (!file.is_open()) { throw std::runtime_error("Failed to open file: " + std::string(filename)); }
    const std::size_t size = static_cast<std::size_t>(file.tellg());
    std::vector<char> buffer(size);
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(size));
    return buffer;
  }

  void CreateCommandPool()
  {
    const QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);
    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.queueFamilyIndex = indices.graphicsFamily;
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    commandPool = vk::raii::CommandPool(device, poolInfo);
  }

  [[nodiscard]] static vk::raii::CommandBuffer CreateCommandBuffer(const vk::raii::Device &device,
    const vk::raii::CommandPool &commandPool)
  {
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.commandPool = *commandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = 1;

    auto commandBuffers = vk::raii::CommandBuffers(device, allocInfo);
    return std::move(commandBuffers[0]);
  }

  static void
    TransitionImageLayout(vk::CommandBuffer cmdBuffer, vk::Image image, const ImageLayoutTransition &transition)
  {
    vk::ImageMemoryBarrier2 barrier{};
    barrier.srcStageMask = transition.srcStageMask;
    barrier.srcAccessMask = transition.srcAccessMask;
    barrier.dstStageMask = transition.dstStageMask;
    barrier.dstAccessMask = transition.dstAccessMask;
    barrier.oldLayout = transition.oldLayout;
    barrier.newLayout = transition.newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    vk::DependencyInfo dependencyInfo{};
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &barrier;
    cmdBuffer.pipelineBarrier2(dependencyInfo);
  }

  void RecordCommandBuffer(vk::CommandBuffer cmdBuffer, std::uint32_t imageIndex)
  {
    const vk::CommandBufferBeginInfo beginInfo{};
    cmdBuffer.begin(beginInfo);

    TransitionImageLayout(cmdBuffer,
      swapChainImages[imageIndex],
      { .oldLayout = vk::ImageLayout::eUndefined,
        .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .srcAccessMask = vk::AccessFlags2{},
        .dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
        .srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe,
        .dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput });

    const vk::ClearValue clearColor = vk::ClearColorValue(0.0F, 0.0F, 0.0F, 1.0F);
    vk::RenderingAttachmentInfo attachmentInfo{};
    attachmentInfo.imageView = *swapChainImageViews[imageIndex];
    attachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    attachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
    attachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
    attachmentInfo.clearValue = clearColor;

    vk::RenderingInfo renderingInfo{};
    renderingInfo.renderArea.offset = vk::Offset2D(0, 0);
    renderingInfo.renderArea.extent = swapChainExtent;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &attachmentInfo;

    cmdBuffer.beginRendering(renderingInfo);
    cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
    cmdBuffer.bindVertexBuffers(0, *vertexBuffer, { 0 });

    const vk::Viewport viewport(
      0.0F, 0.0F, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0F, 1.0F);
    cmdBuffer.setViewport(0, viewport);

    const vk::Rect2D scissor(vk::Offset2D(0, 0), swapChainExtent);
    cmdBuffer.setScissor(0, scissor);

    cmdBuffer.draw(static_cast<std::uint32_t>(vertices.size()), 1, 0, 0);
    cmdBuffer.endRendering();

    TransitionImageLayout(cmdBuffer,
      swapChainImages[imageIndex],
      { .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .newLayout = vk::ImageLayout::ePresentSrcKHR,
        .srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
        .dstAccessMask = vk::AccessFlags2{},
        .srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe });

    cmdBuffer.end();
  }
  
  void CreateBuffer(vk::DeviceSize size, 
                    vk::BufferUsageFlags usage, 
                    vk::MemoryPropertyFlags properties,
                    vk::raii::Buffer& buffer,
                    vk::raii::DeviceMemory& bufferMemory)
  {
    vk::BufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = usage;
    bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

    buffer = vk::raii::Buffer(device, bufferCreateInfo);

    const vk::MemoryRequirements memoryRequirements = buffer.getMemoryRequirements();
    
    vk::MemoryAllocateInfo memoryAllocateInfo{};
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, properties);

    bufferMemory = vk::raii::DeviceMemory(device, memoryAllocateInfo);
    buffer.bindMemory(*bufferMemory, 0);
  }
  
  void CreateVertexBuffer()
  {
    const vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    
    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingBufferMemory = nullptr;
    CreateBuffer(bufferSize, 
                 vk::BufferUsageFlagBits::eTransferSrc, 
                 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                 stagingBuffer, 
                 stagingBufferMemory);
    
    void* dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
    std::memcpy(dataStaging, vertices.data(), static_cast<std::size_t>(bufferSize));
    stagingBufferMemory.unmapMemory();
    
    CreateBuffer(bufferSize,
                 vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
                 vk::MemoryPropertyFlagBits::eDeviceLocal,
                 vertexBuffer,
                 vertexBufferMemory);
                 
    CopyBuffer(stagingBuffer, vertexBuffer, bufferSize);
  }

  void CopyBuffer(const vk::raii::Buffer& srcBuffer, 
                  const vk::raii::Buffer& dstBuffer, 
                  vk::DeviceSize size) 
  {
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.commandPool = *commandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = 1;
    
    auto commandBuffers = device.allocateCommandBuffers(allocInfo);
    vk::raii::CommandBuffer commandCopyBuffer = std::move(commandBuffers[0]);
    
    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    
    commandCopyBuffer.begin(beginInfo);
    
    vk::BufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    
    commandCopyBuffer.copyBuffer(*srcBuffer, *dstBuffer, copyRegion);
    commandCopyBuffer.end();
    
    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &(*commandCopyBuffer);
    
    graphicsQueue.submit(submitInfo, nullptr);
    graphicsQueue.waitIdle();
  }

  [[nodiscard]] std::uint32_t FindMemoryType(std::uint32_t typeFilter, vk::MemoryPropertyFlags properties)
  {
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();
    for (std::uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
      if (((typeFilter & (1U << i)) != 0U) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
        return i;
      }
    }
    throw std::runtime_error("Failed to find suitable memory type");
  }
  
  void CreateSyncObjects()
  {
    if (frames.empty()) {
      frames.reserve(MAX_FRAMES_IN_FLIGHT);
      for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        auto commandBuffer = CreateCommandBuffer(device, commandPool);
        frames.emplace_back(device, std::move(commandBuffer));
      }
    }

    imageAvailableSemaphores.clear();
    renderFinishedSemaphores.clear();
    for (size_t i = 0; i < swapChainImages.size(); ++i) {
      imageAvailableSemaphores.emplace_back(device, vk::SemaphoreCreateInfo{});
      renderFinishedSemaphores.emplace_back(device, vk::SemaphoreCreateInfo{});
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
    CreateGraphicsPipeline();
    CreateCommandPool();
    CreateVertexBuffer();
    CreateSyncObjects();
  }

  void MainLoop()
  {
    while (glfwWindowShouldClose(window) == GLFW_FALSE) {
      glfwPollEvents();

      int width = 0;
      int height = 0;
      glfwGetFramebufferSize(window, &width, &height);
      if (width == 0 || height == 0) {
        glfwWaitEvents();
        continue;
      }

      DrawFrame();
    }

    device.waitIdle();
  }

  void DrawFrame()
  {
    auto &frame = frames[currentFrame];

    vk::Result fenceResult = vk::Result::eTimeout;
    while (fenceResult == vk::Result::eTimeout) {
      fenceResult = device.waitForFences(*frame.inFlightFence, VK_TRUE, std::numeric_limits<std::uint64_t>::max());
    }

    if (fenceResult != vk::Result::eSuccess) { throw std::runtime_error("Failed to wait for fence"); }

    auto acquireResult = swapChain.acquireNextImage(
      std::numeric_limits<std::uint64_t>::max(), *imageAvailableSemaphores[semaphoreIndex], nullptr);

    if (acquireResult.first == vk::Result::eErrorOutOfDateKHR) {
        RecreateSwapChain();
        return;
    }

    if (acquireResult.first != vk::Result::eSuccess && acquireResult.first != vk::Result::eSuboptimalKHR) {
      throw std::runtime_error("Failed to acquire swap chain image");
    }

    const std::uint32_t imageIndex = acquireResult.second;

    device.resetFences(*frame.inFlightFence);
    frame.commandBuffer.reset();
    RecordCommandBuffer(*frame.commandBuffer, imageIndex);

    const vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    vk::SubmitInfo submitInfo{};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &(*imageAvailableSemaphores[semaphoreIndex]);
    submitInfo.pWaitDstStageMask = &waitDestinationStageMask;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &(*frame.commandBuffer);
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &(*renderFinishedSemaphores[imageIndex]);
    graphicsQueue.submit(submitInfo, *frame.inFlightFence);

    vk::PresentInfoKHR presentInfoKHR{};
    presentInfoKHR.waitSemaphoreCount = 1;
    presentInfoKHR.pWaitSemaphores = &(*renderFinishedSemaphores[imageIndex]);
    presentInfoKHR.swapchainCount = 1;
    presentInfoKHR.pSwapchains = &(*swapChain);
    presentInfoKHR.pImageIndices = &imageIndex;
    auto presentResult = presentQueue.presentKHR(presentInfoKHR);

    if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR || framebufferResized) {
        framebufferResized = false;
        RecreateSwapChain();
    } else if (presentResult != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to present swapchain image");
    }

    semaphoreIndex = static_cast<uint32_t>((semaphoreIndex + 1) % imageAvailableSemaphores.size());
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  void CleanupSwapChain()
  {
    if (device != nullptr) { device.waitIdle(); }

    swapChainImageViews.clear();
    swapChain = nullptr;
  }
  void RecreateSwapChain()
  {
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
      glfwGetFramebufferSize(window, &width, &height);
      glfwWaitEvents();
    }

    device.waitIdle();

    for (auto &frame : frames) {
      const vk::Result fenceResult =
        device.waitForFences(*frame.inFlightFence, VK_TRUE, std::numeric_limits<std::uint64_t>::max());
      if (fenceResult != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to wait for frame fence during swapchain recreation");
      }
    }

    CleanupSwapChain();
    CreateSwapChain();
    CreateImageViews();
    CreateSyncObjects();
  }

  static void FramebufferResizeCallback(GLFWwindow *window, int /*width*/, int /*height*/)
  {
    auto *app = reinterpret_cast<HelloVulkan *>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
  }

  void Cleanup()
  {
    if (device != nullptr) { device.waitIdle(); }

    frames.clear();

    CleanupSwapChain();

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

  vk::raii::PipelineLayout pipelineLayout = nullptr;
  vk::raii::Pipeline graphicsPipeline = nullptr;

  vk::raii::Buffer vertexBuffer = nullptr;
  vk::raii::DeviceMemory vertexBufferMemory = nullptr;
  
  static constexpr float VERTEX_HALF = 0.5F;
  std::vector<Vertex> vertices = {
    { { 0.0F, -VERTEX_HALF }, { 1.0F, 0.0F, 0.0F } },
    { { VERTEX_HALF, VERTEX_HALF }, { 0.0F, 1.0F, 0.0F } },
    { { -VERTEX_HALF, VERTEX_HALF }, { 0.0F, 0.0F, 1.0F } }
  };

  vk::raii::CommandPool commandPool = nullptr;

  std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
  std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
  std::vector<FrameData> frames;
  std::uint32_t currentFrame = 0;
  std::uint32_t semaphoreIndex = 0;
  bool framebufferResized = false;

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
