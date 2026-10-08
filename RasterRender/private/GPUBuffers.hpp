#pragma once

#include "Scene.hpp"
#include "VulkanContext.hpp"

#include <vulkan/vulkan.h>

#include <cstring>
#include <stdexcept>

namespace tp {

	class ObjectBuffers : public GPUBuffers {
	public:
		explicit ObjectBuffers(Object* object) : mObject(object) {
			mContext = VulkanContext::current();
			if (!mContext) throw std::runtime_error("ObjectBuffers: no active VulkanContext");

			const auto& points = mObject->mTopology.Points;
			const auto& indices = mObject->mTopology.Indexes;

			mIndexCount = (uint32_t) indices.size() * 3;

			createBuffer(sizeof(Vec3F) * points.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
						 points.getBuff(), mVertexBuffer, mVertexMemory);
			createBuffer(sizeof(Vec3<uhalni>) * indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
						 indices.getBuff(), mIndexBuffer, mIndexMemory);
		}

		~ObjectBuffers() override {
			const VkDevice device = mContext->device();
			mContext->waitIdle();
			if (mVertexBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, mVertexBuffer, nullptr);
			if (mVertexMemory != VK_NULL_HANDLE) vkFreeMemory(device, mVertexMemory, nullptr);
			if (mIndexBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, mIndexBuffer, nullptr);
			if (mIndexMemory != VK_NULL_HANDLE) vkFreeMemory(device, mIndexMemory, nullptr);
		}

		void drawCall(void* commandBuffer) override {
			if (mIndexCount == 0) return;

			auto cmd = static_cast<VkCommandBuffer>(commandBuffer);
			const VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(cmd, 0, 1, &mVertexBuffer, &offset);
			vkCmdBindIndexBuffer(cmd, mIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(cmd, mIndexCount, 1, 0, 0, 0);
		}

		Object* mObject = nullptr;

	private:
		void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, const void* data,
						  VkBuffer& buffer, VkDeviceMemory& memory) {
			if (size == 0) return;

			const VkDevice device = mContext->device();

			VkBufferCreateInfo bufferInfo{
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = size,
				.usage = usage,
				.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			};
			if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
				throw std::runtime_error("ObjectBuffers: failed to create buffer");
			}

			VkMemoryRequirements requirements{};
			vkGetBufferMemoryRequirements(device, buffer, &requirements);

			VkMemoryAllocateInfo allocateInfo{
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
				.allocationSize = requirements.size,
				.memoryTypeIndex = mContext->findMemoryType(
					requirements.memoryTypeBits,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
			};
			if (vkAllocateMemory(device, &allocateInfo, nullptr, &memory) != VK_SUCCESS) {
				throw std::runtime_error("ObjectBuffers: failed to allocate buffer memory");
			}
			vkBindBufferMemory(device, buffer, memory, 0);

			void* mapped = nullptr;
			vkMapMemory(device, memory, 0, size, 0, &mapped);
			std::memcpy(mapped, data, static_cast<size_t>(size));
			vkUnmapMemory(device, memory);
		}

		VulkanContext* mContext = nullptr;
		VkBuffer mVertexBuffer = VK_NULL_HANDLE;
		VkDeviceMemory mVertexMemory = VK_NULL_HANDLE;
		VkBuffer mIndexBuffer = VK_NULL_HANDLE;
		VkDeviceMemory mIndexMemory = VK_NULL_HANDLE;
		uint32_t mIndexCount = 0;
	};
}
