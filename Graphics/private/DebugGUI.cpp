#include "Window.hpp"
#include "WindowContext.hpp"

#include "Graphics.hpp"
#include "ImGuiPlatform.hpp"

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_internal.h>

#include "implot.h"

#include <array>
#include <memory>
#include <stdexcept>

namespace tp {
	class DebugGUI::Context {
	public:
		ImGuiIO* io{};
		ImGuiContext* ctx{};
		VulkanContext* vulkan{};
		VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
		std::unique_ptr<ImGuiPlatform> platform;
	};
}

using namespace tp;

static VkDescriptorPool createImguiDescriptorPool(VkDevice device) {
	const std::array<VkDescriptorPoolSize, 1> poolSizes = {
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 64 },
	};

	VkDescriptorPoolCreateInfo poolInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = 64,
		.poolSizeCount = (uint32_t) poolSizes.size(),
		.pPoolSizes = poolSizes.data(),
	};

	VkDescriptorPool pool = VK_NULL_HANDLE;
	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create imgui descriptor pool");
	}
	return pool;
}

DebugGUI::DebugGUI(Window* window) {
	mContext = new Context();
	mContext->vulkan = window->getContext()->vulkan;

	IMGUI_CHECKVERSION();
	mContext->ctx = ImGui::CreateContext();
	mContext->io = &ImGui::GetIO();

	mContext->platform = std::make_unique<ImGuiPlatform>(*window);

	mContext->descriptorPool = createImguiDescriptorPool(mContext->vulkan->device());

	ImGui_ImplVulkan_InitInfo initInfo{};
	initInfo.Instance = mContext->vulkan->instance();
	initInfo.PhysicalDevice = mContext->vulkan->physicalDevice();
	initInfo.Device = mContext->vulkan->device();
	initInfo.Queue = mContext->vulkan->graphicsQueue();
	initInfo.DescriptorPool = mContext->descriptorPool;
	initInfo.RenderPass = mContext->vulkan->renderPass();
	initInfo.MinImageCount = mContext->vulkan->framesInFlight();
	initInfo.ImageCount = mContext->vulkan->swapchainImageCount();
	initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&initInfo);

	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF("rsc/Font.ttf", 18.f);

	io.ConfigInputTrickleEventQueue = false;

	// appearance
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImPlot::CreateContext();

	return;

	auto& colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };

	// Headers
	colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
	colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
	colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

	// Buttons
	colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
	colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
	colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

	// Frame BG
	colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
	colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
	colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

	// Tabs
	colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
	colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
	colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
	colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

	// Title
	colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
	colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
}

DebugGUI::~DebugGUI() {
	mContext->vulkan->waitIdle();

	ImPlot::DestroyContext();

	ImGui_ImplVulkan_Shutdown();
	mContext->platform.reset();
	ImGui::DestroyContext();

	vkDestroyDescriptorPool(mContext->vulkan->device(), mContext->descriptorPool, nullptr);

	delete mContext;
}

void DebugGUI::procBegin() {}

void DebugGUI::procEnd() {}

void DebugGUI::drawBegin() {
	ImGui_ImplVulkan_NewFrame();
	mContext->platform->newFrame();
	ImGui::NewFrame();
}

void DebugGUI::drawEnd() {
	ImGui::Render();
	mContext->platform->endFrame();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), mContext->vulkan->activeCommandBuffer());
}