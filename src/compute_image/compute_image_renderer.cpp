#include "compute_image_renderer.h"

#include <array>
#include <random>
#include <string>
#include <vector>

void ComputeImageRenderer::createDescriptorSetLayout(vk::raii::Device const& device) {
  std::array layoutBindings{
    vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr},
    vk::DescriptorSetLayoutBinding{1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr},
    vk::DescriptorSetLayoutBinding{2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr},
    vk::DescriptorSetLayoutBinding{3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr},
  };

  vk::DescriptorSetLayoutCreateInfo layoutInfo{
    .bindingCount = static_cast<uint32_t>(layoutBindings.size()), .pBindings = layoutBindings.data()
  };
  descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
}

void ComputeImageRenderer::createComputePipeline(vk::raii::Device const& device) {
  vk::raii::ShaderModule shaderModule = ShaderLoader::createShaderModule(device, std::string(SHADER_DIR) + "/compute_image.spv");

  vk::PipelineShaderStageCreateInfo computeShaderStageInfo{
    .stage = vk::ShaderStageFlagBits::eCompute, .module = shaderModule, .pName = "compMain"
  };
  vk::PipelineLayoutCreateInfo pipelineLayoutInfo{.setLayoutCount = 1, .pSetLayouts = &*descriptorSetLayout};
  computePipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);
  vk::ComputePipelineCreateInfo pipelineInfo{.stage = computeShaderStageInfo, .layout = *computePipelineLayout};
  computePipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
}

void ComputeImageRenderer::populateWorld() {
  hittablesData.clear();
  materialsData.clear();

  std::mt19937 randomEngine{1};
  std::uniform_real_distribution<precision_type> unitDistribution{0.0, 1.0};
  auto random = [&]() { return unitDistribution(randomEngine); };
  auto randomRange = [&](precision_type min, precision_type max) { return min + (max - min) * random(); };
  auto randomColor = [&]() { return glm::vec<3, precision_type>{random(), random(), random()}; };
  auto randomColorRange = [&](precision_type min, precision_type max) {
    return glm::vec<3, precision_type>{
      randomRange(min, max),
      randomRange(min, max),
      randomRange(min, max),
    };
  };

  RtCpuBuilder builder(hittablesData, materialsData);

  const uint32_t groundMaterial = builder.addLambertian({0.5, 0.5, 0.5});
  builder.addStaticSphere({0, -1000, 0}, 1000, groundMaterial);

  for (int a = -11; a < 11; a++) {
    for (int b = -11; b < 11; b++) {
      const precision_type chooseMat = random();
      const glm::vec<3, precision_type> center{
        a + 0.9 * random(),
        0.2,
        b + 0.9 * random(),
      };

      if (glm::length(center - glm::vec<3, precision_type>{4, 0.2, 0}) > 0.9) {
        if (chooseMat < 0.8) {
          const auto albedo = randomColor() * randomColor();
          auto center2 = center + glm::vec<3, precision_type>{0, randomRange(0, .5), 0};
          builder.addMovingSphere(center, center2, 0.2, builder.addLambertian(albedo));
        } else if (chooseMat < 0.95) {
          const auto albedo = randomColorRange(0.5, 1.0);
          const precision_type fuzz = randomRange(0, 0.5);
          builder.addStaticSphere(center, 0.2, builder.addMetal(albedo, fuzz));
        } else {
          builder.addStaticSphere(center, 0.2, builder.addDielectric(1.5));
        }
      }
    }
  }

  builder.addStaticSphere({0, 1, 0}, 1.0, builder.addDielectric(1.5));
  builder.addStaticSphere({-4, 1, 0}, 1.0, builder.addLambertian({0.4, 0.2, 0.1}));
  builder.addStaticSphere({4, 1, 0}, 1.0, builder.addMetal({0.7, 0.6, 0.5}, 0.0));

  BvhBuilder(hittablesData).build();

  hittableBufferSize = sizeof(Hittable) * hittablesData.size();
  materialBufferSize = sizeof(Material) * materialsData.size();
}
void ComputeImageRenderer::createBuffers(GpuResources& gpuResources, vk::Extent2D const& extent) {
  for (auto& pixelBuffer : pixelBuffers) {
    gpuResources.destroyBuffer(pixelBuffer);
  }
  for (auto& hittableBuffer : hittableBuffers) {
    gpuResources.destroyBuffer(hittableBuffer);
  }
  for (auto& materialBuffer : materialBuffers) {
    gpuResources.destroyBuffer(materialBuffer);
  }
  renderExtent = extent;
  const size_t pixelCount = static_cast<size_t>(renderExtent.width) * static_cast<size_t>(renderExtent.height);
  pixelBufferSize = sizeof(uint32_t) * pixelCount;

  populateWorld();

  pixelBuffers.clear();
  hittableBuffers.clear();
  materialBuffers.clear();
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vk::Buffer pixelBuffer;
    gpuResources.createDeviceLocalBuffer(
      pixelBuffer,
      pixelBufferSize,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
    );
    pixelBuffers.push_back(pixelBuffer);

    vk::Buffer hittableBuffer;
    gpuResources.createDeviceLocalBuffer(
      hittableBuffer,
      hittableBufferSize,
      hittablesData.data(),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
    );
    hittableBuffers.push_back(hittableBuffer);

    vk::Buffer materialBuffer;
    gpuResources.createDeviceLocalBuffer(
      materialBuffer,
      materialBufferSize,
      materialsData.data(),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
    );
    materialBuffers.push_back(materialBuffer);
  }
}

