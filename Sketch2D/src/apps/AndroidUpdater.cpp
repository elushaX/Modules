#include "AndroidUpdater.hpp"

#include <android/looper.h>
#include <android_native_app_glue.h>
#include <jni.h>

namespace tp::sketch2d {

	namespace {
		struct AttachedEnv {
			explicit AttachedEnv(JavaVM* vm) :
				vm(vm) {
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

		using ProgressCallback = std::function<void(int8, int8)>;

		void reportProgress(JNIEnv*, jclass, jlong handle, jlong received, jlong total) {
			const auto* callback = reinterpret_cast<const ProgressCallback*>(handle);
			if (callback && *callback) (*callback)((int8) received, (int8) total);
		}

		std::string text(JNIEnv* env, jstring value) {
			if (!value) return {};
			const char* characters = env->GetStringUTFChars(value, nullptr);
			std::string result = characters;
			env->ReleaseStringUTFChars(value, characters);
			return result;
		}

		std::optional<std::string> errorOrNothing(JNIEnv* env, jstring value) {
			if (env->ExceptionCheck()) {
				env->ExceptionClear();
				return std::string("The updater failed unexpectedly.");
			}
			if (!value) return std::nullopt;
			std::string message = text(env, value);
			env->DeleteLocalRef(value);
			return message;
		}
	}

	AndroidUpdateBackend::AndroidUpdateBackend(android_app& app) :
		mApp(app) {
		AttachedEnv attached(app.activity->vm);
		JNIEnv* env = attached.env;
		jobject activity = app.activity->clazz;
		jclass activityClass = env->GetObjectClass(activity);
		jmethodID getClassLoader = env->GetMethodID(activityClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
		jobject loader = env->CallObjectMethod(activity, getClassLoader);
		jclass loaderClass = env->FindClass("java/lang/ClassLoader");
		jmethodID loadClass = env->GetMethodID(loaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
		jstring name = env->NewStringUTF("online.ilushabasic.sketch2d.Updater");
		auto updater = (jclass) env->CallObjectMethod(loader, loadClass, name);
		if (env->ExceptionCheck()) {
			env->ExceptionClear();
			updater = nullptr;
		}
		if (updater) {
			mUpdater = (jclass) env->NewGlobalRef(updater);
			const JNINativeMethod natives[] = { { "onProgress", "(JJJ)V", (void*) reportProgress } };
			if (env->RegisterNatives(updater, natives, 1) != JNI_OK) env->ExceptionClear();
		}

		env->DeleteLocalRef(name);
		if (updater) env->DeleteLocalRef(updater);
		env->DeleteLocalRef(loaderClass);
		env->DeleteLocalRef(loader);
		env->DeleteLocalRef(activityClass);
	}

	AndroidUpdateBackend::~AndroidUpdateBackend() {
		if (!mUpdater) return;
		AttachedEnv attached(mApp.activity->vm);
		attached.env->DeleteGlobalRef((jobject) mUpdater);
	}

	int8 AndroidUpdateBackend::installedVersionCode() {
		if (!mUpdater) return 0;
		AttachedEnv attached(mApp.activity->vm);
		JNIEnv* env = attached.env;
		auto updater = (jclass) mUpdater;
		jmethodID method = env->GetStaticMethodID(updater, "installedVersionCode", "(Landroid/app/Activity;)J");
		const jlong code = env->CallStaticLongMethod(updater, method, mApp.activity->clazz);
		if (env->ExceptionCheck()) env->ExceptionClear();
		return (int8) code;
	}

	std::optional<std::string> AndroidUpdateBackend::fetchText(const std::string& url) {
		if (!mUpdater) return std::nullopt;
		AttachedEnv attached(mApp.activity->vm);
		JNIEnv* env = attached.env;
		auto updater = (jclass) mUpdater;
		jmethodID method = env->GetStaticMethodID(updater, "fetchText", "(Ljava/lang/String;)Ljava/lang/String;");
		jstring address = env->NewStringUTF(url.c_str());
		auto result = (jstring) env->CallStaticObjectMethod(updater, method, address);
		env->DeleteLocalRef(address);
		if (env->ExceptionCheck()) {
			env->ExceptionClear();
			return std::nullopt;
		}
		if (!result) return std::nullopt;
		std::string body = text(env, result);
		env->DeleteLocalRef(result);
		return body;
	}

	std::optional<std::string>
	AndroidUpdateBackend::download(const Release& release, const std::function<void(int8, int8)>& onProgress) {
		if (!mUpdater) return std::string("Updates are not available in this build.");
		AttachedEnv attached(mApp.activity->vm);
		JNIEnv* env = attached.env;
		auto updater = (jclass) mUpdater;
		jmethodID method = env->GetStaticMethodID(
			updater, "download", "(Landroid/app/Activity;Ljava/lang/String;Ljava/lang/String;J)Ljava/lang/String;"
		);
		jstring address = env->NewStringUTF(release.apkUrl.c_str());
		jstring sha256 = env->NewStringUTF(release.sha256.c_str());
		const ProgressCallback callback = onProgress;
		auto result = (jstring) env->CallStaticObjectMethod(
			updater, method, mApp.activity->clazz, address, sha256, (jlong) reinterpret_cast<intptr_t>(&callback)
		);
		env->DeleteLocalRef(address);
		env->DeleteLocalRef(sha256);
		return errorOrNothing(env, result);
	}

	std::optional<std::string> AndroidUpdateBackend::install() {
		if (!mUpdater) return std::string("Updates are not available in this build.");
		AttachedEnv attached(mApp.activity->vm);
		JNIEnv* env = attached.env;
		auto updater = (jclass) mUpdater;
		jmethodID method = env->GetStaticMethodID(updater, "install", "(Landroid/app/Activity;)Ljava/lang/String;");
		auto result = (jstring) env->CallStaticObjectMethod(updater, method, mApp.activity->clazz);
		return errorOrNothing(env, result);
	}

	void AndroidUpdateBackend::wakeMainThread() { ALooper_wake(mApp.looper); }
}
