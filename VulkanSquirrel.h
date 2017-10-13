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
  int windowWidth;
  int windowHeight;
};

enum VulkanSquirrelErrorCodes {
  kGLFWWindowCouldNotBeCreated = 1000,
  kVKRequiredExtensionNotAvailable = 2000,
  kVKFailedToCreateInstance = 2001,
  kVKNoCompatibleGPUAvailable = 2002,
  kVKFailedToCreateVulkanLogicalDevice = 2003
};

class VulkanSquirrel
{
  public:
    void Run(const VulkanSquirrelOptions &options);
};

} // namespace VKS
