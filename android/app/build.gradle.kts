import java.util.Properties

plugins {
	id("com.android.application")
}

fun git(vararg arguments: String): String =
	providers.exec { commandLine("git", *arguments) }.standardOutput.asText.get().trim()

val commitCount = (findProperty("sketch2dVersionCode") as String?)?.toInt() ?: git("rev-list", "--count", "HEAD").toInt()
val commitHash = git("rev-parse", "--short", "HEAD")
val abis = (findProperty("sketch2dAbis") as String? ?: "arm64-v8a,x86_64").split(",")
val signingFile = file(System.getenv("SKETCH2D_SIGNING") ?: "${System.getProperty("user.home")}/.sketch2d-release/signing.properties")

android {
	namespace = "online.ilushabasic.sketch2d"
	compileSdk = 36
	ndkVersion = "29.0.14206865"

	defaultConfig {
		applicationId = "online.ilushabasic.sketch2d"
		minSdk = 29
		targetSdk = 36
		versionCode = commitCount
		versionName = "0.1.$commitCount ($commitHash)"

		ndk {
			abiFilters += abis
		}

		externalNativeBuild {
			cmake {
				targets += listOf("sketch2dAndroid", "widgetGallery")
				arguments += listOf(
					"-DANDROID_STL=c++_shared",
					"-DMODULES_BUILD_DESKTOP=OFF",
					"-DMODULES_BUILD_TESTS=OFF",
					"-DMODULES_BUILD_EXAMPLES=OFF",
					"-DCMAKE_POLICY_VERSION_MINIMUM=3.5",
				)
			}
		}
	}

	externalNativeBuild {
		cmake {
			path = file("../../CMakeLists.txt")
			version = "4.4.4"
		}
	}

	signingConfigs {
		create("release") {
			if (signingFile.exists()) {
				val signing = Properties().apply { signingFile.inputStream().use { load(it) } }
				storeFile = file(signing.getProperty("storeFile"))
				storePassword = signing.getProperty("storePassword")
				keyAlias = signing.getProperty("keyAlias")
				keyPassword = signing.getProperty("keyPassword")
			}
		}
	}

	buildTypes {
		release {
			isMinifyEnabled = false
			signingConfig = signingConfigs.getByName(if (signingFile.exists()) "release" else "debug")
		}
	}
}

androidComponents {
	onVariants { variant ->
		variant.sources.assets?.addStaticSourceDirectory(rootProject.file("assets").absolutePath)
	}
}
