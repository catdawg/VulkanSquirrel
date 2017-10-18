#include "VulkanSquirrel.h"

#include <iostream>
#include <functional>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>

#include "TaskSequence.h"
#include "VulkanUtils.h"

namespace vks {

struct VulkanSquirrelData {
  VulkanSquirrelOptions options;

  // created by taskInitGLFWWindow
  GLFWwindow* window = nullptr;

  // created by taskCheckVulkanExtensions
  std::vector<VkExtensionProperties> extensions;

  // created by taskInitVulkanInstance
  VkInstance instance = VK_NULL_HANDLE;

  // created by taskInitVulkanDebug
  VkDebugReportCallbackEXT callbackDebugInstance = VK_NULL_HANDLE;

  // created by taskCreateVulkanSurface
  VkSurfaceKHR surface = VK_NULL_HANDLE;

  // created by taskCheckSurfaceCapabilities
  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  std::vector<VkSurfaceFormatKHR> surfaceFormats;
  std::vector<VkPresentModeKHR> surfacePresentModes;

  // created by taskPickVulkanPhysicalDevice
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

  // created by taskCreateVulkanLogicalDevice
  VkDevice device = VK_NULL_HANDLE;
  uint32_t queueFamilyIndex;
  VkQueue mainQueue = VK_NULL_HANDLE;

  // created by taskCheckVulkanSurfaceCapabilities
  VkSurfaceFormatKHR surfaceFormat;;
  VkPresentModeKHR presentMode;;
  VkExtent2D extent;

  // created by taskCreateVulkanSwapChain
  VkSwapchainKHR swapChain;
  std::vector<VkImage> swapChainImages;
  VkExtent2D swapChainExtent;
};

tsk::TaskResult taskInitGLFWWindow(
  VulkanSquirrelData &data
) {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  data.window = glfwCreateWindow(data.options.windowWidth, data.options.windowHeight, "VulkanSquirrel", nullptr, nullptr);

  if (data.window == nullptr) {
    return {
      false,
      kGLFWWindowCouldNotBeCreated,
      "Failed to create GLFW window"
    };
  }

  return tsk::kTaskSuccess;
}

tsk::TaskResult taskCheckVulkanExtensions(
  VulkanSquirrelData &data
) {
  std::vector<VkExtensionProperties> availableExtensions = GetVkExtensions();

  std::cout << "available extensions:" << std::endl;

  for (const auto& availableExtension : availableExtensions) {
    std::cout << "\t" << availableExtension.extensionName << std::endl;
  }

  unsigned int glfwExtensionCount = 0;
  const char** glfwExtensions;

  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::cout << "enabling extensions:" << std::endl;

  const auto checkAndAddExtension = [&](const char* neededExtension) -> bool {
    const auto availableExtension = FindVkExtension(availableExtensions, neededExtension);
    if (availableExtension == nullptr) {
      return false;
    }
    else {
      const char* extensionName = (*availableExtension).extensionName;
      std::cout << "\t" << extensionName << std::endl;
      data.extensions.push_back(*availableExtension);
      return true;
    }
  };

  for (unsigned int i = 0; i < glfwExtensionCount; ++i) {
    if (!checkAndAddExtension(glfwExtensions[i])) {
      std::stringstream errorStringStream;
      errorStringStream << "Vulkan GLFW Required Extension missing" << glfwExtensions[i];
      return {
        false,
        kVKRequiredExtensionNotAvailable,
        errorStringStream.str()
      };
    }
  }

  if (data.options.vulkanValidationLayersMode == kEnabledVulkanValidationLayers) {
    if (!checkAndAddExtension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME)) {
      std::stringstream errorStringStream;
      errorStringStream << "Vulkan Debug Extension missing" << VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
      std::cerr << errorStringStream.str() << std::endl;
    }
  }

  return tsk::kTaskSuccess;
}

