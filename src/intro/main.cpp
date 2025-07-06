#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

constexpr std::uint32_t WIDTH = 800;
constexpr std::uint32_t HEIGHT = 600;

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
    if (glfwInit() == GLFW_FALSE) { throw std::runtime_error("Failed to initialize GLFW"); }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Hello Vulkan", nullptr, nullptr);
  }

  void CreateInstance()
  {
    vk::ApplicationInfo appInfo{};
    appInfo.pApplicationName = "Hello Vulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = vk::ApiVersion14;

    std::uint32_t glfwExtensionCount = 0;
    auto const *glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    vk::InstanceCreateInfo createInfo{};
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;

    instance = std::make_unique<vk::raii::Instance>(context, createInfo);
  }

  void InitVulkan() { CreateInstance(); }

  void MainLoop() const
  {
    while (glfwWindowShouldClose(window) == GLFW_FALSE) { glfwPollEvents(); }
  }

  void Cleanup() const
  {
    glfwDestroyWindow(window);
    glfwTerminate();
  }

  GLFWwindow *window = nullptr;
  vk::raii::Context context;
  std::unique_ptr<vk::raii::Instance> instance = nullptr;
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