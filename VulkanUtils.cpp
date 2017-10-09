#include "VulkanUtils.h"

#include <vector>

#include <vulkan\vulkan.hpp>

namespace vks {

std::vector<VkExtensionProperties> GetVkExtensions() {
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

  std::vector<VkExtensionProperties> extensions(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

  return extensions;
}

const VkExtensionProperties* FindVKExtension(std::vector<VkExtensionProperties> &availableVKExtensions, const char* neededExtension) {
  for (const auto& availableExtension : availableVKExtensions) {

    if (strcmp(availableExtension.extensionName, neededExtension) == 0) {
      return &availableExtension;
    }
  }

  return nullptr;
}

bool CheckVKValidationLayerSupport(const char* const* layers, size_t layersCount) {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (int i = 0; i < layersCount; ++i) {
    bool layerFound = false;

    for (const auto& layerProperties : availableLayers) {
      if (strcmp(layers[i], layerProperties.layerName) == 0) {
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


VkResult CreateVKDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
  auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pCallback);
  }
  else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyVKDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
  auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
  if (func != nullptr) {
    func(instance, callback, pAllocator);
  }
}

} // namespace vks