tsk::TaskResult taskInitVulkanInstance(
    VulkanSquirrelData &data
) {
  bool enableValidationLayersAfterCheck = true;
  if (
      data.options.vulkanValidationLayersMode == kEnabledVulkanValidationLayers && 
      !CheckVkValidationLayerSupport(data.options.vulkanValidationLayers)
    ) {
    std::cerr << "Vulkan validation layer requested, but not available!";
    enableValidationLayersAfterCheck = false;
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

  std::vector<const char*> extensions(data.extensions.size());

  for (int i = 0; i < data.extensions.size(); ++i) {
    extensions[i] = data.extensions[i].extensionName;
  }

  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();


  if (data.options.vulkanValidationLayersMode == kEnabledVulkanValidationLayers && enableValidationLayersAfterCheck) {
    createInfo.enabledLayerCount = static_cast<uint32_t>(data.options.vulkanValidationLayers.size());
    createInfo.ppEnabledLayerNames = data.options.vulkanValidationLayers.data();
  }
  else {
    createInfo.enabledLayerCount = 0;
  }

  VkResult result;
  if ((result = vkCreateInstance(&createInfo, nullptr, &data.instance)) != VK_SUCCESS || data.instance == VK_NULL_HANDLE) {

    std::stringstream errorStringStream;
    errorStringStream << "Failed to create Vulkan instance with vk error code:" << result;
    return {
      false,
      kVKFailedToCreateInstance,
      errorStringStream.str()
    };
  }
  return tsk::kTaskSuccess;
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

tsk::TaskResult taskInitVulkanDebug(VulkanSquirrelData &data) {

  if (data.options.vulkanValidationLayersMode == kNoVulkanValidationLayers) return tsk::kTaskSuccess;

  VkDebugReportCallbackCreateInfoEXT createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
  createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
  createInfo.pfnCallback = VKDebugOutputCallback;

  VkResult result;
  if ((result = CreateVkDebugReportCallbackEXT(data.instance, &createInfo, nullptr, &data.callbackDebugInstance)) != VK_SUCCESS) {

    std::cerr << "Failed to set up vulkan debug callback with vk error code:" << result << std::endl;
    return tsk::kTaskSuccess; // failing here is not critical
  }

  return tsk::kTaskSuccess;
}

tsk::TaskResult taskCreateVulkanSurface(VulkanSquirrelData &data) {

  VkResult result;
  if ((result = glfwCreateWindowSurface(data.instance, data.window, nullptr, &data.surface)) != VK_SUCCESS) {

    std::stringstream errorStringStream;
    errorStringStream << "Failed to create Vulkan surface with vk error code: " << result;
    return {
      false,
      kVKFailedToCreateSurface,
      errorStringStream.str()
    };
  }

  return tsk::kTaskSuccess;
}

int findSuitableVKQueue(const VkPhysicalDevice &device, const VkSurfaceKHR &surface) {

  std::vector<VkQueueFamilyProperties> queueFamilies = GetVkFamiliesOfDevice(device);

  bool foundQueueWithGraphicsCapabilityAndPresentation = false;
  for (int i = 0; i < queueFamilies.size(); ++i) {
    const auto& queueFamily = queueFamilies[i];

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
    if (presentSupport && queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {

      return i;
    }
  }

  return -1;
}

bool isVKDeviceSuitable(const VkPhysicalDevice &device, const VkSurfaceKHR &surface, const std::vector<const char*> &extensions) {

  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(device, &deviceProperties);

  //unused for now
  //VkPhysicalDeviceFeatures deviceFeatures;
  //vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

  if (deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
    return false;
  }

  if (!CheckVkExtensionSupport(device, extensions)) {
    return false;
  }

  return findSuitableVKQueue(device, surface) != -1;
}

tsk::TaskResult taskPickVulkanPhysicalDevice(VulkanSquirrelData &data) {

  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(data.instance, &deviceCount, nullptr);

  std::vector<VkPhysicalDevice> devices(deviceCount);

  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  vkEnumeratePhysicalDevices(data.instance, &deviceCount, devices.data());
  for (const auto& device : devices) {
    if (isVKDeviceSuitable(device, data.surface, data.options.vulkanExtensions)) {
      data.physicalDevice = device;
      break;
    }
  }

  if (data.physicalDevice == VK_NULL_HANDLE) {
    return {
      false,
      kVKFailedToCreateVulkanPhysicalDevice,
      "Failed to find a suitable GPU!"
    };
  }

  return tsk::kTaskSuccess;
}

tsk::TaskResult taskCreateVulkanLogicalDevice(VulkanSquirrelData &data) {

  bool enableValidationLayersAfterCheck = false;
  if (data.options.vulkanValidationLayersMode == kEnabledVulkanValidationLayers && !CheckVkValidationLayerSupport(data.options.vulkanValidationLayers)) {
    std::cerr << "vulkan validation layers requested, but not available!" << std::endl;

    enableValidationLayersAfterCheck = true;
  }

  data.queueFamilyIndex = findSuitableVKQueue(data.physicalDevice, data.surface);
  VkDeviceQueueCreateInfo queueCreateInfo = {};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = data.queueFamilyIndex;
  queueCreateInfo.queueCount = 1;

  float queuePriority = 1.0f;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  VkPhysicalDeviceFeatures deviceFeatures = {}; // empty for now

  VkDeviceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  createInfo.pQueueCreateInfos = &queueCreateInfo;
  createInfo.queueCreateInfoCount = 1;

  createInfo.pEnabledFeatures = &deviceFeatures;

  createInfo.enabledExtensionCount = static_cast<uint32_t>(data.options.vulkanExtensions.size());
  createInfo.ppEnabledExtensionNames = data.options.vulkanExtensions.data();

  if (enableValidationLayersAfterCheck && data.options.vulkanValidationLayersMode == kEnabledVulkanValidationLayers) {
    createInfo.enabledLayerCount = static_cast<uint32_t>(data.options.vulkanValidationLayers.size());
    createInfo.ppEnabledLayerNames = data.options.vulkanValidationLayers.data();
  }
  else {
    createInfo.enabledLayerCount = 0;
  }

  VkResult result;
  if ((result = vkCreateDevice(data.physicalDevice, &createInfo, nullptr, &data.device)) != VK_SUCCESS || data.device == VK_NULL_HANDLE) {

    return {
      false,
      kVKFailedToCreateVulkanLogicalDevice,
      "Failed to create Vulkan logical device!"
    };
  }

  vkGetDeviceQueue(data.device, data.queueFamilyIndex, 0, &data.mainQueue);

  return tsk::kTaskSuccess;
}

tsk::TaskResult taskCheckVulkanSurfaceCapabilities(VulkanSquirrelData &data) {

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(data.physicalDevice, data.surface, &data.surfaceCapabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(data.physicalDevice, data.surface, &formatCount, nullptr);

  if (formatCount != 0) {
    data.surfaceFormats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(data.physicalDevice, data.surface, &formatCount, data.surfaceFormats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(data.physicalDevice, data.surface, &presentModeCount, nullptr);

  if (presentModeCount != 0) {
    data.surfacePresentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(data.physicalDevice, data.surface, &presentModeCount, data.surfacePresentModes.data());
  }

  return tsk::kTaskSuccess;
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {
  VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

  for (const auto& availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return availablePresentMode;
    }
    else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
      bestMode = availablePresentMode;
    }
  }

  return bestMode;
}

VkExtent2D chooseSwapExtent(const uint32_t &width, const uint32_t &height, const VkSurfaceCapabilitiesKHR& capabilities) {
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  }
  else {
    VkExtent2D actualExtent = { width, height };

    actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
    actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

    return actualExtent;
  }
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
  if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
    return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
  }

  for (const auto& availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }

  return availableFormats[0];
}

tsk::TaskResult taskCreateVulkanSwapChain(VulkanSquirrelData &data) {

  data.surfaceFormat = chooseSwapSurfaceFormat(data.surfaceFormats);
  data.presentMode = chooseSwapPresentMode(data.surfacePresentModes);
  data.swapChainExtent = chooseSwapExtent(data.options.windowWidth, data.options.windowHeight, data.surfaceCapabilities);

  // we try one more than minimum to implement triple buffering
  uint32_t imageCount = data.surfaceCapabilities.minImageCount + 1;
  if (data.surfaceCapabilities.maxImageCount > 0 && imageCount > data.surfaceCapabilities.maxImageCount) {
    imageCount = data.surfaceCapabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = data.surface;

  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = data.surfaceFormat.format;
  createInfo.imageColorSpace = data.surfaceFormat.colorSpace;
  createInfo.imageExtent = data.swapChainExtent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  createInfo.pQueueFamilyIndices = &data.queueFamilyIndex;
  createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

  createInfo.preTransform = data.surfaceCapabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = data.presentMode;
  createInfo.clipped = VK_TRUE;

  createInfo.oldSwapchain = VK_NULL_HANDLE;

  VkResult result;
  if ((result = vkCreateSwapchainKHR(data.device, &createInfo, nullptr, &data.swapChain)) != VK_SUCCESS) {

    std::stringstream errorStringStream;
    errorStringStream << "Failed to create Vulkan swap chain with vk error code: " << result;
    return {
      false,
      kVKFailedToCreateVulkanSwapChain,
      errorStringStream.str()
    };
  }

  vkGetSwapchainImagesKHR(data.device, data.swapChain, &imageCount, nullptr);
  data.swapChainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(data.device, data.swapChain, &imageCount, data.swapChainImages.data());

  return tsk::kTaskSuccess;
}

void VulkanSquirrel::Run(const VulkanSquirrelOptions &options) {

  VulkanSquirrelData data;

  data.options = options;

  tsk::TaskSequenceResult result = tsk::ExecuteTaskSequence<VulkanSquirrelData>(
    data,
    "Initialize GLFW and Vulkan",
    {
      {
        "Initialize GLFW Window",
        taskInitGLFWWindow
      }, {
        "Check Vulkan Extensions",
        taskCheckVulkanExtensions
      }, {
        "Initialize Vulkan Instance",
        taskInitVulkanInstance
      }, {
        "Initialize Vulkan Debug",
        taskInitVulkanDebug
      }, {
        "Create Vulkan Surface",
        taskCreateVulkanSurface
      },{
        "Pick Vulkan Physical Device",
        taskPickVulkanPhysicalDevice
      }, {
        "Create Vulkan Logical Device",
        taskCreateVulkanLogicalDevice
      }, {
        "Checking Vulkan Swap Chain Capabilities",
        taskCheckVulkanSurfaceCapabilities
      }, {
        "Create Vulkan Swap Chain",
        taskCreateVulkanSwapChain
      }
    }
  );

  // THE LOOP!
  while (!glfwWindowShouldClose(data.window)) {
    glfwPollEvents();
  } // the loop

  if (data.swapChain != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(data.device, data.swapChain, nullptr);
  }

  if (data.device != VK_NULL_HANDLE) {
    vkDestroyDevice(data.device, nullptr);
  }

  if (data.callbackDebugInstance != VK_NULL_HANDLE) {
    DestroyVkDebugReportCallbackEXT(data.instance, data.callbackDebugInstance, nullptr);
  }

  if (data.surface != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(data.instance, data.surface, nullptr);
  }

  if (data.instance != VK_NULL_HANDLE) {
    vkDestroyInstance(data.instance, nullptr);
  }

  if (data.window != nullptr) {
    glfwDestroyWindow(data.window);
  }

  glfwTerminate();
}

} // namespace VKS
