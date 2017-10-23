#include "VulkanSquirrel.h"

#include <iostream>
#include <fstream>
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
  uint32_t mainQueueFamilyIndex;
  VkQueue mainQueue = VK_NULL_HANDLE;

  // created by taskCheckVulkanSurfaceCapabilities
  VkSurfaceFormatKHR surfaceFormat;
  VkPresentModeKHR presentMode;
  VkExtent2D extent;

  // created by taskCreateVulkanSwapChain
  VkSwapchainKHR swapChain = VK_NULL_HANDLE;
  std::vector<VkImage> swapChainImages;
  VkExtent2D swapChainExtent;

  // created by taskCreateVulkanSwapChainImageViews
  std::vector<VkImageView> swapChainImageViews;

  // create by taskCreateVulkanDefaultRenderPass
  VkRenderPass defaultRenderPass;

  // created by taskCreateVulkanDefaultPipeline
  VkShaderModule vertShaderModule = VK_NULL_HANDLE;
  VkShaderModule fragShaderModule = VK_NULL_HANDLE;
  VkPipelineLayout defaultPipelineLayout = VK_NULL_HANDLE;
  VkPipeline defaultGraphicsPipeline = VK_NULL_HANDLE;

  // created by taskCreateVulkanDefaultFramebuffers
  std::vector<VkFramebuffer> swapChainFramebuffers;

  // created by taskCreateVulkanCommandPool
  VkCommandPool commandPool;

  // created by taskCreateVulkanCommandBuffers
  std::vector<VkCommandBuffer> commandBuffers;

  // created by taskCreateVulkanSemaphores
  VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
  VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
};

