#include <iostream>
#include <vector>

#include "VulkanSquirrel.h"

int main() {
  vks::VulkanSquirrel app;

  struct vks::VulkanSquirrelOptions options;
  options.windowWidth = 800;
  options.windowHeight = 600;

#ifdef NODEBUG
  options.vulkanValidationLayersMode = vks::kNoVulkanValidationLayers;
#else
  options.vulkanValidationLayersMode = vks::kEnabledVulkanValidationLayers;
#endif

  options.vulkanValidationLayers = {
    "VK_LAYER_LUNARG_standard_validation"
  };

  options.vulkanExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };

  try {
    app.Run(options);
  }
  catch (const std::runtime_error& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
