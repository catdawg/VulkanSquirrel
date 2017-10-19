#include "VulkanUtils.h"

#include <vector>
#include <set>

#include <vulkan\vulkan.hpp>

namespace vks {

std::vector<VkExtensionProperties> GetVkExtensions() {
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

  std::vector<VkExtensionProperties> extensions(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

  return extensions;
}

const VkExtensionProperties* FindVkExtension(std::vector<VkExtensionProperties> &availableVKExtensions, const char* neededExtension) {
  for (const auto& availableExtension : availableVKExtensions) {

    if (strcmp(availableExtension.extensionName, neededExtension) == 0) {
      return &availableExtension;
    }
  }

  return nullptr;
}

bool CheckVkValidationLayerSupport(const std::vector<const char*> &requestedLayers) {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (int i = 0; i < requestedLayers.size(); ++i) {
    bool layerFound = false;

    for (const auto& layerProperties : availableLayers) {
      if (strcmp(requestedLayers[i], layerProperties.layerName) == 0) {
        layerFound = true;
        break;
      }
    }

    if (!layerFound) {
      return false;
    }
  }

  return true;
}


VkResult CreateVkDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
  auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pCallback);
  }
  else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyVkDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
  auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
  if (func != nullptr) {
    func(instance, callback, pAllocator);
  }
}

std::vector<VkQueueFamilyProperties> GetVkFamiliesOfDevice(const VkPhysicalDevice &device) {

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

  return queueFamilies;
}

bool CheckVkExtensionSupport(const VkPhysicalDevice &device, const std::vector<const char*> &extensions) {

  uint32_t deviceExtensionCount;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, nullptr);

  std::vector<VkExtensionProperties> deviceAvailableExtensions(deviceExtensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, deviceAvailableExtensions.data());

  //making a std::string allows the comparison in .erase to work
  std::set<std::string> requiredExtensions(extensions.begin(), extensions.end());

  for (const auto& extension : deviceAvailableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}

VkResult createVkShaderModule(const VkDevice &device, const std::vector<char>& code, VkShaderModule &output) {

  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

  return vkCreateShaderModule(device, &createInfo, nullptr, &output);
}

} // namespace vks
