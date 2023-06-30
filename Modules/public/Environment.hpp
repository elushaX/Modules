
#pragma once

#include <climits>
#include <cfloat>

namespace tp {

	class Environment {
	public:

		enum class Arch { UNDEF, INTEL, ARM } mArch = Arch::UNDEF;

		// Build Type
		#if  defined(__DEBUG__) || defined(_DEBUG) || defined(DEBUG) || !defined(NDEBUG)
			#define ENV_BUILD_DEBUG
			enum class BuildType { UNDEF, DEBUG, RELEASE } mBuildType = BuildType::DEBUG;
		#else
			#define ENV_BUILD_RELEASE
			enum class BuildType { UNDEF, DEBUG, RELEASE } mBuildType = BuildType::RELEASE;
		#endif

		// MCVS
		#ifdef _MSC_VER
			#define ENV_COMPILER_MSVC
			enum class Toolchain { UNDEF, GNU, LLVM, MSVC } mToolchain = Toolchain::MSVC;

			// VERSION
			// TODO

			// TARGET OS
			#if defined(_WIN32) || defined(_WIN64)
				#define ENV_OS_WINDOWS
				enum class OS { UNDEF, LINUX, WINDOWS, ANDROID, IOS } mOS = OS::WINDOWS;
			#else
				enum class OS { UNDEF, LINUX, WINDOWS, ANDROID, IOS } mOS = OS::UNDEF;
				#error "unexplored compilation to os target"
			#endif

			// TARGET ALIGNED SIZE
			#ifdef  _WIN64
				enum class ArchWidth { UNDEF, X64, X32 } mWidth = ArchWidth::X64;
				#define ENV_BITS_64
			#else
				enum class ArchWidth { UNDEF, X64, X32 } mWidth = ArchWidth::X32;
				#define ENV_BITS_32
			#endif

		#endif

		// GCC
		#if defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
			#define ENV_COMPILER_GCC
			enum class Toolchain { UNDEF, GNU, LLVM, MSVC } mToolchain = Toolchain::GNU;

			// VERSION
			#if (__GNUC___ > 5 || (__GNUC__ == 5 && __GNUC_MINOR__ >= 1))
				// TODO
			#endif

			// TARGET OS
			#if defined(__linux__) && !defined(__ANDROID__)
				#define ENV_OS_LINUX
				enum class OS { UNDEF, LINUX, WINDOWS, ANDROID, IOS } mOS = OS::LINUX;
			#else
				#error "unexplored compilation to os target"
			#endif

			// TARGET ALIGNED SIZE
			#if defined(__aarch64__) || defined(__x86_64__) || defined(__ARM_64BIT_STATE)
				#define ENV_BITS_64
				enum class ArchWidth { UNDEF, X64, X32 } mWidth = ArchWidth::X64;
			#elif defined(__x86_64__) || defined(i386)
				#define ENV_BITS_32
				enum class ArchWidth { UNDEF, X64, X32 } mWidth = ArchWidth::X32;
			#endif

		#endif

		// CLANG
		#if defined(__clang__) && !defined(ENV_COMPILER_GCC)
			#define ENV_COMPILER_CLANG
			enum class Toolchain { UNDEF, GNU, LLVM, MSVC } mToolchain = Toolchain::LLVM;

			// VERSION
			#if (__clang_major__ > 5 || (__GNUC__ == 5 && __GNUC_MINOR__ >= 1))
				// TODO
			#endif

			// TARGET OS
			#if defined(__linux__) && !defined(__ANDROID__)
				#define ENV_OS_LINUX
				enum class OS { UNDEF, LINUX, WINDOWS, ANDROID, IOS } mOS = OS::LINUX;
			#elif defined(__ANDROID__)
				enum class OS { UNDEF, LINUX, WINDOWS, ANDROID, IOS } mOS = OS::ANDOID;
				#define ENV_OS_ANDROID
			#else
				#error "unexplored compilation to target os"
			#endif

			// TARGET ALIGNED SIZE
			#if defined(__aarch64__) || defined(__x86_64__) || defined(__ARM_64BIT_STATE)
				#define ENV_BITS_64
				enum class ArchWidth { UNDEF, X64, X32 } mWidth = ArchWidth::X64;
			#elif defined(__x86_64__) || defined(i386)
				#define ENV_BITS_32
				enum class ArchWidth { UNDEF, X64, X32 } mWidth = ArchWidth::X32;
			#endif

		#endif

