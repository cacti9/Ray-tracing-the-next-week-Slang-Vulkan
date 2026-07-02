#include "renderer.h"
#include "vma_raii.h"
#include <algorithm>
#include <assert.h>
#include <stdexcept>
#include <unordered_map>
#include <vulkan/vulkan_raii.hpp>

void VulkanRenderer::initWindow() {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  window = glfwCreateWindow(WIDTH, HEIGHT, APP_NAME, nullptr, nullptr);
  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
  glfwSetKeyCallback(window, keyCallback);
}

void VulkanRenderer::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
  auto app = static_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
  app->framebufferResized = true;
}

void VulkanRenderer::mainLoop() {
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    drawFrame();
  }

  device.waitIdle();
}

void VulkanRenderer::cleanup() {
  glfwDestroyWindow(window);
  glfwTerminate();
}

void VulkanRenderer::createInstance() {
  // Validation layers are now managed by vulkanconfig instead of being hard-coded
  constexpr vk::ApplicationInfo appInfo{
    .pApplicationName = APP_NAME,
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName = "No Engine",
    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion = vk::ApiVersion14
  };

  auto requiredExtensions = getRequiredExtensions();

  vk::InstanceCreateInfo createInfo{
    .pApplicationInfo = &appInfo,
    .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
    .ppEnabledExtensionNames = requiredExtensions.data()
  };
  instance = vk::raii::Instance(context, createInfo);
}

[[nodiscard]] std::vector<const char*> VulkanRenderer::getRequiredExtensions() const {
  // Get the required extensions from GLFW
  uint32_t glfwExtensionCount = 0;
  auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

  // Check if the debug utils extension is available
  std::vector<vk::ExtensionProperties> props = context.enumerateInstanceExtensionProperties();
  bool debugUtilsAvailable = std::ranges::any_of(props, [](vk::ExtensionProperties const& ep) {
    return strcmp(ep.extensionName, vk::EXTDebugUtilsExtensionName) == 0;
  });

  // Always include the debug utils extension if available
  // This allows validation layers to be enabled via vulkanconfig
  if (debugUtilsAvailable) {
    extensions.push_back(vk::EXTDebugUtilsExtensionName);
  } else {
    std::cout << "VK_EXT_debug_utils extension not available. Validation layers may not work." << std::endl;
  }

  return extensions;
}

void VulkanRenderer::setupDebugMessenger() {
  // Always set up the debug messenger
  // It will only be used if validation layers are enabled via vulkanconfig

  vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
    vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
    vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
  );

  vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
    vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
    vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
  );

  vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
    .messageSeverity = severityFlags, .messageType = messageTypeFlags, .pfnUserCallback = &debugCallback
  };

  try {
    debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
  } catch (vk::SystemError& err) {
    // If the debug utils extension is not available, this will fail
    // That's okay; it just means validation layers aren't enabled
    std::cout << "Debug messenger not available. Validation layers may not be enabled." << std::endl;
  }
}
VKAPI_ATTR vk::Bool32 VKAPI_CALL VulkanRenderer::debugCallback(
  vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type,
  const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*
) {
  if (
    severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError || severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
  ) {
    std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;
  }

  return vk::False;
}

void VulkanRenderer::createSurface() {
  VkSurfaceKHR _surface;
  if (glfwCreateWindowSurface(*instance, window, nullptr, &_surface) != 0) {
    throw std::runtime_error("failed to create window surface!");
  }
  surface = vk::raii::SurfaceKHR(instance, _surface);
}

