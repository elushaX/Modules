
#include "Environment.hpp"

#include <iostream>

const char* ArchString[] = { "UNDEF", "INTEL", "ARM" };
const char* BuildTypeString[] = { "UNDEF", "DEBUG", "RELEASE" };
const char* ToolchainString[] = { "UNDEF", "GNU", "LLVM", "MSVC" };
const char* OSString[] = { "UNDEF", "LINUX", "WINDOWS", "ANDROID", "IOS" };
const char* ArchWidthString[] = { "UNDEF", "X64", "X32" };


const tp::Environment tp::gEnvironment;

void tp::Environment::log() const {
	std::cout << "ARCH       : " << ArchString[(int)mArch] << "\n";
	std::cout << "WIDTH      : " << ArchWidthString[(int)mWidth] << "\n";
	std::cout << "CURRENT OS : " << OSString[(int)mOS] << "\n";
	std::cout << "TOOLCHAIN  : " << ToolchainString[(int)mToolchain] << "\n";
	std::cout << "BUILD TYPE : " << BuildTypeString[(int)mBuildType] << "\n";
}