		#if defined(__EMSCRIPTEN__) || defined(__MINGW32__) || defined(__MINGW32__) || defined(__MINGW64__)
		#error "compiler is not supported"
		#else
		#if !(defined(ENV_COMPILER_MSVC) || defined(ENV_COMPILER_CLANG) || defined(ENV_COMPILER_GCC))
						// Linux and Linux - derived           __linux__
				// Darwin(Mac OS X and iOS)         __APPLE__
				// Akaros     __ros__
				// NaCL                             __native_client__
				// AsmJS                            __asmjs__
				// Fuschia                         __Fuchsia__
				#error "unknown compiler"
		#endif
		#endif

		void log() const;
	};

	#ifndef ENV_BITS_64
		#error "ERROR - not 64 bit archytectures are out of support"
	#endif

	const extern Environment gEnvironment;

	typedef char int1;
	typedef unsigned char uint1;
	typedef short int2;
	typedef unsigned short uint2;
	typedef unsigned int uint4;
	typedef uint2 ufalni;
	typedef uint4 uhalni;
	typedef int int4;
	typedef int4 halni;
	typedef unsigned long long uint8;
	typedef uint8 ualni;
	typedef long long int8;
	typedef int8 alni;
	typedef double flt8;
	typedef flt8 alnf;
	typedef float flt4;
	typedef flt4 halnf;

	#define ENV_INT1_MAX tp::int1( 0x7f)
	#define ENV_INT1_MIN (-tp::int1(0x80))
	#define ENV_UINT1_MAX tp::uint1(0xff)
	#define ENV_UINT1_MIN tp::uint1(0x00)
	#define ENV_INT2_MAX tp::int2( 0x7fff)
	#define ENV_INT2_MIN (-tp::int2(0x8000))
	#define ENV_UINT2_MAX tp::uint2(0xffff)
	#define ENV_UINT2_MIN tp::uint2(0x0000)
	#define ENV_UHALNI_MAX tp::uhalni(0xffffffff)
	#define ENV_UHALNI_MIN tp::uhalni(0x00000000)
	#define ENV_HALNI_MAX tp::halni( 0x7fffffff)
	#define ENV_HALNI_MIN (-tp::halni(0x80000000))
	#define ENV_UALNI_MAX tp::ualni(0xffffffffffffffff)
	#define ENV_UALNI_MIN tp::ualni(0x0000000000000000)
	#define ENV_ALNI_MAX tp::alni( 0x7fffffffffffffff)
	#define ENV_ALNI_MIN (-tp::alni(0x8000000000000000))
	#define ENV_ALNI_SIZE_B (8)
	#define ENV_ALNF_DECIMAL_DIG  DBL_DECIMAL_DIG
	#define ENV_ALNF_DIG          DBL_DIG
	#define ENV_ALNF_EPSILON      DBL_EPSILON
	#define ENV_ALNF_HAS_SUBNORM  DBL_HAS_SUBNORM
	#define ENV_ALNF_MANT_DIG     DBL_MANT_DIG
	#define ENV_ALNF_MAX          DBL_MAX
	#define ENV_ALNF_MAX_10_EXP   DBL_MAX_10_EXP
	#define ENV_ALNF_MAX_EXP      DBL_MAX_EXP
	#define ENV_ALNF_MIN          DBL_MIN
	#define ENV_ALNF_MIN_10_EXP   DBL_MIN_10_EXP
	#define ENV_ALNF_MIN_EXP      DBL_MIN_EXP
	#define ENV_ALNF_RADIX       _DBL_RADIX
	#define ENV_ALNF_TRUE_MIN     DBL_TRUE_MIN
	#define ENV_HALNF_DECIMAL_DIG  FLT_DECIMAL_DIG
	#define ENV_HALNF_DIG          FLT_DIG        
	#define ENV_HALNF_EPSILON      FLT_EPSILON    
	#define ENV_HALNF_HAS_SUBNORM  FLT_HAS_SUBNORM
	#define ENV_HALNF_GUARD        FLT_GUARD      
	#define ENV_HALNF_MANT_DIG     FLT_MANT_DIG   
	#define ENV_HALNF_MAX          FLT_MAX        
	#define ENV_HALNF_MAX_10_EXP   FLT_MAX_10_EXP 
	#define ENV_HALNF_MAX_EXP      FLT_MAX_EXP    
	#define ENV_HALNF_MIN          FLT_MIN        
	#define ENV_HALNF_MIN_10_EXP   FLT_MIN_10_EXP 
	#define ENV_HALNF_MIN_EXP      FLT_MIN_EXP    
	#define ENV_HALNF_NORMALIZE    FLT_NORMALIZE  
	#define ENV_HALNF_RADIX        FLT_RADIX      
	#define ENV_HALNF_TRUE_MIN     FLT_TRUE_MIN
};