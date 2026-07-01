#include "renderer.h"
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>

namespace {
void writeBmp32(const char* path, uint32_t width, uint32_t height, const void* bgraPixels) {
  const uint32_t rowSize = width * 4;
  const uint32_t pixelDataSize = rowSize * height;
  const uint32_t fileSize = 14 + 40 + pixelDataSize;
  const int32_t topDownHeight = -static_cast<int32_t>(height);

  uint8_t fileHeader[14] = {
    'B',
    'M',
    static_cast<uint8_t>(fileSize),
    static_cast<uint8_t>(fileSize >> 8),
    static_cast<uint8_t>(fileSize >> 16),
    static_cast<uint8_t>(fileSize >> 24),
    0,
    0,
    0,
    0,
    54,
    0,
    0,
    0
  };

  uint8_t infoHeader[40] = {};
  infoHeader[0] = 40;
  infoHeader[12] = 1;
  infoHeader[14] = 32;
  std::memcpy(&infoHeader[4], &width, sizeof(width));
  std::memcpy(&infoHeader[8], &topDownHeight, sizeof(topDownHeight));
  std::memcpy(&infoHeader[20], &pixelDataSize, sizeof(pixelDataSize));

  std::ofstream file(path, std::ios::binary);
  file.write(reinterpret_cast<char*>(fileHeader), sizeof(fileHeader));
  file.write(reinterpret_cast<char*>(infoHeader), sizeof(infoHeader));
  file.write(reinterpret_cast<const char*>(bgraPixels), pixelDataSize);
}
} // namespace

void VulkanRenderer::drawFrame() {
  auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, nullptr, *inFlightFences[currentFrame]);
  while (vk::Result::eTimeout == device.waitForFences(*inFlightFences[currentFrame], vk::True, UINT64_MAX))
    ;
  device.resetFences(*inFlightFences[currentFrame]);

  uint64_t frameSignalValue = ++timelineValue;

  updateUniformBuffer(currentFrame);
  recordFrameCommandBuffer(imageIndex);

  vk::TimelineSemaphoreSubmitInfo frameTimelineInfo{
    .signalSemaphoreValueCount = 1,
    .pSignalSemaphoreValues = &frameSignalValue,
  };
  vk::SubmitInfo frameSubmitInfo{
    .pNext = &frameTimelineInfo,
    .commandBufferCount = 1,
    .pCommandBuffers = &*frameCommandBuffers[currentFrame],
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = &*semaphore
  };
  allInOneQueue.submit(frameSubmitInfo, nullptr);

  vk::SemaphoreWaitInfo waitInfo{.semaphoreCount = 1, .pSemaphores = &*semaphore, .pValues = &frameSignalValue};
  while (vk::Result::eTimeout == device.waitSemaphores(waitInfo, UINT64_MAX))
    ;

  if (saveBmpRequested) {
    savePixelBufferToBmp(currentFrame);
    saveBmpRequested = false;
  }

  vk::PresentInfoKHR presentInfo{
    .waitSemaphoreCount = 0,
    .pWaitSemaphores = nullptr,
    .swapchainCount = 1,
    .pSwapchains = &*swapChain,
    .pImageIndices = &imageIndex
  };

  try {
    result = allInOneQueue.presentKHR(presentInfo);
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebufferResized) {
      framebufferResized = false;
      recreateSwapChain();
    } else if (result != vk::Result::eSuccess) {
      throw std::runtime_error("failed to present swap chain image!");
    }
  } catch (const vk::SystemError& e) {
    if (e.code().value() == static_cast<int>(vk::Result::eErrorOutOfDateKHR)) {
      recreateSwapChain();
      return;
    } else {
      throw;
    }
  }
  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::recreateSwapChain() {
  int width = 0, height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window, &width, &height);
    glfwWaitEvents();
  }

  device.waitIdle();

  cleanupSwapChain();
  createSwapChain();
  createSwapChainImageViews();
  std::cerr << width << " " << height << std::endl;
  computeImageRenderer.createBuffers(*gpuResources, swapChainExtent);
  computeImageRenderer.createDescriptorSets(device, descriptorPool, uniformBuffers);
}
void VulkanRenderer::cleanupSwapChain() {
  swapChainImageViews.clear();
  swapChain = nullptr;
}

