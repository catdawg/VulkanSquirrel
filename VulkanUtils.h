#pragma once

#include <vector>

#include <vulkan\vulkan.hpp>

namespace vks {

std::vector<VkExtensionProperties> GetVkExtensions();
const VkExtensionProperties* FindVkExtension(std::vector<VkExtensionProperties> &availableVKExtensions, const char* neededExtension);

bool CheckVkValidationLayerSupport(const std::vector<const char*> &requestedLayers);

VkResult CreateVkDebugReportCallbackEXT(
  VkInstance instance,
  const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
  const VkAllocationCallbacks* pAllocator,
  VkDebugReportCallbackEXT* pCallback);

void DestroyVkDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);

std::vector<VkQueueFamilyProperties> GetVkFamiliesOfDevice(const VkPhysicalDevice &device);

}
