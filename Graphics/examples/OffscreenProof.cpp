#include <vulkan/vulkan.h>

#include <nanovg.h>
#include <nanovg_vk_api.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace {

constexpr uint32_t kWidth = 256;
constexpr uint32_t kHeight = 256;
constexpr VkFormat kColorFormat = VK_FORMAT_R8G8B8A8_UNORM;
constexpr VkFormat kDepthFormat = VK_FORMAT_D24_UNORM_S8_UINT;

void check(VkResult result, const char* what) {
  if (result != VK_SUCCESS) {
    throw std::runtime_error(std::string(what) + " failed");
  }
}

uint32_t findMemoryType(VkPhysicalDevice gpu, uint32_t filter, VkMemoryPropertyFlags flags) {
  VkPhysicalDeviceMemoryProperties properties{};
  vkGetPhysicalDeviceMemoryProperties(gpu, &properties);
  for (uint32_t i = 0; i < properties.memoryTypeCount; i++) {
    if ((filter & (1 << i)) && (properties.memoryTypes[i].propertyFlags & flags) == flags) return i;
  }
  throw std::runtime_error("no memory type");
}

struct Image {
  VkImage image = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkImageView view = VK_NULL_HANDLE;
};

Image createImage(VkPhysicalDevice gpu, VkDevice device, VkFormat format, VkImageUsageFlags usage,
                  VkImageAspectFlags aspect) {
  Image out;
  VkImageCreateInfo info{
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = format,
      .extent = {kWidth, kHeight, 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = usage,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };
  check(vkCreateImage(device, &info, nullptr, &out.image), "vkCreateImage");

  VkMemoryRequirements req{};
  vkGetImageMemoryRequirements(device, out.image, &req);
  VkMemoryAllocateInfo alloc{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = req.size,
      .memoryTypeIndex = findMemoryType(gpu, req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
  };
  check(vkAllocateMemory(device, &alloc, nullptr, &out.memory), "vkAllocateMemory");
  vkBindImageMemory(device, out.image, out.memory, 0);

  VkImageViewCreateInfo viewInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = out.image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = format,
      .subresourceRange = {aspect, 0, 1, 0, 1},
  };
  check(vkCreateImageView(device, &viewInfo, nullptr, &out.view), "vkCreateImageView");
  return out;
}

void writePpm(const char* path, const std::vector<uint8_t>& rgba) {
  FILE* file = std::fopen(path, "wb");
  std::fprintf(file, "P6\n%u %u\n255\n", kWidth, kHeight);
  for (uint32_t i = 0; i < kWidth * kHeight; i++) {
    std::fputc(rgba[i * 4 + 0], file);
    std::fputc(rgba[i * 4 + 1], file);
    std::fputc(rgba[i * 4 + 2], file);
  }
  std::fclose(file);
}

}

