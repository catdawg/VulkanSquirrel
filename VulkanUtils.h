#pragma once

#include <vector>

#include <vulkan\vulkan.hpp>

namespace vks {

std::vector<VkExtensionProperties> GetVkExtensions();
const VkExtensionProperties* FindVKExtension(std::vector<VkExtensionProperties> &availableVKExtensions, const char* neededExtension);

bool CheckVKValidationLayerSupport(const char* const* layers, size_t layersCount);

VkResult CreateVKDebugReportCallbackEXT(
  VkInstance instance,
  const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
  const VkAllocationCallbacks* pAllocator,
  VkDebugReportCallbackEXT* pCallback);

void DestroyVKDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);

}