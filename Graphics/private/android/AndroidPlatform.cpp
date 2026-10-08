#include "AndroidPlatform.hpp"

#include <android/api-level.h>
#include <android/asset_manager.h>
#include <android/input.h>
#include <android/log.h>
#include <android/native_activity.h>
#include <android_native_app_glue.h>
#include <jni.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <vector>

namespace tp::android {

	namespace {
		android_app* gApplication = nullptr;

		struct AttachedEnv {
			explicit AttachedEnv(ANativeActivity* activity) :
				vm(activity->vm) {
				if (vm->GetEnv((void**) &env, JNI_VERSION_1_6) == JNI_EDETACHED) {
					vm->AttachCurrentThread(&env, nullptr);
					attached = true;
				}
			}

			~AttachedEnv() {
				if (attached) vm->DetachCurrentThread();
			}

			JavaVM* vm;
			JNIEnv* env = nullptr;
			bool attached = false;
		};

		void appendUtf8(std::string& out, uint32_t codepoint) {
			if (codepoint < 0x80) {
				out += (char) codepoint;
			} else if (codepoint < 0x800) {
				out += (char) (0xC0 | (codepoint >> 6));
				out += (char) (0x80 | (codepoint & 0x3F));
			} else if (codepoint < 0x10000) {
				out += (char) (0xE0 | (codepoint >> 12));
				out += (char) (0x80 | ((codepoint >> 6) & 0x3F));
				out += (char) (0x80 | (codepoint & 0x3F));
			} else {
				out += (char) (0xF0 | (codepoint >> 18));
				out += (char) (0x80 | ((codepoint >> 12) & 0x3F));
				out += (char) (0x80 | ((codepoint >> 6) & 0x3F));
				out += (char) (0x80 | (codepoint & 0x3F));
			}
		}
	}

	android_app* application() { return gApplication; }

	void setApplication(android_app* app) { gApplication = app; }

	std::string unicodeText(android_app& app, const AInputEvent* event) {
		AttachedEnv attached(app.activity);
		JNIEnv* env = attached.env;
		if (!env) return {};

		jclass keyEventClass = env->FindClass("android/view/KeyEvent");
		jmethodID constructor = env->GetMethodID(keyEventClass, "<init>", "(JJIIIIIIII)V");
		jmethodID getUnicodeChar = env->GetMethodID(keyEventClass, "getUnicodeChar", "(I)I");
		jobject keyEvent = env->NewObject(
			keyEventClass,
			constructor,
			(jlong) AKeyEvent_getDownTime(event),
			(jlong) AKeyEvent_getEventTime(event),
			(jint) AKeyEvent_getAction(event),
			(jint) AKeyEvent_getKeyCode(event),
			(jint) AKeyEvent_getRepeatCount(event),
			(jint) AKeyEvent_getMetaState(event),
			(jint) AInputEvent_getDeviceId(event),
			(jint) AKeyEvent_getScanCode(event),
			(jint) AKeyEvent_getFlags(event),
			(jint) AInputEvent_getSource(event)
		);
		const jint codepoint = env->CallIntMethod(keyEvent, getUnicodeChar, (jint) AKeyEvent_getMetaState(event));
		env->DeleteLocalRef(keyEvent);
		env->DeleteLocalRef(keyEventClass);

		std::string text;
		if (codepoint > 0 && codepoint >= 0x20 && codepoint != 0x7F) appendUtf8(text, (uint32_t) codepoint);
		return text;
	}

