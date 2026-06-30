#include "gpu_resources.h"

#include <cstring>

GpuResources::GpuResources(
  VmaAllocatorCreateFlags vmaAvailableFlags, vk::raii::PhysicalDevice& physicalDevice, vk::raii::Device& device,
  vk::raii::Instance& instance, vk::raii::CommandPool& commandPool, vk::raii::Queue& queue
)
    : physicalDevice(physicalDevice), device(device), commandPool(commandPool), queue(queue) {
  allocator.init(vmaAvailableFlags, physicalDevice, device, instance);
}

void GpuResources::createDeviceLocalBuffer(vk::Buffer& buffer, VkDeviceSize size, const void* data, VkBufferUsageFlags usage) {
  destroyBuffer(buffer);
  auto stagingBuffer = allocator.createStagingBuffer(size, data);
  buffer = allocator.createDeviceLocalBuffer(size, usage);
  copyBuffer(stagingBuffer.getBuffer(), buffer, size);
}

void GpuResources::createDeviceLocalBuffer(vk::Buffer& buffer, VkDeviceSize size, VkBufferUsageFlags usage) {
  destroyBuffer(buffer);
  buffer = allocator.createDeviceLocalBuffer(size, usage);
}

std::pair<vk::Buffer, void*> GpuResources::createMappedBuffer(VkDeviceSize size, VkBufferUsageFlags usage) {
  return allocator.createMappedBuffer(size, usage);
}

void GpuResources::readDeviceLocalBuffer(vk::Buffer srcBuffer, void* dst, VkDeviceSize size) {
  auto [readbackBuffer, mapped] = allocator.createReadbackBuffer(size);
  copyBuffer(srcBuffer, readbackBuffer, size);
  allocator.invalidateBuffer(readbackBuffer, size);
  std::memcpy(dst, mapped, size);
  allocator.destroyBuffer(readbackBuffer);
}

void GpuResources::destroyBuffer(vk::Buffer& buffer) { allocator.destroyBuffer(buffer); }

void GpuResources::createImage(
  uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format,
  vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::Image& image
) {
  vk::ImageCreateInfo imageInfo{
    .imageType = vk::ImageType::e2D,
    .format = format,
    .extent = {width, height, 1},
    .mipLevels = mipLevels,
    .arrayLayers = 1,
    .samples = numSamples,
    .tiling = tiling,
    .usage = usage,
    .sharingMode = vk::SharingMode::eExclusive
  };
  allocator.createImage(imageInfo, image);
}

vk::raii::ImageView
GpuResources::createImageView(vk::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels) {
  vk::ImageViewCreateInfo viewInfo{
    .image = image, .viewType = vk::ImageViewType::e2D, .format = format, .subresourceRange = {aspectFlags, 0, mipLevels, 0, 1}
  };
  return vk::raii::ImageView(device, viewInfo);
}

void GpuResources::destroyImage(vk::Image& image) { allocator.destroyImage(image); }

void GpuResources::copyBuffer(const vk::Buffer& srcBuffer, const vk::Buffer& dstBuffer, VkDeviceSize size) {
  auto commandCopyBuffer = beginSingleTimeCommands();
  commandCopyBuffer.copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy{0, 0, size});
  endSingleTimeCommands(commandCopyBuffer);
}

vk::raii::CommandBuffer GpuResources::beginSingleTimeCommands() {
  vk::CommandBufferAllocateInfo allocInfo{
    .commandPool = *commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1
  };
  vk::raii::CommandBuffer commandBuffer = std::move(vk::raii::CommandBuffers(device, allocInfo).front());

  vk::CommandBufferBeginInfo beginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
  commandBuffer.begin(beginInfo);

  return commandBuffer;
}

void GpuResources::endSingleTimeCommands(vk::raii::CommandBuffer& commandBuffer) {
  commandBuffer.end();
  vk::SubmitInfo submitInfo{.commandBufferCount = 1, .pCommandBuffers = &*commandBuffer};
  queue.submit(submitInfo, nullptr);
  queue.waitIdle();
}
