#include "renderer.h"

int main() {
  try {
    VulkanRenderer app;
    std::cout << "Press S to save current frame to bmp." << std::endl;
    std::cout << "Press R to request redraw." << std::endl;
    std::cout << "max depth of bouncing rays: ";
    std::cin >> app.maxDepthRequested;
    std::cout << "samples per pixel: : ";
    std::cin >> app.samplesPerPixelRequested;
    app.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
