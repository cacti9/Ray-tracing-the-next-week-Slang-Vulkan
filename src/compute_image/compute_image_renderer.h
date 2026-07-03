#pragma once

#include <vector>

#include "gpu_resources.h"
#include "rt_cpu_types.h"

struct ComputeImageRenderer {
  vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
  vk::raii::PipelineLayout computePipelineLayout = nullptr;
  vk::raii::Pipeline computePipeline = nullptr;

  std::vector<vk::raii::DescriptorSet> descriptorSets;
  std::vector<vk::Buffer> pixelBuffers;
  vk::DeviceSize pixelBufferSize = 0;
  std::vector<vk::Buffer> hittableBuffers;
  std::vector<Hittable> hittablesData;
  vk::DeviceSize hittableBufferSize = 0;
  std::vector<vk::Buffer> materialBuffers;
  std::vector<Material> materialsData;
  vk::DeviceSize materialBufferSize = 0;
  std::vector<vk::Buffer> textureBuffers;
  std::vector<Texture> texturesData;
  vk::DeviceSize textureBufferSize = 0;
  CameraSettings cameraSettings;
  vk::Extent2D renderExtent{};

  void createDescriptorSetLayout(vk::raii::Device const& device);
  void createComputePipeline(vk::raii::Device const& device);
  void populateWorld();
  void createBuffers(GpuResources& gpuResources, vk::Extent2D const& extent);
  void createDescriptorSets(
    vk::raii::Device const& device, vk::raii::DescriptorPool const& descriptorPool, std::vector<vk::Buffer> const& uniformBuffers
  );

  [[nodiscard]] vk::Buffer pixelBuffer(uint32_t currentFrame) const;
  [[nodiscard]] vk::DeviceSize pixelOutputSize() const;
  void recordComputeDispatch(vk::raii::CommandBuffer& commandBuffer, uint32_t currentFrame);
  void recordCopyToSwapChain(
    vk::raii::CommandBuffer& commandBuffer, uint32_t currentFrame, vk::Extent2D const& swapChainExtent, vk::Image swapChainImage
  );
};
