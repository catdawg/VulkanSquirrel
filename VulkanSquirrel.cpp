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

  GLFWwindow* window = nullptr;
  std::vector<VkExtensionProperties> extensions;
  VkInstance instance = VK_NULL_HANDLE;
  VkDebugReportCallbackEXT callbackDebugInstance = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  VkQueue mainQueue = VK_NULL_HANDLE;
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

  createInfo.enabledExtensionCount = (uint32_t)extensions.size();
  createInfo.ppEnabledExtensionNames = extensions.data();


  if (data.options.vulkanValidationLayersMode == kEnabledVulkanValidationLayers && enableValidationLayersAfterCheck) {
    createInfo.enabledLayerCount = (uint32_t)data.options.vulkanValidationLayers.size();
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

bool isVKDeviceSuitable(const VkPhysicalDevice &device, const VkSurfaceKHR &surface) {

  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(device, &deviceProperties);

  //unused for now
  //VkPhysicalDeviceFeatures deviceFeatures;
  //vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

  if (deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
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
    if (isVKDeviceSuitable(device, data.surface)) {
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

  int queueFamily = findSuitableVKQueue(data.physicalDevice, data.surface);
  VkDeviceQueueCreateInfo queueCreateInfo = {};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = queueFamily;
  queueCreateInfo.queueCount = 1;

  float queuePriority = 1.0f;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  VkPhysicalDeviceFeatures deviceFeatures = {}; // empty for now

  VkDeviceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  createInfo.pQueueCreateInfos = &queueCreateInfo;
  createInfo.queueCreateInfoCount = 1;

  createInfo.pEnabledFeatures = &deviceFeatures;

  createInfo.enabledExtensionCount = 0;

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

  vkGetDeviceQueue(data.device, queueFamily, 0, &data.mainQueue);

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
      }
    }
  );

  // THE LOOP!
  while (!glfwWindowShouldClose(data.window)) {
    glfwPollEvents();
  } // the loop

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