void VulkanRenderer::pickPhysicalDevice() {
  std::vector<vk::raii::PhysicalDevice> devices = instance.enumeratePhysicalDevices();
  const auto devIter = std::ranges::find_if(devices, [&](auto const& device) {
    // Check if the device supports the Vulkan 1.3 API version
    bool supportsVulkan1_3 = device.getProperties().apiVersion >= VK_API_VERSION_1_3;

    // Check if any of the queue families support graphics operations
    auto queueFamilies = device.getQueueFamilyProperties();
    bool supportsGraphics =
      std::ranges::any_of(queueFamilies, [](auto const& qfp) { return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics); });

    // Check if all required device extensions are available
    auto availableDeviceExtensions = device.enumerateDeviceExtensionProperties();
    bool supportsAllRequiredExtensions =
      std::ranges::all_of(requiredDeviceExtension, [&availableDeviceExtensions](auto const& requiredDeviceExtension) {
        return std::ranges::any_of(availableDeviceExtensions, [requiredDeviceExtension](auto const& availableDeviceExtension) {
          return strcmp(availableDeviceExtension.extensionName, requiredDeviceExtension) == 0;
        });
      });

    auto features = device.template getFeatures2<
      vk::PhysicalDeviceFeatures2,
      vk::PhysicalDeviceVulkan11Features,
      vk::PhysicalDeviceVulkan13Features,
      vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
      vk::PhysicalDeviceTimelineSemaphoreFeaturesKHR>();
    bool supportsRequiredFeatures =
      features.template get<vk::PhysicalDeviceFeatures2>().features.shaderFloat64 &&
      features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
      features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
      features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState &&
      features.template get<vk::PhysicalDeviceTimelineSemaphoreFeaturesKHR>().timelineSemaphore;

    return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
  });
  if (devIter != devices.end()) {
    physicalDevice = *devIter;

    // Print device information
    vk::PhysicalDeviceProperties deviceProperties = physicalDevice.getProperties();
    std::cout << "Selected GPU: " << deviceProperties.deviceName << std::endl;
    std::cout << "API Version: " << VK_VERSION_MAJOR(deviceProperties.apiVersion) << "."
              << VK_VERSION_MINOR(deviceProperties.apiVersion) << "." << VK_VERSION_PATCH(deviceProperties.apiVersion) << std::endl;

    // check vma optimization possibilities
    auto availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
    for (auto vmaAskedExtension : vma_raii::vmaAskedExtensions) {
      bool available = std::ranges::any_of(availableDeviceExtensions, [vmaAskedExtension](auto const& availableDeviceExtension) {
        return strcmp(availableDeviceExtension.extensionName, vmaAskedExtension) == 0;
      });
      if (available) {
        requiredDeviceExtension.push_back(vmaAskedExtension);
        vmaAvailableFlags |= vma_raii::vmaFlagFromExtension[vmaAskedExtension];
      }
    }
  } else {
    throw std::runtime_error("failed to find a suitable GPU!");
  }
}

void VulkanRenderer::createLogicalDevice() {
  std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

  // get the first index into queueFamilyProperties which supports both graphics and present
  for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++) {
    if (
      (queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
      (queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eCompute) &&
      physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface)
    ) {
      // found a queue family that supports both graphics and present
      queueIndex = qfpIndex;
      break;
    }
  }
  if (queueIndex == ~0) {
    throw std::runtime_error("Could not find a queue for graphics, compute, and present -> terminating");
  }

  // query for required features (Vulkan 1.1 and 1.3)
  vk::StructureChain<
    vk::PhysicalDeviceFeatures2,
    vk::PhysicalDeviceVulkan11Features,
    vk::PhysicalDeviceVulkan13Features,
    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
    vk::PhysicalDeviceMaintenance5Features,
    vk::PhysicalDeviceBufferDeviceAddressFeatures,
    vk::PhysicalDeviceMemoryPriorityFeaturesEXT,
    vk::PhysicalDeviceTimelineSemaphoreFeaturesKHR>
    featureChain = {
      // vk::PhysicalDeviceFeatures2
      {.features = {.shaderFloat64 = true}},
      // vk::PhysicalDeviceVulkan11Features
      {.shaderDrawParameters = true},
      // vk::PhysicalDeviceVulkan13Features
      {.synchronization2 = true,
       .dynamicRendering = true,
       .maintenance4 = static_cast<bool>(vmaAvailableFlags & VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE4_BIT)},
      // vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
      {.extendedDynamicState = true},
      // vk::PhysicalDeviceMaintenance5Features,
      {.maintenance5 = static_cast<bool>(vmaAvailableFlags & VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE5_BIT)},
      // vk::PhysicalDeviceBufferDeviceAddressFeatures,
      {.bufferDeviceAddress = static_cast<bool>(vmaAvailableFlags & VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT)},
      // vk::PhysicalDeviceMemoryPriorityFeaturesEXT,
      {.memoryPriority = static_cast<bool>(vmaAvailableFlags & VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT)},
      // vk::PhysicalDeviceTimelineSemaphoreFeaturesKHR
      {.timelineSemaphore = true}
    };

  // create a Device
  float queuePriority = 0.0f;
  vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
    .queueFamilyIndex = queueIndex, .queueCount = 1, .pQueuePriorities = &queuePriority
  };
  vk::DeviceCreateInfo deviceCreateInfo{
    .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
    .queueCreateInfoCount = 1,
    .pQueueCreateInfos = &deviceQueueCreateInfo,
    .enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtension.size()),
    .ppEnabledExtensionNames = requiredDeviceExtension.data()
  };

  device = vk::raii::Device(physicalDevice, deviceCreateInfo);
  allInOneQueue = vk::raii::Queue(device, queueIndex, 0);
}

