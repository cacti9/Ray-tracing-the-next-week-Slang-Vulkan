#pragma once

#include "renderer_types.h"
#include "vma_raii.h"

class GpuResources {
public:
  GpuResources(
    VmaAllocatorCreateFlags vmaAvailableFlags, vk::raii::PhysicalDevice& physicalDevice, vk::raii::Device& device,
    vk::raii::Instance& instance, vk::raii::CommandPool& commandPool, vk::raii::Queue& queue
  );

  void createDeviceLocalBuffer(vk::Buffer& buffer, VkDeviceSize size, const void* data, VkBufferUsageFlags usage);
  void createDeviceLocalBuffer(vk::Buffer& buffer, VkDeviceSize size, VkBufferUsageFlags usage);
  std::pair<vk::Buffer, void*> createMappedBuffer(VkDeviceSize size, VkBufferUsageFlags usage);
  void readDeviceLocalBuffer(vk::Buffer srcBuffer, void* dst, VkDeviceSize size);
  void destroyBuffer(vk::Buffer& buffer);

  void createImage(
    uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format,
    vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::Image& image
  );
  void createSampledImage(uint32_t width, uint32_t height, vk::Format format, VkDeviceSize size, const void* data, vk::Image& image);
  [[nodiscard]] vk::raii::ImageView
  createImageView(vk::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels);
  [[nodiscard]] vk::raii::Sampler createSampler();
  void destroyImage(vk::Image& image);

  void copyBuffer(const vk::Buffer& srcBuffer, const vk::Buffer& dstBuffer, VkDeviceSize size);

private:
  vk::raii::CommandBuffer beginSingleTimeCommands();
  void endSingleTimeCommands(vk::raii::CommandBuffer& commandBuffer);
  void copyBufferToImage(const vk::Buffer& srcBuffer, vk::Image image, uint32_t width, uint32_t height);
  void transitionImageLayout(
    vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::AccessFlags2 srcAccessMask,
    vk::AccessFlags2 dstAccessMask, vk::PipelineStageFlags2 srcStageMask, vk::PipelineStageFlags2 dstStageMask
  );

  vma_raii::Allocator allocator;
  vk::raii::PhysicalDevice& physicalDevice;
  vk::raii::Device& device;
  vk::raii::CommandPool& commandPool;
  vk::raii::Queue& queue;
};