tsk::TaskResult taskInitGLFWWindow(VulkanSquirrelData &data) {
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

tsk::TaskResult taskCheckVulkanExtensions(VulkanSquirrelData &data) {
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

tsk::TaskResult taskInitVulkanInstance(VulkanSquirrelData &data) {
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

  data.mainQueueFamilyIndex = findSuitableVKQueue(data.physicalDevice, data.surface);
  VkDeviceQueueCreateInfo queueCreateInfo = {};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = data.mainQueueFamilyIndex;
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

  vkGetDeviceQueue(data.device, data.mainQueueFamilyIndex, 0, &data.mainQueue);

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

  createInfo.pQueueFamilyIndices = &data.mainQueueFamilyIndex;
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

tsk::TaskResult taskCreateVulkanSwapChainImageViews(VulkanSquirrelData &data) {
  data.swapChainImageViews.resize(data.swapChainImages.size(), VK_NULL_HANDLE);

  for (size_t i = 0; i < data.swapChainImages.size(); i++) {
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = data.swapChainImages[i];

    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = data.surfaceFormat.format;

    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    VkResult result;
    if ((result = vkCreateImageView(data.device, &createInfo, nullptr, &data.swapChainImageViews[i])) != VK_SUCCESS) {

      std::stringstream errorStringStream;
      errorStringStream << "Failed to create Vulkan swap chain image view with vk error code: " << result;
      return {
        false,
        kVKFailedToCreateVulkanSwapChainImageView,
        errorStringStream.str()
      };
    }
  }

  return tsk::kTaskSuccess;
}

bool readFile(const std::string& filename, std::vector<char> &output) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    return false;
  }

  size_t fileSize = (size_t)file.tellg();
  output.resize(fileSize);

  file.seekg(0);
  file.read(output.data(), fileSize);

  file.close();

  return true;
}

tsk::TaskResult taskCreateVulkanDefaultRenderPass(VulkanSquirrelData &data) {

  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format = data.surfaceFormat.format;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;

  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;

  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;

  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  VkResult result;
  if ((result = vkCreateRenderPass(data.device, &renderPassInfo, nullptr, &data.defaultRenderPass)) != VK_SUCCESS) {

    std::stringstream errorStringStream;
    errorStringStream << "Failed to create Vulkan render pass with vk error code: " << result;
    return {
      false,
      kVKFailedToCreateDefaultVulkanRenderPass,
      errorStringStream.str()
    };
  }

  return tsk::kTaskSuccess;
}

tsk::TaskResult taskCreateVulkanDefaultPipeline(VulkanSquirrelData &data) {

  std::vector<char> vertShaderCode;
  std::vector<char> fragShaderCode;

  if (!readFile("./Assets/test.vert.spv", vertShaderCode)) {
    return {
      false,
      kVKFailedToReadDefaultVulkanVertShader,
      "Failed to read default Vulkan vert shader"
    };
  }

  if (!readFile("./Assets/test.frag.spv", fragShaderCode)) {
    return {
      false,
      kVKFailedToReadDefaultVulkanFragShader,
      "Failed to read default Vulkan frag shader"
    };
  }

  {
    VkResult vertResult = createVkShaderModule(data.device, vertShaderCode, data.vertShaderModule);
    if (vertResult != VK_SUCCESS) {

      std::stringstream errorStringStream;
      errorStringStream << "Failed to create Vulkan default vert shader module with vk error code: " << vertResult;
      return {
        false,
        kVKFailedToCreateDefaultVulkanVertShaderModule,
        errorStringStream.str()
      };
    }
  }

  {
    VkResult fragResult = createVkShaderModule(data.device, fragShaderCode, data.fragShaderModule);
    if (fragResult != VK_SUCCESS) {

      std::stringstream errorStringStream;
      errorStringStream << "Failed to create Vulkan default frag shader module with vk error code: " << fragResult;
      return {
        false,
        kVKFailedToCreateDefaultVulkanFragShaderModule,
        errorStringStream.str()
      };
    }
  }

  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = data.vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = data.fragShaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(data.swapChainExtent.width);
  viewport.height = static_cast<float>(data.swapChainExtent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {};
  scissor.offset = { 0, 0 };
  scissor.extent = data.swapChainExtent;

  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;

  rasterizer.rasterizerDiscardEnable = VK_FALSE;

  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

  rasterizer.lineWidth = 1.0f;

  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f; // Optional
  rasterizer.depthBiasClamp = 0.0f; // Optional
  rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f; // Optional
  multisampling.pSampleMask = nullptr; // Optional
  multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
  multisampling.alphaToOneEnable = VK_FALSE; // Optional

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f; // Optional
  colorBlending.blendConstants[1] = 0.0f; // Optional
  colorBlending.blendConstants[2] = 0.0f; // Optional
  colorBlending.blendConstants[3] = 0.0f; // Optional

  VkDynamicState dynamicStates[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_LINE_WIDTH
  };

  VkPipelineDynamicStateCreateInfo dynamicState = {};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = 2;
  dynamicState.pDynamicStates = dynamicStates;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0; // Optional
  pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
  pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
  pipelineLayoutInfo.pPushConstantRanges = 0; // Optional

  {
    VkResult result;
    if ((result = vkCreatePipelineLayout(data.device, &pipelineLayoutInfo, nullptr, &data.defaultPipelineLayout)) != VK_SUCCESS) {

      std::stringstream errorStringStream;
      errorStringStream << "Failed to create Vulkan pipeline layout with vk error code: " << result;
      return {
        false,
        kVKFailedToCreateDefaultVulkanPipelineLayout,
        errorStringStream.str()
      };
    }
  }

  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;

  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = nullptr; // Optional
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = nullptr; // Optional

  pipelineInfo.layout = data.defaultPipelineLayout;

  pipelineInfo.renderPass = data.defaultRenderPass;
  pipelineInfo.subpass = 0;

  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
  pipelineInfo.basePipelineIndex = -1; // Optional

  {
    VkResult result;
    if ((result = vkCreateGraphicsPipelines(data.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &data.defaultGraphicsPipeline)) != VK_SUCCESS) {

      std::stringstream errorStringStream;
      errorStringStream << "Failed to create Vulkan graphics pipeline with vk error code: " << result;
      return {
        false,
        kVKFailedToCreateDefaultVulkanGraphicsPipeline,
        errorStringStream.str()
      };
    }
  }

  return tsk::kTaskSuccess;
}

tsk::TaskResult taskCreateVulkanDefaultFramebuffers(VulkanSquirrelData &data) {
  
  data.swapChainFramebuffers.resize(data.swapChainImageViews.size(), VK_NULL_HANDLE);

  for (size_t i = 0; i < data.swapChainImageViews.size(); i++) {
    VkImageView attachments[] = {
      data.swapChainImageViews[i]
    };

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = data.defaultRenderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = data.swapChainExtent.width;
    framebufferInfo.height = data.swapChainExtent.height;
    framebufferInfo.layers = 1;

    VkResult result;
    if ((result = vkCreateFramebuffer(data.device, &framebufferInfo, nullptr, &data.swapChainFramebuffers[i])) != VK_SUCCESS) {

      std::stringstream errorStringStream;
      errorStringStream << "Failed to create Vulkan framebuffer with vk error code: " << result;
      return {
        false,
        kVKFailedToCreateDefaultVulkanFramebuffer,
        errorStringStream.str()
      };
    }
  }

  return tsk::kTaskSuccess;
}

tsk::TaskResult taskCreateVulkanCommandPool(VulkanSquirrelData &data) {

  VkCommandPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = data.mainQueueFamilyIndex;
  poolInfo.flags = 0; // Optional

  VkResult result;
  if ((result = vkCreateCommandPool(data.device, &poolInfo, nullptr, &data.commandPool)) != VK_SUCCESS) {

    std::stringstream errorStringStream;
    errorStringStream << "Failed to create Vulkan command pool with vk error code: " << result;
    return {
      false,
      kVKFailedToCreateDefaultVulkanCommandPool,
      errorStringStream.str()
    };
  }

  return tsk::kTaskSuccess;
}


tsk::TaskResult taskCreateVulkanCommandBuffers(VulkanSquirrelData &data) {

  data.commandBuffers.resize(data.swapChainFramebuffers.size());

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = data.commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)data.commandBuffers.size();

  VkResult result;
  if ((result = vkAllocateCommandBuffers(data.device, &allocInfo, data.commandBuffers.data())) != VK_SUCCESS) {

    std::stringstream errorStringStream;
    errorStringStream << "Failed to create Vulkan command buffers with vk error code: " << result;
    return {
      false,
      kVKFailedToCreateDefaultVulkanCommandBuffers,
      errorStringStream.str()
    };
  }

  for (size_t i = 0; i < data.commandBuffers.size(); i++) {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    beginInfo.pInheritanceInfo = nullptr; // Optional

    vkBeginCommandBuffer(data.commandBuffers[i], &beginInfo);

    VkRenderPassBeginInfo renderPassInfo = {};

    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = data.defaultRenderPass;
    renderPassInfo.framebuffer = data.swapChainFramebuffers[i];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = data.swapChainExtent;

    VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(data.commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(data.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, data.defaultGraphicsPipeline);
    vkCmdDraw(data.commandBuffers[i], 3, 1, 0, 0);
    vkCmdEndRenderPass(data.commandBuffers[i]);

    VkResult result;
    if ((result = vkEndCommandBuffer(data.commandBuffers[i])) != VK_SUCCESS) {

      std::stringstream errorStringStream;
      errorStringStream << "Failed to create Vulkan command buffer with vk error code: " << result;
      return {
        false,
        kVKFailedToCreateDefaultVulkanCommandBuffer,
        errorStringStream.str()
      };
    }
  }

  return tsk::kTaskSuccess;
}

tsk::TaskResult taskCreateVulkanSemaphores(VulkanSquirrelData &data) {

  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  const auto createSemaphore = [&](VkSemaphore &semaphore, tsk::TaskResult &taskResult) -> bool {

    VkResult result;
    if ((result = vkCreateSemaphore(data.device, &semaphoreInfo, nullptr, &semaphore)) != VK_SUCCESS) {

      std::stringstream errorStringStream;
      errorStringStream << "Failed to create Vulkan semaphore with vk error code: " << result;
      taskResult = {
        false,
        kVKFailedToCreateDefaultVulkanSemaphore,
        errorStringStream.str()
      };

      return false;
    }

    return true;
  };

  tsk::TaskResult taskResult;

  if (!createSemaphore(data.imageAvailableSemaphore, taskResult)) {
    return taskResult;
  }

  if (!createSemaphore(data.renderFinishedSemaphore, taskResult)) {
    return taskResult;
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
      }, {
        "Create Vulkan Swap Chain Image Views",
        taskCreateVulkanSwapChainImageViews
      }, {
        "Create Vulkan Render Pass",
        taskCreateVulkanDefaultRenderPass
      }, {
        "Create Vulkan Default Pipeline",
        taskCreateVulkanDefaultPipeline
      }, {
        "Create Vulkan Default Framebuffers",
        taskCreateVulkanDefaultFramebuffers
      }, {
        "Create Vulkan command pool",
        taskCreateVulkanCommandPool
      }, {
        "Create Vulkan command buffers",
        taskCreateVulkanCommandBuffers
      }, {
        "Create Vulkan semaphores",
        taskCreateVulkanSemaphores
      }
    }
  );

  // THE LOOP!
  while (!glfwWindowShouldClose(data.window)) {
    glfwPollEvents();

    uint32_t imageIndex;
    vkAcquireNextImageKHR(data.device, data.swapChain, std::numeric_limits<uint64_t>::max(), data.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { data.imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &data.commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = { data.renderFinishedSemaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    VkResult result;
    if ((result = vkQueueSubmit(data.mainQueue, 1, &submitInfo, VK_NULL_HANDLE)) != VK_SUCCESS) {

      std::cerr << "Failed to submit to Vulkan queue with vk error code: " << result;
      break;
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { data.swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = nullptr; // Optional

    vkQueuePresentKHR(data.mainQueue, &presentInfo);

    vkQueueWaitIdle(data.mainQueue);
  } // the loop

  if (data.device != VK_NULL_HANDLE) {

    vkDeviceWaitIdle(data.device);

    if (data.fragShaderModule != VK_NULL_HANDLE) {
      vkDestroyShaderModule(data.device, data.fragShaderModule, nullptr);
    }

    if (data.vertShaderModule != VK_NULL_HANDLE) {
      vkDestroyShaderModule(data.device, data.vertShaderModule, nullptr);
    }

    if (data.swapChain != VK_NULL_HANDLE) {
      vkDestroySwapchainKHR(data.device, data.swapChain, nullptr);
    }

    for (size_t i = 0; i < data.swapChainFramebuffers.size(); i++) {
      if (data.swapChainFramebuffers[i] != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(data.device, data.swapChainFramebuffers[i], nullptr);
      }
    }

    for (size_t i = 0; i < data.swapChainImageViews.size(); i++) {
      if (data.swapChainImageViews[i] != VK_NULL_HANDLE) {
        vkDestroyImageView(data.device, data.swapChainImageViews[i], nullptr);
      }
    }

    if (data.defaultPipelineLayout != VK_NULL_HANDLE) {
      vkDestroyPipelineLayout(data.device, data.defaultPipelineLayout, nullptr);
    }

    if (data.defaultRenderPass != VK_NULL_HANDLE) {
      vkDestroyRenderPass(data.device, data.defaultRenderPass, nullptr);
    }

    if (data.defaultGraphicsPipeline != VK_NULL_HANDLE) {
      vkDestroyPipeline(data.device, data.defaultGraphicsPipeline, nullptr);
    }
    
    if (data.commandPool != VK_NULL_HANDLE) {
      vkDestroyCommandPool(data.device, data.commandPool, nullptr);
    }

    if (data.renderFinishedSemaphore != VK_NULL_HANDLE) {
      vkDestroySemaphore(data.device, data.renderFinishedSemaphore, nullptr);
    }

    if (data.imageAvailableSemaphore != VK_NULL_HANDLE) {
      vkDestroySemaphore(data.device, data.imageAvailableSemaphore, nullptr);
    }

    vkDestroyDevice(data.device, nullptr);
  }

  if (data.instance != VK_NULL_HANDLE) {

    if (data.callbackDebugInstance != VK_NULL_HANDLE) {
      DestroyVkDebugReportCallbackEXT(data.instance, data.callbackDebugInstance, nullptr);
    }

    if (data.surface != VK_NULL_HANDLE) {
      vkDestroySurfaceKHR(data.instance, data.surface, nullptr);
    }

    vkDestroyInstance(data.instance, nullptr);
  }

  if (data.window != nullptr) {
    glfwDestroyWindow(data.window);
  }

  glfwTerminate();
}

} // namespace VKS