void VulkanRenderer::createSwapChain() {
  auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
  swapChainExtent = chooseSwapExtent(surfaceCapabilities);
  swapChainSurfaceFormat = chooseSwapSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(*surface));
  if (!(surfaceCapabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferDst)) {
    throw std::runtime_error("swapchain images do not support transfer destination usage");
  }
  if (swapChainSurfaceFormat.format != vk::Format::eB8G8R8A8Srgb) {
    throw std::runtime_error("compute gradient path expects a BGRA8 sRGB swapchain format");
  }
  vk::SwapchainCreateInfoKHR swapChainCreateInfo{
    .surface = *surface,
    .minImageCount = chooseSwapMinImageCount(surfaceCapabilities),
    .imageFormat = swapChainSurfaceFormat.format,
    .imageColorSpace = swapChainSurfaceFormat.colorSpace,
    .imageExtent = swapChainExtent,
    .imageArrayLayers = 1,
    .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
    .imageSharingMode = vk::SharingMode::eExclusive,
    .preTransform = surfaceCapabilities.currentTransform,
    .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
    .presentMode = chooseSwapPresentMode(physicalDevice.getSurfacePresentModesKHR(*surface)),
    .clipped = true
  };

  swapChain = vk::raii::SwapchainKHR(device, swapChainCreateInfo);
  swapChainImages = swapChain.getImages();
}
uint32_t VulkanRenderer::chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities) {
  auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
  if ((0 < surfaceCapabilities.maxImageCount) && (surfaceCapabilities.maxImageCount < minImageCount)) {
    minImageCount = surfaceCapabilities.maxImageCount;
  }
  return minImageCount;
}
vk::SurfaceFormatKHR VulkanRenderer::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
  assert(!availableFormats.empty());
  const auto formatIt = std::ranges::find_if(availableFormats, [](const auto& format) {
    return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
  });
  return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
}
vk::PresentModeKHR VulkanRenderer::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
  assert(std::ranges::any_of(availablePresentModes, [](auto presentMode) { return presentMode == vk::PresentModeKHR::eFifo; }));
  return std::ranges::any_of(
           availablePresentModes, [](const vk::PresentModeKHR value) { return vk::PresentModeKHR::eMailbox == value; }
         )
           ? vk::PresentModeKHR::eMailbox
           : vk::PresentModeKHR::eFifo;
}
vk::Extent2D VulkanRenderer::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
  if (capabilities.currentExtent.width != 0xFFFFFFFF) {
    return capabilities.currentExtent;
  }
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);

  return {
    std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
    std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
  };
}

void VulkanRenderer::createSwapChainImageViews() {
  assert(swapChainImageViews.empty());

  for (uint32_t i = 0; i < swapChainImages.size(); i++) {
    vk::ImageViewCreateInfo viewInfo{
      .image = swapChainImages[i],
      .viewType = vk::ImageViewType::e2D,
      .format = swapChainSurfaceFormat.format,
      .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
    };
    swapChainImageViews.emplace_back(device, viewInfo);
  }
}

void VulkanRenderer::createCommandPool() {
  vk::CommandPoolCreateInfo poolInfo{.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer, .queueFamilyIndex = queueIndex};
  commandPool = vk::raii::CommandPool(device, poolInfo);
}

void VulkanRenderer::createUniformBuffers() {
  uniformBuffersMapped.clear();
  uniformBuffers.clear();
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    auto [buf, map] = gpuResources->createMappedBuffer(
      sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
    );
    uniformBuffersMapped.push_back(map);
    uniformBuffers.push_back(buf);
  }
}

void VulkanRenderer::createDescriptorPool() {
  const uint32_t descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
  std::array poolSize{
    vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, descriptorSetCount},
    vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, descriptorSetCount * 3},
  };
  vk::DescriptorPoolCreateInfo poolInfo{
    .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
    .maxSets = descriptorSetCount,
    .poolSizeCount = static_cast<uint32_t>(poolSize.size()),
    .pPoolSizes = poolSize.data()
  };
  descriptorPool = vk::raii::DescriptorPool(device, poolInfo);
}

void VulkanRenderer::createFrameCommandBuffers() {
  frameCommandBuffers.clear();
  vk::CommandBufferAllocateInfo allocInfo{
    .commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = MAX_FRAMES_IN_FLIGHT
  };
  frameCommandBuffers = vk::raii::CommandBuffers(device, allocInfo);
}

void VulkanRenderer::createSyncObjects() {
  inFlightFences.clear();

  vk::SemaphoreTypeCreateInfo semaphoreType{.semaphoreType = vk::SemaphoreType::eTimeline, .initialValue = 0};
  semaphore = vk::raii::Semaphore(device, {.pNext = &semaphoreType});
  timelineValue = 0;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vk::FenceCreateInfo fenceInfo{};
    inFlightFences.emplace_back(device, fenceInfo);
  }
}
