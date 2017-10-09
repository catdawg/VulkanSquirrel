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

class VulkanSquirrel
{
  public:
    void Run(const VulkanSquirrelOptions &options);
};

} // namespace VKS


