#include <vulkan/vulkan.h>

#define NANOVG_VULKAN_IMPLEMENTATION
#include "nanovg_vk.h"

#include "UnitTest++/UnitTest++.h"

#include <cstdlib>
#include <cstring>

namespace {
	struct SlotContext {
		VKNVGcontext vk{};

		~SlotContext() { std::free(vk.textures); }

		int external() {
			VKNVGtexture* texture = vknvg_allocTexture(&vk);
			texture->flags = NVGVK_IMAGE_EXTERNAL;
			return vknvg_textureId(&vk, texture);
		}

		int owned() {
			VKNVGtexture* texture = vknvg_allocTexture(&vk);
			texture->image = reinterpret_cast<VkImage>(static_cast<uintptr_t>(0x1000 + vk.ntextures));
			return vknvg_textureId(&vk, texture);
		}
	};
}

SUITE(NanovgTextureSlots) {
	TEST(ExternalTexturesKeepTheirSlot) {
		SlotContext context;
		const int canvas = context.external();
		const int thumbnail = context.owned();

		CHECK(canvas != thumbnail);
		CHECK(vknvg_findTexture(&context.vk, canvas)->flags & NVGVK_IMAGE_EXTERNAL);
	}

	TEST(DeletedSlotsAreReusedAndOldIdsResolveToNothing) {
		SlotContext context;
		const int first = context.external();
		const int second = context.external();

		vknvg_deleteTexture(&context.vk, vknvg_findTexture(&context.vk, first));
		CHECK(vknvg_findTexture(&context.vk, first) == nullptr);
		CHECK(vknvg_findTexture(&context.vk, second) != nullptr);

		CHECK_EQUAL(first, context.external());
	}

	TEST(UnknownIdsResolveToNothing) {
		SlotContext context;
		CHECK(vknvg_findTexture(&context.vk, 0) == nullptr);
		CHECK(vknvg_findTexture(&context.vk, 5) == nullptr);
	}
}