int main() {
  VkApplicationInfo appInfo{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, .apiVersion = VK_API_VERSION_1_2};
  VkInstanceCreateInfo instanceInfo{.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pApplicationInfo = &appInfo};
  VkInstance instance = VK_NULL_HANDLE;
  check(vkCreateInstance(&instanceInfo, nullptr, &instance), "vkCreateInstance");

  uint32_t gpuCount = 0;
  vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr);
  std::vector<VkPhysicalDevice> gpus(gpuCount);
  vkEnumeratePhysicalDevices(instance, &gpuCount, gpus.data());
  VkPhysicalDevice gpu = gpus.at(0);

  uint32_t familyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, nullptr);
  std::vector<VkQueueFamilyProperties> families(familyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, families.data());
  uint32_t graphicsFamily = 0;
  for (uint32_t i = 0; i < familyCount; i++) {
    if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      graphicsFamily = i;
      break;
    }
  }

  uint32_t deviceExtCount = 0;
  vkEnumerateDeviceExtensionProperties(gpu, nullptr, &deviceExtCount, nullptr);
  std::vector<VkExtensionProperties> deviceExts(deviceExtCount);
  vkEnumerateDeviceExtensionProperties(gpu, nullptr, &deviceExtCount, deviceExts.data());
  bool hasExtendedDynamicState = false;
  for (const auto& extension : deviceExts) {
    if (std::strcmp(extension.extensionName, VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME) == 0) {
      hasExtendedDynamicState = true;
    }
  }

  std::vector<const char*> enabledDeviceExts;
  if (hasExtendedDynamicState) enabledDeviceExts.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);

  VkPhysicalDeviceExtendedDynamicStateFeaturesEXT dynamicStateFeatures{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
      .extendedDynamicState = VK_TRUE,
  };

  const float priority = 1.0f;
  VkDeviceQueueCreateInfo queueInfo{
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = graphicsFamily,
      .queueCount = 1,
      .pQueuePriorities = &priority,
  };
  VkDeviceCreateInfo deviceInfo{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = hasExtendedDynamicState ? &dynamicStateFeatures : nullptr,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &queueInfo,
      .enabledExtensionCount = static_cast<uint32_t>(enabledDeviceExts.size()),
      .ppEnabledExtensionNames = enabledDeviceExts.data(),
  };
  VkDevice device = VK_NULL_HANDLE;
  check(vkCreateDevice(gpu, &deviceInfo, nullptr, &device), "vkCreateDevice");
  std::printf("extended_dynamic_state: %s\n", hasExtendedDynamicState ? "enabled" : "unavailable");

  VkQueue queue = VK_NULL_HANDLE;
  vkGetDeviceQueue(device, graphicsFamily, 0, &queue);

  Image color = createImage(gpu, device, kColorFormat,
                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                            VK_IMAGE_ASPECT_COLOR_BIT);
  Image depth = createImage(gpu, device, kDepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                            VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);

  const std::array<VkAttachmentDescription, 2> attachments = {
      VkAttachmentDescription{
          .format = kColorFormat,
          .samples = VK_SAMPLE_COUNT_1_BIT,
          .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
          .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
          .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      },
      VkAttachmentDescription{
          .format = kDepthFormat,
          .samples = VK_SAMPLE_COUNT_1_BIT,
          .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
          .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      },
  };
  VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
  VkAttachmentReference depthRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
  VkSubpassDescription subpass{
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorRef,
      .pDepthStencilAttachment = &depthRef,
  };
  VkRenderPassCreateInfo renderPassInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = static_cast<uint32_t>(attachments.size()),
      .pAttachments = attachments.data(),
      .subpassCount = 1,
      .pSubpasses = &subpass,
  };
  VkRenderPass renderPass = VK_NULL_HANDLE;
  check(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass), "vkCreateRenderPass");

  const std::array<VkImageView, 2> framebufferViews = {color.view, depth.view};
  VkFramebufferCreateInfo framebufferInfo{
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass = renderPass,
      .attachmentCount = static_cast<uint32_t>(framebufferViews.size()),
      .pAttachments = framebufferViews.data(),
      .width = kWidth,
      .height = kHeight,
      .layers = 1,
  };
  VkFramebuffer framebuffer = VK_NULL_HANDLE;
  check(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffer), "vkCreateFramebuffer");

  VkCommandPoolCreateInfo poolInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = graphicsFamily,
  };
  VkCommandPool pool = VK_NULL_HANDLE;
  check(vkCreateCommandPool(device, &poolInfo, nullptr, &pool), "vkCreateCommandPool");

  std::array<VkCommandBuffer, 1> commandBuffers{};
  VkCommandBufferAllocateInfo cmdAlloc{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };
  check(vkAllocateCommandBuffers(device, &cmdAlloc, commandBuffers.data()), "vkAllocateCommandBuffers");

  uint32_t currentFrame = 0;
  VKNVGCreateInfo nvgInfo{};
  nvgInfo.gpu = gpu;
  nvgInfo.device = device;
  nvgInfo.renderpass = renderPass;
  nvgInfo.cmdBuffer = commandBuffers.data();
  nvgInfo.queueFamilyIndex = graphicsFamily;
  nvgInfo.currentFrame = &currentFrame;
  nvgInfo.swapchainImageCount = 1;
  nvgInfo.ext.dynamicState = hasExtendedDynamicState;
  NVGcontext* vg = nvgCreateVkApi(nvgInfo, NVG_ANTIALIAS | NVG_STENCIL_STROKES, queue);
  if (vg == nullptr) throw std::runtime_error("nvgCreateVkApi returned null");

  VkCommandBuffer cmd = commandBuffers[0];
  VkCommandBufferBeginInfo begin{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  vkBeginCommandBuffer(cmd, &begin);

  std::array<VkClearValue, 2> clears{};
  clears[0].color = {{0.10f, 0.10f, 0.12f, 1.0f}};
  clears[1].depthStencil = {1.0f, 0};
  VkRenderPassBeginInfo pass{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = renderPass,
      .framebuffer = framebuffer,
      .renderArea = {{0, 0}, {kWidth, kHeight}},
      .clearValueCount = static_cast<uint32_t>(clears.size()),
      .pClearValues = clears.data(),
  };
  vkCmdBeginRenderPass(cmd, &pass, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport{
      .x = 0.0f,
      .y = 0.0f,
      .width = static_cast<float>(kWidth),
      .height = static_cast<float>(kHeight),
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
  };
  vkCmdSetViewport(cmd, 0, 1, &viewport);
  VkRect2D scissor{{0, 0}, {kWidth, kHeight}};
  vkCmdSetScissor(cmd, 0, 1, &scissor);

  nvgBeginFrame(vg, kWidth, kHeight, 1.0f);

  nvgBeginPath(vg);
  nvgRect(vg, 40, 40, 120, 80);
  nvgFillColor(vg, nvgRGBA(220, 80, 60, 255));
  nvgFill(vg);

  nvgBeginPath(vg);
  nvgCircle(vg, 170, 170, 50);
  nvgFillColor(vg, nvgRGBA(60, 160, 230, 255));
  nvgFill(vg);

  nvgBeginPath(vg);
  nvgMoveTo(vg, 30, 220);
  nvgLineTo(vg, 230, 150);
  nvgStrokeColor(vg, nvgRGBA(240, 220, 40, 255));
  nvgStrokeWidth(vg, 6);
  nvgStroke(vg);

  nvgEndFrame(vg);

  vkCmdEndRenderPass(cmd);
  vkEndCommandBuffer(cmd);

  VkSubmitInfo submit{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmd,
  };
  vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);

  VkBufferCreateInfo bufferInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = static_cast<VkDeviceSize>(kWidth) * kHeight * 4,
      .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };
  VkBuffer readback = VK_NULL_HANDLE;
  check(vkCreateBuffer(device, &bufferInfo, nullptr, &readback), "vkCreateBuffer");
  VkMemoryRequirements bufReq{};
  vkGetBufferMemoryRequirements(device, readback, &bufReq);
  VkMemoryAllocateInfo bufAlloc{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = bufReq.size,
      .memoryTypeIndex = findMemoryType(gpu, bufReq.memoryTypeBits,
                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
  };
  VkDeviceMemory readbackMemory = VK_NULL_HANDLE;
  check(vkAllocateMemory(device, &bufAlloc, nullptr, &readbackMemory), "vkAllocateMemory readback");
  vkBindBufferMemory(device, readback, readbackMemory, 0);

  vkResetCommandBuffer(cmd, 0);
  vkBeginCommandBuffer(cmd, &begin);
  VkBufferImageCopy region{
      .imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
      .imageExtent = {kWidth, kHeight, 1},
  };
  vkCmdCopyImageToBuffer(cmd, color.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback, 1, &region);
  vkEndCommandBuffer(cmd);
  vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);

  void* mapped = nullptr;
  vkMapMemory(device, readbackMemory, 0, bufferInfo.size, 0, &mapped);
  std::vector<uint8_t> pixels(bufferInfo.size);
  std::memcpy(pixels.data(), mapped, bufferInfo.size);
  vkUnmapMemory(device, readbackMemory);

  writePpm("nanovg_offscreen.ppm", pixels);

  uint64_t nonBackground = 0;
  for (uint32_t i = 0; i < kWidth * kHeight; i++) {
    const uint8_t r = pixels[i * 4 + 0];
    const uint8_t g = pixels[i * 4 + 1];
    const uint8_t b = pixels[i * 4 + 2];
    if (r > 40 || g > 40 || b > 40) nonBackground++;
  }
  std::printf("rendered %llu / %u non-background pixels\n",
              static_cast<unsigned long long>(nonBackground), kWidth * kHeight);

  nvgDeleteVkApi(vg);
  return nonBackground > 1000 ? 0 : 1;
}