void VulkanRenderer::updateUniformBuffer(uint32_t currentImage) {
  static auto startTime = std::chrono::high_resolution_clock::now();

  auto currentTime = std::chrono::high_resolution_clock::now();
  float time = std::chrono::duration<float>(currentTime - startTime).count() / 3.0f;
  const float deltaSeconds = static_cast<float>(lastFrameTime) / 1000.0f;

  UniformBufferObject ubo{};
  ubo.renderExtent = glm::uvec4(swapChainExtent.width, swapChainExtent.height, currentImage, 0);
  ubo.deltaTime = static_cast<float>(lastFrameTime) * 2.0f;

  constexpr double focal_length = 1.0;
  constexpr double vfov = 90.0;
  constexpr double theta = vfov * 3.14159265358979323846264338327950288 / 180.0;
  const double h = std::tan(theta / 2.);
  const double viewport_height = 2.0 * h * focal_length;
  double viewport_width = viewport_height * ((double)swapChainExtent.width / swapChainExtent.height);
  glm::dvec3 camera_center{0.};
  glm::dvec3 viewport_u{viewport_width, 0., 0.};
  glm::dvec3 viewport_v{0., -viewport_height, 0.};

  glm::dvec3 pixel_delta_u = viewport_u / (double)swapChainExtent.width;
  glm::dvec3 pixel_delta_v = viewport_v / (double)swapChainExtent.height;
  glm::dvec3 viewport_upper_left = camera_center - glm::dvec3(0, 0, focal_length) - viewport_u / 2. - viewport_v / 2.;
  glm::dvec3 pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

  ubo.pixel00_loc = {pixel00_loc, 0.};
  ubo.pixel_delta_u = {pixel_delta_u, 0.};
  ubo.pixel_delta_v = {pixel_delta_v, 0.};
  ubo.camera_center = {camera_center, 0.};

  memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void VulkanRenderer::savePixelBufferToBmp(uint32_t currentFrame) {
  std::vector<uint8_t> pixels(static_cast<size_t>(computeImageRenderer.pixelOutputSize()));
  gpuResources->readDeviceLocalBuffer(
    computeImageRenderer.pixelBuffer(currentFrame), pixels.data(), computeImageRenderer.pixelOutputSize()
  );
  writeBmp32("compute_image.bmp", swapChainExtent.width, swapChainExtent.height, pixels.data());
}

void VulkanRenderer::recordFrameCommandBuffer(uint32_t imageIndex) {
  frameCommandBuffers[currentFrame].reset();
  frameCommandBuffers[currentFrame].begin({});
  computeImageRenderer.recordComputeDispatch(frameCommandBuffers[currentFrame], currentFrame);
  recordBufferBarrier(
    frameCommandBuffers[currentFrame],
    computeImageRenderer.pixelBuffer(currentFrame),
    computeImageRenderer.pixelOutputSize(),
    vk::AccessFlagBits2::eShaderWrite,
    vk::AccessFlagBits2::eTransferRead,
    vk::PipelineStageFlagBits2::eComputeShader,
    vk::PipelineStageFlagBits2::eTransfer
  );
  recordImageBarrier(
    frameCommandBuffers[currentFrame],
    swapChainImages[imageIndex],
    vk::ImageLayout::eUndefined,
    vk::ImageLayout::eTransferDstOptimal,
    {},
    vk::AccessFlagBits2::eTransferWrite,
    vk::PipelineStageFlagBits2::eTopOfPipe,
    vk::PipelineStageFlagBits2::eTransfer,
    vk::ImageAspectFlagBits::eColor
  );
  computeImageRenderer.recordCopyToSwapChain(
    frameCommandBuffers[currentFrame], currentFrame, swapChainExtent, swapChainImages[imageIndex]
  );

  recordImageBarrier(
    frameCommandBuffers[currentFrame],
    swapChainImages[imageIndex],
    vk::ImageLayout::eTransferDstOptimal,
    vk::ImageLayout::ePresentSrcKHR,
    vk::AccessFlagBits2::eTransferWrite,
    {},
    vk::PipelineStageFlagBits2::eTransfer,
    vk::PipelineStageFlagBits2::eBottomOfPipe,
    vk::ImageAspectFlagBits::eColor
  );
  frameCommandBuffers[currentFrame].end();
}

void VulkanRenderer::recordBufferBarrier(
  vk::raii::CommandBuffer& commandBuffer, vk::Buffer buffer, vk::DeviceSize size, vk::AccessFlags2 src_access_mask,
  vk::AccessFlags2 dst_access_mask, vk::PipelineStageFlags2 src_stage_mask, vk::PipelineStageFlags2 dst_stage_mask
) {
  vk::BufferMemoryBarrier2 barrier{
    .srcStageMask = src_stage_mask,
    .srcAccessMask = src_access_mask,
    .dstStageMask = dst_stage_mask,
    .dstAccessMask = dst_access_mask,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .buffer = buffer,
    .offset = 0,
    .size = size
  };
  vk::DependencyInfo dependency_info{.bufferMemoryBarrierCount = 1, .pBufferMemoryBarriers = &barrier};
  commandBuffer.pipelineBarrier2(dependency_info);
}

void VulkanRenderer::recordImageBarrier(
  vk::raii::CommandBuffer& commandBuffer, vk::Image image, vk::ImageLayout old_layout, vk::ImageLayout new_layout,
  vk::AccessFlags2 src_access_mask, vk::AccessFlags2 dst_access_mask, vk::PipelineStageFlags2 src_stage_mask,
  vk::PipelineStageFlags2 dst_stage_mask, vk::ImageAspectFlags image_aspect_flags
) {
  vk::ImageMemoryBarrier2 barrier = {
    .srcStageMask = src_stage_mask,
    .srcAccessMask = src_access_mask,
    .dstStageMask = dst_stage_mask,
    .dstAccessMask = dst_access_mask,
    .oldLayout = old_layout,
    .newLayout = new_layout,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = image,
    .subresourceRange = {.aspectMask = image_aspect_flags, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}
  };
  vk::DependencyInfo dependency_info = {.dependencyFlags = {}, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier};
  commandBuffer.pipelineBarrier2(dependency_info);
}
