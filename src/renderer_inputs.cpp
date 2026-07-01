#include "renderer.h"
#include "renderer_types.h"

void VulkanRenderer::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  (void)scancode;
  (void)mods;
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
    return;
  }

  if (key == GLFW_KEY_S && action == GLFW_PRESS) {
    auto app = static_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
    app->saveBmpRequested = true;
    return;
  }
  if (key == GLFW_KEY_R && action == GLFW_PRESS) {
    auto app = static_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
    app->redrawRequested = MAX_FRAMES_IN_FLIGHT;
    return;
  }
}
