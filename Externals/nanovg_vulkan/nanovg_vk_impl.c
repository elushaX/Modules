#include <vulkan/vulkan.h>

#define NANOVG_VULKAN_IMPLEMENTATION
#include "nanovg_vk.h"

#include "nanovg_vk_api.h"

NVGcontext* nvgCreateVkApi(VKNVGCreateInfo createInfo, int flags, VkQueue queue) {
  return nvgCreateVk(createInfo, flags, queue);
}

void nvgDeleteVkApi(NVGcontext* ctx) {
  nvgDeleteVk(ctx);
}

static VKNVGcontext* nvgVkContext(NVGcontext* ctx) {
  return (VKNVGcontext*) nvgInternalParams(ctx)->userPtr;
}

int nvgCreateImageFromVkImageApi(NVGcontext* ctx, VkImageView view, VkSampler sampler,
                                 int width, int height) {
  VKNVGcontext* vk = nvgVkContext(ctx);
  VKNVGtexture* tex = vknvg_allocTexture(vk);
  if (tex == NULL) return 0;

  tex->view = view;
  tex->sampler = sampler;
  tex->image = VK_NULL_HANDLE;
  tex->mem = VK_NULL_HANDLE;
  tex->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  tex->width = width;
  tex->height = height;
  tex->type = NVG_TEXTURE_RGBA;
  tex->flags = NVGVK_IMAGE_EXTERNAL;

  return vknvg_textureId(vk, tex);
}

void nvgUpdateVkImageApi(NVGcontext* ctx, int image, VkImageView view, VkSampler sampler,
                         int width, int height) {
  VKNVGcontext* vk = nvgVkContext(ctx);
  VKNVGtexture* tex = vknvg_findTexture(vk, image);
  if (tex == NULL) return;

  tex->view = view;
  tex->sampler = sampler;
  tex->image = VK_NULL_HANDLE;
  tex->mem = VK_NULL_HANDLE;
  tex->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  tex->width = width;
  tex->height = height;
  tex->flags = NVGVK_IMAGE_EXTERNAL;
}
