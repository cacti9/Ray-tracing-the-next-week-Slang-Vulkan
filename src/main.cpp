#include "renderer.h"

int main() {
  try {
    VulkanRenderer app;
    std::cout << "Press S to save current frame to bmp." << std::endl;
    app.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
