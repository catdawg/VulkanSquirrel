#pragma once

#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace vks {

enum VulkanValidationLayerMode {
  kNoVulkanValidationLayers = 0,
  kEnabledVulkanValidationLayers,
};


struct VulkanSquirrelOptions {
  VulkanValidationLayerMode vulkanValidationLayersMode;
  std::vector<const char*> vulkanValidationLayers;
  std::vector<const char*> vulkanExtensions;
  int windowWidth;
  int windowHeight;
};

enum VulkanSquirrelErrorCodes {
  kGLFWWindowCouldNotBeCreated = 1000,
  kVKRequiredExtensionNotAvailable = 2000,
  kVKFailedToCreateInstance = 2001,
  kVKNoCompatibleGPUAvailable = 2002,
  kVKFailedToCreateVulkanPhysicalDevice = 2003,
  kVKFailedToCreateVulkanLogicalDevice = 2004,
  kVKFailedToCreateSurface = 2005,
  kVKFailedToCreateVulkanSwapChain = 2006
};

class VulkanSquirrel
{
  public:
    void Run(const VulkanSquirrelOptions &options);
};

} // namespace VKS
