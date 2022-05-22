#include "Open4XVulkan.hpp"

void Open4XVulkan::createImageViews() {
  swapChainImageViews.resize(swapChainImages.size());
  for (size_t i = 0; i < swapChainImages.size(); i++) {
    swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat);
  }
}
