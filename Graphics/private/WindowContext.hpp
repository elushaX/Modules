#pragma once

#include "Window.hpp"

#include "VulkanContext.hpp"
#include "PlatformWindow.hpp"

#include <memory>

namespace tp {

	class Window::Context {
	public:
		std::unique_ptr<PlatformWindow> platform;
		VulkanContext* vulkan = nullptr;
	};
}
