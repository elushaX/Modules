#pragma once

#include <vulkan/vulkan.h>

#include "nanovg.h"
#include "nanovg_vk.h"

#ifdef __cplusplus
extern "C" {
#endif

NVGcontext* nvgCreateVkApi(VKNVGCreateInfo createInfo, int flags, VkQueue queue);
void nvgDeleteVkApi(NVGcontext* ctx);

int nvgCreateImageFromVkImageApi(NVGcontext* ctx, VkImageView view, VkSampler sampler,
								 int width, int height);
void nvgUpdateVkImageApi(NVGcontext* ctx, int image, VkImageView view, VkSampler sampler,
						 int width, int height);

#ifdef __cplusplus
}
#endif