	void setSoftKeyboardVisible(android_app& app, bool visible) {
		AttachedEnv attached(app.activity);
		JNIEnv* env = attached.env;
		if (!env) return;

		jobject activity = app.activity->clazz;
		jclass activityClass = env->GetObjectClass(activity);
		jclass contextClass = env->FindClass("android/content/Context");
		jfieldID serviceField = env->GetStaticFieldID(contextClass, "INPUT_METHOD_SERVICE", "Ljava/lang/String;");
		jobject serviceName = env->GetStaticObjectField(contextClass, serviceField);
		jmethodID getSystemService =
			env->GetMethodID(activityClass, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
		jobject inputManager = env->CallObjectMethod(activity, getSystemService, serviceName);

		jmethodID getWindow = env->GetMethodID(activityClass, "getWindow", "()Landroid/view/Window;");
		jobject window = env->CallObjectMethod(activity, getWindow);
		jclass windowClass = env->FindClass("android/view/Window");
		jmethodID getDecorView = env->GetMethodID(windowClass, "getDecorView", "()Landroid/view/View;");
		jobject decorView = env->CallObjectMethod(window, getDecorView);

		jclass managerClass = env->FindClass("android/view/inputmethod/InputMethodManager");
		if (visible) {
			jmethodID show = env->GetMethodID(managerClass, "showSoftInput", "(Landroid/view/View;I)Z");
			env->CallBooleanMethod(inputManager, show, decorView, 0);
		} else {
			jclass viewClass = env->FindClass("android/view/View");
			jmethodID getToken = env->GetMethodID(viewClass, "getWindowToken", "()Landroid/os/IBinder;");
			jobject token = env->CallObjectMethod(decorView, getToken);
			jmethodID hide = env->GetMethodID(managerClass, "hideSoftInputFromWindow", "(Landroid/os/IBinder;I)Z");
			env->CallBooleanMethod(inputManager, hide, token, 0);
			env->DeleteLocalRef(token);
			env->DeleteLocalRef(viewClass);
		}
		if (env->ExceptionCheck()) env->ExceptionClear();

		env->DeleteLocalRef(managerClass);
		env->DeleteLocalRef(decorView);
		env->DeleteLocalRef(windowClass);
		env->DeleteLocalRef(window);
		env->DeleteLocalRef(inputManager);
		env->DeleteLocalRef(serviceName);
		env->DeleteLocalRef(contextClass);
		env->DeleteLocalRef(activityClass);
	}

	namespace {
		struct InsetEdges {
			jobject bars = nullptr;
			jobject gestures = nullptr;
		};

		InsetEdges typedInsetEdges(JNIEnv* env, jobject insets, jclass insetsClass) {
			jclass typeClass = env->FindClass("android/view/WindowInsets$Type");
			const auto typeMask = [&](const char* name) {
				return env->CallStaticIntMethod(typeClass, env->GetStaticMethodID(typeClass, name, "()I"));
			};
			const jint bars = typeMask("systemBars") | typeMask("displayCutout");
			const jint gestures = typeMask("mandatorySystemGestures");
			env->DeleteLocalRef(typeClass);

			jmethodID getInsetsOf = env->GetMethodID(insetsClass, "getInsets", "(I)Landroid/graphics/Insets;");
			return { env->CallObjectMethod(insets, getInsetsOf, bars), env->CallObjectMethod(insets, getInsetsOf, gestures) };
		}

		InsetEdges legacyInsetEdges(JNIEnv* env, jobject insets, jclass insetsClass) {
			const auto edgesOf = [&](const char* getter) {
				return env->CallObjectMethod(insets, env->GetMethodID(insetsClass, getter, "()Landroid/graphics/Insets;"));
			};
			return { edgesOf("getSystemWindowInsets"), edgesOf("getMandatorySystemGestureInsets") };
		}
	}

	EdgeInsets systemInsets(android_app& app) {
		AttachedEnv attached(app.activity);
		JNIEnv* env = attached.env;
		if (!env) return {};

		EdgeInsets result;
		jobject activity = app.activity->clazz;
		jclass activityClass = env->GetObjectClass(activity);
		jmethodID getWindow = env->GetMethodID(activityClass, "getWindow", "()Landroid/view/Window;");
		jobject window = env->CallObjectMethod(activity, getWindow);
		jclass windowClass = env->FindClass("android/view/Window");
		jmethodID getDecorView = env->GetMethodID(windowClass, "getDecorView", "()Landroid/view/View;");
		jobject decorView = window ? env->CallObjectMethod(window, getDecorView) : nullptr;
		jclass viewClass = env->FindClass("android/view/View");
		jmethodID getInsets = env->GetMethodID(viewClass, "getRootWindowInsets", "()Landroid/view/WindowInsets;");
		jobject insets = decorView ? env->CallObjectMethod(decorView, getInsets) : nullptr;

		if (insets) {
			jclass insetsClass = env->FindClass("android/view/WindowInsets");
			const InsetEdges sources = android_get_device_api_level() >= 30 ? typedInsetEdges(env, insets, insetsClass)
																																			 : legacyInsetEdges(env, insets, insetsClass);
			jclass edgesClass = env->FindClass("android/graphics/Insets");
			const auto edge = [&](jobject edges, const char* name) {
				return edges ? (halnf) env->GetIntField(edges, env->GetFieldID(edgesClass, name, "I")) : 0.0f;
			};
			const auto both = [&](const char* name) { return std::max(edge(sources.bars, name), edge(sources.gestures, name)); };
			result = { both("left"), both("top"), both("right"), both("bottom") };

			env->DeleteLocalRef(edgesClass);
			if (sources.gestures) env->DeleteLocalRef(sources.gestures);
			if (sources.bars) env->DeleteLocalRef(sources.bars);
			env->DeleteLocalRef(insetsClass);
			env->DeleteLocalRef(insets);
		}
		if (env->ExceptionCheck()) env->ExceptionClear();

		env->DeleteLocalRef(viewClass);
		if (decorView) env->DeleteLocalRef(decorView);
		env->DeleteLocalRef(windowClass);
		if (window) env->DeleteLocalRef(window);
		env->DeleteLocalRef(activityClass);
		return result;
	}

	bool openUrl(android_app& app, const std::string& url) {
		AttachedEnv attached(app.activity);
		JNIEnv* env = attached.env;
		if (!env || url.empty()) return false;

		jclass uriClass = env->FindClass("android/net/Uri");
		jmethodID parse = env->GetStaticMethodID(uriClass, "parse", "(Ljava/lang/String;)Landroid/net/Uri;");
		jstring text = env->NewStringUTF(url.c_str());
		jobject uri = env->CallStaticObjectMethod(uriClass, parse, text);

		jclass intentClass = env->FindClass("android/content/Intent");
		jmethodID constructor = env->GetMethodID(intentClass, "<init>", "(Ljava/lang/String;Landroid/net/Uri;)V");
		jstring action = env->NewStringUTF("android.intent.action.VIEW");
		jobject intent = env->NewObject(intentClass, constructor, action, uri);
		jmethodID addFlags = env->GetMethodID(intentClass, "addFlags", "(I)Landroid/content/Intent;");
		constexpr jint kNewTask = 0x10000000;
		env->DeleteLocalRef(env->CallObjectMethod(intent, addFlags, kNewTask));

		jobject activity = app.activity->clazz;
		jclass activityClass = env->GetObjectClass(activity);
		jmethodID startActivity = env->GetMethodID(activityClass, "startActivity", "(Landroid/content/Intent;)V");
		env->CallVoidMethod(activity, startActivity, intent);
		const bool failed = env->ExceptionCheck();
		if (failed) env->ExceptionClear();

		env->DeleteLocalRef(activityClass);
		env->DeleteLocalRef(intent);
		env->DeleteLocalRef(action);
		env->DeleteLocalRef(intentClass);
		if (uri) env->DeleteLocalRef(uri);
		env->DeleteLocalRef(text);
		env->DeleteLocalRef(uriClass);
		return !failed;
	}

	namespace {
		void
		extractDirectory(AAssetManager* manager, const std::string& assetDirectory, const std::filesystem::path& target) {
			AAssetDir* directory = AAssetManager_openDir(manager, assetDirectory.c_str());
			if (!directory) return;
			std::filesystem::create_directories(target);
			while (const char* name = AAssetDir_getNextFileName(directory)) {
				const std::string assetPath = assetDirectory.empty() ? name : assetDirectory + "/" + name;
				AAsset* asset = AAssetManager_open(manager, assetPath.c_str(), AASSET_MODE_STREAMING);
				if (!asset) continue;
				std::ofstream out(target / name, std::ios::binary | std::ios::trunc);
				std::vector<char> buffer(64 * 1024);
				int read = 0;
				while ((read = AAsset_read(asset, buffer.data(), buffer.size())) > 0)
					out.write(buffer.data(), read);
				AAsset_close(asset);
			}
			AAssetDir_close(directory);
		}
	}

	void extractAssets(android_app& app, const char* directory) {
		AAssetManager* manager = app.activity->assetManager;
		const std::filesystem::path target = std::filesystem::path(app.activity->internalDataPath) / directory;
		extractDirectory(manager, directory, target);
		__android_log_print(ANDROID_LOG_INFO, "modules", "assets extracted to %s", target.c_str());
	}
}
