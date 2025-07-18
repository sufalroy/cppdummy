#include <iostream>
#include <stdexcept>
#include <cassert>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <memory>
#include <algorithm>
#include <span>
#include <string_view>
#include <array>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

constexpr std::uint32_t WIDTH = 800;
constexpr std::uint32_t HEIGHT = 600;

constexpr std::array VALIDATION_LAYERS = {
  "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
constexpr bool ENABLE_VALIDATION_LAYERS = false;
#else
constexpr bool ENABLE_VALIDATION_LAYERS = true;
#endif

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
    
    std::vector<char const*> requiredLayers;
    if (ENABLE_VALIDATION_LAYERS) {
      requiredLayers.assign(VALIDATION_LAYERS.begin(), VALIDATION_LAYERS.end());
    }

    auto layerProperties = context.enumerateInstanceLayerProperties();
    for (auto const& requiredLayer : requiredLayers)
    {
      if (std::ranges::none_of(layerProperties, [requiredLayer](auto const& layerProperty) {
         return strcmp(layerProperty.layerName, requiredLayer) == 0; 
      })) {
        throw std::runtime_error("Required layer not supported: " + std::string(requiredLayer));
      }
    }

    auto requiredExtensions = GetRequiredExtensions();

    auto extensionProperties = context.enumerateInstanceExtensionProperties();
    for (auto const& requiredExtension : requiredExtensions)
    {
      if (std::ranges::none_of(extensionProperties,
        [requiredExtension](auto const& extensionProperty)
        { return strcmp(extensionProperty.extensionName, requiredExtension) == 0; }))
      {
        throw std::runtime_error("Required extension not supported: " + std::string(requiredExtension));
      }
    }

    vk::InstanceCreateInfo createInfo{};
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = static_cast<std::uint32_t>(requiredLayers.size());
    createInfo.ppEnabledLayerNames = requiredLayers.data();
    createInfo.enabledExtensionCount = static_cast<std::uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    instance = vk::raii::Instance(context, createInfo);
  }

  void SetupDebugMessenger()
  {
    if (!ENABLE_VALIDATION_LAYERS) {
      return;
    }

    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
    );
    
    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
      vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
      vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
      vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
    );
    
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{};
    debugUtilsMessengerCreateInfoEXT.messageSeverity = severityFlags;
    debugUtilsMessengerCreateInfoEXT.messageType = messageTypeFlags;
    debugUtilsMessengerCreateInfoEXT.pfnUserCallback = &DebugCallback;

    debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
  }

  static std::vector<const char*> GetRequiredExtensions()
  {
    std::uint32_t glfwExtensionCount = 0;
    auto const* glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions;
    extensions.reserve(glfwExtensionCount + (ENABLE_VALIDATION_LAYERS ? 1U : 0U));
    
    std::span<const char* const> extensionSpan(glfwExtensions, glfwExtensionCount);
    std::copy(extensionSpan.begin(), extensionSpan.end(), std::back_inserter(extensions));
    
    if (ENABLE_VALIDATION_LAYERS) {
      extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

    return extensions;
  }

  static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    vk::DebugUtilsMessageTypeFlagsEXT type,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* /*unused*/
  )
  {
    if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError || 
        severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
          std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;
    }

    return vk::False;
  }
  
  void PickPhysicalDevice() {
    std::vector<vk::raii::PhysicalDevice> devices = instance.enumeratePhysicalDevices();
    
    auto devIter = std::ranges::find_if(devices, [this](const vk::raii::PhysicalDevice& physDev) {
      const auto properties = physDev.getProperties();
      if (properties.apiVersion < VK_API_VERSION_1_3) {
        return false;
      }
      
      const auto queueFamilies = physDev.getQueueFamilyProperties();
      if (!std::ranges::any_of(queueFamilies, [](const auto& qfp) {
        return static_cast<bool>(qfp.queueFlags & vk::QueueFlagBits::eGraphics);
      })) {
        return false;
      }
      
      const auto availableExtensions = physDev.enumerateDeviceExtensionProperties();
      if (!std::ranges::all_of(requiredDeviceExtension, [&](const char* ext) {
        return std::ranges::any_of(availableExtensions, [ext](const auto& prop) {
          return strcmp(prop.extensionName, ext) == 0;
        });
      })) {
        return false;
      }
      
      const auto features = physDev.getFeatures2<vk::PhysicalDeviceFeatures2,
                                                vk::PhysicalDeviceVulkan13Features,
                                                vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
                                                
      return (features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering != 0U) &&
            (features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState != 0U);
    });
    
    if (devIter == devices.end()) {
      throw std::runtime_error("failed to find a suitable GPU!");
    }
    
    physicalDevice = *devIter;
  }
  
  void CreateLogicalDevice() {
    const auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
    
    const auto graphicsQueueFamilyProperty = std::ranges::find_if(queueFamilyProperties, [](const auto& qfp) {
      return static_cast<bool>(qfp.queueFlags & vk::QueueFlagBits::eGraphics);
    });

    if (graphicsQueueFamilyProperty == queueFamilyProperties.end()) {
        throw std::runtime_error("No graphics queue family found!");
    }

    const auto graphicsIndex = static_cast<std::uint32_t>(
        std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty));

    vk::PhysicalDeviceFeatures2 features2{};
    vk::PhysicalDeviceVulkan13Features vulkan13Features{};
    vulkan13Features.dynamicRendering = VK_TRUE;
    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures{};
    extendedDynamicStateFeatures.extendedDynamicState = VK_TRUE;

    features2.pNext = &vulkan13Features;
    vulkan13Features.pNext = &extendedDynamicStateFeatures;

    constexpr float queuePriority = 1.0F;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{};
    deviceQueueCreateInfo.queueFamilyIndex = graphicsIndex;
    deviceQueueCreateInfo.queueCount = 1;
    deviceQueueCreateInfo.pQueuePriorities = &queuePriority;

    vk::DeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.pNext = &features2;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = static_cast<std::uint32_t>(requiredDeviceExtension.size());
    deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtension.data();

    device = vk::raii::Device(physicalDevice, deviceCreateInfo);
    graphicsQueue = vk::raii::Queue(device, graphicsIndex, 0);
  }

  void InitVulkan() 
  { 
    CreateInstance(); 
    SetupDebugMessenger();
    PickPhysicalDevice();
    CreateLogicalDevice();
  }

  void MainLoop() const
  {
    while (glfwWindowShouldClose(window) == GLFW_FALSE) { 
      glfwPollEvents(); 
    }
  }

  void Cleanup() const
  {
    glfwDestroyWindow(window);
    glfwTerminate();
  }

  GLFWwindow* window = nullptr;

  vk::raii::Context context;
  vk::raii::Instance instance = nullptr;
  vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;

  vk::raii::PhysicalDevice physicalDevice = nullptr;
  vk::raii::Device device = nullptr;

  vk::raii::Queue graphicsQueue = nullptr;

  std::vector<const char*> requiredDeviceExtension = {
     vk::KHRSwapchainExtensionName,
     vk::KHRSpirv14ExtensionName,
     vk::KHRSynchronization2ExtensionName,
     vk::KHRCreateRenderpass2ExtensionName
  };
};

int main()
{
  HelloVulkan app{};

  try {
    app.Run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}