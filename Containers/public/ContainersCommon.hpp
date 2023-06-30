
#pragma once

#include "Module.hpp"

namespace tp {
	extern ModuleManifest gModuleContainers;

	class DefaultAllocator {
	public:
		DefaultAllocator() = default;
		static void *allocate(ualni);
		static void deallocate(void *);
	};

	class DefaultSaverLoader {
	public:
		DefaultSaverLoader() = default;

		template<typename Type>
		static void write(const Type&) {}

		template<typename Type>
		static void read(Type&) {}
	};
}