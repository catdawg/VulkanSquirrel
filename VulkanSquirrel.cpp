#include "VulkanSquirrel.h"

#include <iostream>
#include <functional>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>

#include "VulkanUtils.h"

namespace vks {

namespace {
  
GLFWwindow* initGLFWWindow(int width, int height) {

  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  return glfwCreateWindow(width, height, "VulkanSquirrel", nullptr, nullptr);
}

void initVulkan(const VulkanValidationLayerMode &validationLayerMode, const std::vector<const char*> &validationLayers, VkInstance &instance) {

  if (validationLayerMode == kEnabledVulkanValidationLayers && !CheckVKValidationLayerSupport(validationLayers.data(), validationLayers.size())) {
    throw std::runtime_error("validation layers requested, but not available!");
  }

  std::vector<VkExtensionProperties> availableExtensions = GetVkExtensions();

  std::cout << "available extensions:" << std::endl;

  for (const auto& availableExtension : availableExtensions) {
    std::cout << "\t" << availableExtension.extensionName << std::endl;
  }

  std::vector<const char*> enabledExtensions = {};

  unsigned int glfwExtensionCount = 0;
  const char** glfwExtensions;

  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::cout << "enabling extensions:" << std::endl;

  const auto checkAndAddExtension = [&](const char* neededExtension) {
    const auto availableExtension = FindVKExtension(availableExtensions, neededExtension);
    if (availableExtension == nullptr) {
      std::stringstream errorStringStream;
      errorStringStream << "missing extension " << neededExtension;
      throw std::runtime_error(errorStringStream.str());
    }
    else {
      const char* extensionName = (*availableExtension).extensionName;
      std::cout << "\t" << extensionName << std::endl;
      enabledExtensions.push_back(extensionName);
    }
  };

  for (unsigned int i = 0; i < glfwExtensionCount; ++i) {
    checkAndAddExtension(glfwExtensions[i]);
  }

  if (validationLayerMode == kEnabledVulkanValidationLayers) {
    checkAndAddExtension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
  }

  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "VulkanSquirrel";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledLayerCount = 0;
  createInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
  createInfo.ppEnabledExtensionNames = enabledExtensions.data();

  if (validationLayerMode == kEnabledVulkanValidationLayers) {
    createInfo.enabledLayerCount = (uint32_t)validationLayers.size();
    createInfo.ppEnabledLayerNames = validationLayers.data();
  }
  else {
    createInfo.enabledLayerCount = 0;
  }
  if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
    throw std::runtime_error("failed to create instance!");
  }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL VKDebugOutputCallback(
  VkDebugReportFlagsEXT flags,
  VkDebugReportObjectTypeEXT objType,
  uint64_t obj,
  size_t location,
  int32_t code,
  const char* layerPrefix,
  const char* msg,
  void* userData) {

  std::cerr << "validation layer: " << msg << std::endl;

  return VK_FALSE;
}

void initVulkanDebug(const VulkanValidationLayerMode &validationLayerMode, const VkInstance &instance, VkDebugReportCallbackEXT &callbackDebugInstance) {
  if (validationLayerMode == kNoVulkanValidationLayers) return;

  VkDebugReportCallbackCreateInfoEXT createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
  createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
  createInfo.pfnCallback = VKDebugOutputCallback;

  if (CreateVKDebugReportCallbackEXT(instance, &createInfo, nullptr, &callbackDebugInstance) != VK_SUCCESS) {
    throw std::runtime_error("failed to set up debug callback!");
  }
}

void mainLoopWithGLFWWindow(GLFWwindow* glfwWindow) {
  while (!glfwWindowShouldClose(glfwWindow)) {
    glfwPollEvents();
  }
}

} // namespace

void VulkanSquirrel::Run(const VulkanSquirrelOptions &options) {

  GLFWwindow* window = initGLFWWindow(options.windowWidth, options.windowHeight);

  VkInstance instance;
  VkDebugReportCallbackEXT callbackDebugInstance;

  initVulkan(options.vulkanValidationLayersMode, options.vulkanValidationLayers, instance);
  initVulkanDebug(options.vulkanValidationLayersMode, instance, callbackDebugInstance);

  mainLoopWithGLFWWindow(window);
  
  DestroyVKDebugReportCallbackEXT(instance, callbackDebugInstance, nullptr);
  vkDestroyInstance(instance, nullptr);
  glfwDestroyWindow(window);
  glfwTerminate();
}

} // namespace VKS

