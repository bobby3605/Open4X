#include "Open4XVulkan.hpp"

void Open4XVulkan::createTextureImageView() {
  textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB);
}
