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
  kVKFailedToCreateVulkanSwapChain = 2006,
  kVKFailedToCreateVulkanSwapChainImageView = 2007,
  kVKFailedToReadDefaultVulkanVertShader = 2008,
  kVKFailedToReadDefaultVulkanFragShader = 2009,
  kVKFailedToCreateDefaultVulkanFragShaderModule = 2010,
  kVKFailedToCreateDefaultVulkanVertShaderModule = 2011,
  kVKFailedToCreateDefaultVulkanRenderPass = 2012,
  kVKFailedToCreateDefaultVulkanPipelineLayout = 2013,
  kVKFailedToCreateDefaultVulkanGraphicsPipeline = 2014,
  kVKFailedToCreateDefaultVulkanFramebuffer = 2015,
  kVKFailedToCreateDefaultVulkanCommandPool = 2016,
  kVKFailedToCreateDefaultVulkanCommandBuffers = 2017,
  kVKFailedToCreateDefaultVulkanCommandBuffer = 2018,
  kVKFailedToCreateDefaultVulkanSemaphore = 2019,
};

class VulkanSquirrel
{
  public:
    void Run(const VulkanSquirrelOptions &options);
};

} // namespace VKS
