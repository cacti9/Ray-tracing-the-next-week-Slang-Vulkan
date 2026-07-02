#include <cstdint>
#include <iostream>
#include <optional>
#include <vector>

#include "compute_image/compute_image_renderer.h"
#include "gpu_resources.h"
#include "renderer_types.h"
#include "vma_raii.h"

class VulkanRenderer {
public:
  void run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
  }
  uint32_t maxDepthRequested = 10;
  uint32_t samplesPerPixelRequested = 10;

private:
  GLFWwindow* window = nullptr;
  vk::raii::Context context;
  vk::raii::Instance instance = nullptr;
  vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
  vk::raii::SurfaceKHR surface = nullptr;
  vk::raii::PhysicalDevice physicalDevice = nullptr;

  vk::raii::Device device = nullptr;
  uint32_t queueIndex = ~0;
  vk::raii::Queue allInOneQueue = nullptr;
  vk::raii::CommandPool commandPool = nullptr;
  vk::raii::DescriptorPool descriptorPool = nullptr;

  VmaAllocatorCreateFlags vmaAvailableFlags = 0;
  std::optional<GpuResources> gpuResources;

  vk::raii::SwapchainKHR swapChain = nullptr;
  std::vector<vk::Image> swapChainImages;
  vk::SurfaceFormatKHR swapChainSurfaceFormat;
  vk::Extent2D swapChainExtent;
  std::vector<vk::raii::ImageView> swapChainImageViews;

  ComputeImageRenderer computeImageRenderer;

  std::vector<vk::Buffer> uniformBuffers;
  std::vector<void*> uniformBuffersMapped;

  std::vector<vk::raii::CommandBuffer> frameCommandBuffers;

  vk::raii::Semaphore semaphore = nullptr;
  uint64_t timelineValue = 0;
  std::vector<vk::raii::Fence> inFlightFences;
  uint32_t currentFrame = 0;

  bool framebufferResized = false;
  bool saveBmpRequested = false;
  int redrawRequested = 0;

  std::vector<const char*> requiredDeviceExtension = {
    vk::KHRSwapchainExtensionName,
    vk::KHRSpirv14ExtensionName,
    vk::KHRSynchronization2ExtensionName,
    vk::KHRCreateRenderpass2ExtensionName
  };

  void initWindow();
  static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
  static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

  void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();

    createLogicalDevice();

    createSwapChain();
    createSwapChainImageViews();

    computeImageRenderer.createDescriptorSetLayout(device);
    computeImageRenderer.createComputePipeline(device);

    createCommandPool();

    // Resources
    gpuResources.emplace(vmaAvailableFlags, physicalDevice, device, instance, commandPool, allInOneQueue);
    computeImageRenderer.createBuffers(*gpuResources, swapChainExtent);

    createUniformBuffers();
    createDescriptorPool();
    computeImageRenderer.createDescriptorSets(device, descriptorPool, uniformBuffers);

    createFrameCommandBuffers();
    createSyncObjects();
  }

  void mainLoop();
  void cleanup();
  void createInstance();
  [[nodiscard]] std::vector<const char*> getRequiredExtensions() const;
  void setupDebugMessenger();
  static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*
  );
  void createSurface();
  void pickPhysicalDevice();

  void createLogicalDevice();

  void createSwapChain();
  static uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities);
  static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
  static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
  vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
  void createSwapChainImageViews();

  void createCommandPool();

  void createUniformBuffers();
  void createDescriptorPool();

  void createFrameCommandBuffers();
  void createSyncObjects();

  void drawFrame();
  void recreateSwapChain();
  void cleanupSwapChain();
  void updateUniformBuffer(uint32_t currentImage);
  void savePixelBufferToBmp(uint32_t currentFrame);
  void recordFrameCommandBuffer(uint32_t imageIndex);
  void recordBufferBarrier(
    vk::raii::CommandBuffer& commandBuffer, vk::Buffer buffer, vk::DeviceSize size, vk::AccessFlags2 src_access_mask,
    vk::AccessFlags2 dst_access_mask, vk::PipelineStageFlags2 src_stage_mask, vk::PipelineStageFlags2 dst_stage_mask
  );
  void recordImageBarrier(
    vk::raii::CommandBuffer& commandBuffer, vk::Image image, vk::ImageLayout old_layout, vk::ImageLayout new_layout,
    vk::AccessFlags2 src_access_mask, vk::AccessFlags2 dst_access_mask, vk::PipelineStageFlags2 src_stage_mask,
    vk::PipelineStageFlags2 dst_stage_mask, vk::ImageAspectFlags image_aspect_flags
  );
};