void ComputeImageRenderer::createDescriptorSets(
  vk::raii::Device const& device, vk::raii::DescriptorPool const& descriptorPool, std::vector<vk::Buffer> const& uniformBuffers
) {
  std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
  vk::DescriptorSetAllocateInfo allocInfo{
    .descriptorPool = descriptorPool,
    .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
    .pSetLayouts = layouts.data(),
  };
  descriptorSets.clear();
  descriptorSets = device.allocateDescriptorSets(allocInfo);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vk::DescriptorBufferInfo bufferInfo{uniformBuffers[i], 0, sizeof(UniformBufferObject)};
    vk::DescriptorBufferInfo pixelBufferInfo{pixelBuffers[i], 0, pixelBufferSize};
    vk::DescriptorBufferInfo hittableBufferInfo{hittableBuffers[i], 0, hittableBufferSize};
    vk::DescriptorBufferInfo materialBufferInfo{materialBuffers[i], 0, materialBufferSize};
    std::array descriptorWrites{
      vk::WriteDescriptorSet{
        .dstSet = *descriptorSets[i],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .pBufferInfo = &bufferInfo,
      },
      vk::WriteDescriptorSet{
        .dstSet = *descriptorSets[i],
        .dstBinding = 1,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .pBufferInfo = &pixelBufferInfo,
      },
      vk::WriteDescriptorSet{
        .dstSet = *descriptorSets[i],
        .dstBinding = 2,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .pBufferInfo = &hittableBufferInfo,
      },
      vk::WriteDescriptorSet{
        .dstSet = *descriptorSets[i],
        .dstBinding = 3,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .pBufferInfo = &materialBufferInfo,
      },
    };
    device.updateDescriptorSets(descriptorWrites, {});
  }
}

vk::Buffer ComputeImageRenderer::pixelBuffer(uint32_t currentFrame) const { return pixelBuffers[currentFrame]; }

vk::DeviceSize ComputeImageRenderer::pixelOutputSize() const { return pixelBufferSize; }

void ComputeImageRenderer::recordComputeDispatch(vk::raii::CommandBuffer& commandBuffer, uint32_t currentFrame) {
  commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline);
  commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, computePipelineLayout, 0, {descriptorSets[currentFrame]}, {});
  commandBuffer.dispatch((renderExtent.width + 15) / 16, (renderExtent.height + 15) / 16, 1);
}

void ComputeImageRenderer::recordCopyToSwapChain(
  vk::raii::CommandBuffer& commandBuffer, uint32_t currentFrame, vk::Extent2D const& swapChainExtent, vk::Image swapChainImage
) {
  vk::BufferImageCopy copyRegion{
    .bufferOffset = 0,
    .bufferRowLength = 0,
    .bufferImageHeight = 0,
    .imageSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1},
    .imageOffset = {0, 0, 0},
    .imageExtent = {swapChainExtent.width, swapChainExtent.height, 1}
  };
  commandBuffer.copyBufferToImage(pixelBuffers[currentFrame], swapChainImage, vk::ImageLayout::eTransferDstOptimal, copyRegion);
}
