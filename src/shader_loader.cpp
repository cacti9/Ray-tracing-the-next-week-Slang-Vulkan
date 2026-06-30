#include "renderer_types.h"

#include <fstream>
#include <stdexcept>

std::vector<char> ShaderLoader::readFile(const std::string& filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("failed to open file!");
  }
  std::vector<char> buffer(file.tellg());
  file.seekg(0, std::ios::beg);
  file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
  file.close();
  return buffer;
}

[[nodiscard]] vk::raii::ShaderModule ShaderLoader::createShaderModule(vk::raii::Device const& device, const std::string& filename) {
  const std::vector<char>& code = readFile(filename);
  vk::ShaderModuleCreateInfo createInfo{.codeSize = code.size(), .pCode = reinterpret_cast<const uint32_t*>(code.data())};
  vk::raii::ShaderModule shaderModule{device, createInfo};

  return shaderModule;
}
