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

  GLFWwindow* window;
  std::vector<VkExtensionProperties> extensions;
  VkInstance instance;
  VkDebugReportCallbackEXT callbackDebugInstance;
  VkPhysicalDevice physicalDevice;
  VkDevice device;
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

  auto enabledExtensions = std::make_unique<std::vector<VkExtensionProperties>>();

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
      enabledExtensions->push_back(*availableExtension);
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

  if (vkCreateInstance(&createInfo, nullptr, &data.instance) != VK_SUCCESS || data.instance == VK_NULL_HANDLE) {
    return {
      false,
      kVKFailedToCreateInstance,
      "Failed to create Vulkan instance"
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

  VkDebugReportCallbackEXT callbackDebugInstance = VK_NULL_HANDLE;
  if (CreateVkDebugReportCallbackEXT(data.instance, &createInfo, nullptr, &data.callbackDebugInstance) != VK_SUCCESS) {

    std::cerr << "Failed to set up vulkan debug callback!" << std::endl;
    return tsk::kTaskSuccess; // failing here is not critical
  }

  return tsk::kTaskSuccess;
}

bool isVKDeviceSuitable(const VkPhysicalDevice &device) {

  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(device, &deviceProperties);

  //unused for now
  //VkPhysicalDeviceFeatures deviceFeatures;
  //vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

  if (deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
    return false;
  }

  std::vector<VkQueueFamilyProperties> queueFamilies = GetVkFamiliesOfDevice(device);

  bool foundQueueWithGraphicsCapability = false;
  for (const auto& queueFamily : queueFamilies) {
    if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      foundQueueWithGraphicsCapability = true;
      break;
    }
  }

  return foundQueueWithGraphicsCapability;
}

tsk::TaskResult taskPickVulkanPhysicalDevice(VulkanSquirrelData &data) {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(data.instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);

  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  vkEnumeratePhysicalDevices(data.instance, &deviceCount, devices.data());
  for (const auto& device : devices) {
    if (isVKDeviceSuitable(device)) {
      data.physicalDevice = device;
      break;
    }
  }

  if (data.physicalDevice == VK_NULL_HANDLE) {
    return {
      false,
      kGLFWWindowCouldNotBeCreated,
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

  std::vector<VkQueueFamilyProperties> queueFamilies = GetVkFamiliesOfDevice(data.physicalDevice);

  int indexOfGraphicsFamily = -1;
  for (int i = 0; i < queueFamilies.size(); ++i) {
    if (queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indexOfGraphicsFamily = i;
      break;
    }
  }

  if (indexOfGraphicsFamily == -1) {
    return {
      false,
      kVKNoCompatibleGPUAvailable,
      "Failed to find a suitable GPU!"
    };
  }

  VkDeviceQueueCreateInfo queueCreateInfo = {};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = indexOfGraphicsFamily;
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

  if (vkCreateDevice(data.physicalDevice, &createInfo, nullptr, &data.device) != VK_SUCCESS || data.device == VK_NULL_HANDLE) {

    return {
      false,
      kVKFailedToCreateVulkanLogicalDevice,
      "Failed to create Vulkan logical device!"
    };
  }

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

  if (data.instance != VK_NULL_HANDLE) {
    vkDestroyInstance(data.instance, nullptr);
  }

  if (data.window != nullptr) {
    glfwDestroyWindow(data.window);
  }

  glfwTerminate();
}

} // namespace VKS
