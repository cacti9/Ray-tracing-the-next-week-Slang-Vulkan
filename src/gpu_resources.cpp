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

void GpuResources::createSampledImage(
  uint32_t width, uint32_t height, vk::Format format, VkDeviceSize size, const void* data, vk::Image& image
) {
  auto stagingBuffer = allocator.createStagingBuffer(size, data);
  createImage(
    width,
    height,
    1,
    vk::SampleCountFlagBits::e1,
    format,
    vk::ImageTiling::eOptimal,
    vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
    image
  );
  transitionImageLayout(
    image,
    vk::ImageLayout::eUndefined,
    vk::ImageLayout::eTransferDstOptimal,
    {},
    vk::AccessFlagBits2::eTransferWrite,
    vk::PipelineStageFlagBits2::eTopOfPipe,
    vk::PipelineStageFlagBits2::eTransfer
  );
  copyBufferToImage(stagingBuffer.getBuffer(), image, width, height);
  transitionImageLayout(
    image,
    vk::ImageLayout::eTransferDstOptimal,
    vk::ImageLayout::eShaderReadOnlyOptimal,
    vk::AccessFlagBits2::eTransferWrite,
    vk::AccessFlagBits2::eShaderSampledRead,
    vk::PipelineStageFlagBits2::eTransfer,
    vk::PipelineStageFlagBits2::eComputeShader
  );
}

vk::raii::ImageView
GpuResources::createImageView(vk::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels) {
  vk::ImageViewCreateInfo viewInfo{
    .image = image, .viewType = vk::ImageViewType::e2D, .format = format, .subresourceRange = {aspectFlags, 0, mipLevels, 0, 1}
  };
  return vk::raii::ImageView(device, viewInfo);
}

vk::raii::Sampler GpuResources::createSampler() {
  vk::SamplerCreateInfo samplerInfo{
    .magFilter = vk::Filter::eNearest,
    .minFilter = vk::Filter::eNearest,
    .mipmapMode = vk::SamplerMipmapMode::eLinear,
    .addressModeU = vk::SamplerAddressMode::eClampToEdge,
    .addressModeV = vk::SamplerAddressMode::eClampToEdge,
    .addressModeW = vk::SamplerAddressMode::eClampToEdge,
    .mipLodBias = 0.0f,
    .anisotropyEnable = vk::False,
    .maxAnisotropy = 1.0f,
    .compareEnable = vk::False,
    .compareOp = vk::CompareOp::eAlways,
    .minLod = 0.0f,
    .maxLod = 0.0f,
    .borderColor = vk::BorderColor::eIntOpaqueBlack,
    .unnormalizedCoordinates = vk::False,
  };
  return vk::raii::Sampler(device, samplerInfo);
}

void GpuResources::destroyImage(vk::Image& image) { allocator.destroyImage(image); }

void GpuResources::copyBuffer(const vk::Buffer& srcBuffer, const vk::Buffer& dstBuffer, VkDeviceSize size) {
  auto commandCopyBuffer = beginSingleTimeCommands();
  commandCopyBuffer.copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy{0, 0, size});
  endSingleTimeCommands(commandCopyBuffer);
}

void GpuResources::copyBufferToImage(const vk::Buffer& srcBuffer, vk::Image image, uint32_t width, uint32_t height) {
  vk::BufferImageCopy copyRegion{
    .bufferOffset = 0,
    .bufferRowLength = 0,
    .bufferImageHeight = 0,
    .imageSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1},
    .imageOffset = {0, 0, 0},
    .imageExtent = {width, height, 1},
  };
  auto commandCopyBuffer = beginSingleTimeCommands();
  commandCopyBuffer.copyBufferToImage(srcBuffer, image, vk::ImageLayout::eTransferDstOptimal, copyRegion);
  endSingleTimeCommands(commandCopyBuffer);
}

void GpuResources::transitionImageLayout(
  vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::AccessFlags2 srcAccessMask,
  vk::AccessFlags2 dstAccessMask, vk::PipelineStageFlags2 srcStageMask, vk::PipelineStageFlags2 dstStageMask
) {
  vk::ImageMemoryBarrier2 barrier{
    .srcStageMask = srcStageMask,
    .srcAccessMask = srcAccessMask,
    .dstStageMask = dstStageMask,
    .dstAccessMask = dstAccessMask,
    .oldLayout = oldLayout,
    .newLayout = newLayout,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = image,
    .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}
  };
  vk::DependencyInfo dependencyInfo{.imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier};
  auto commandBarrier = beginSingleTimeCommands();
  commandBarrier.pipelineBarrier2(dependencyInfo);
  endSingleTimeCommands(commandBarrier);
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
