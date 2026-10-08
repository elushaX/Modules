#pragma once

#include "Updates.hpp"

struct android_app;

namespace tp::sketch2d {

	class AndroidUpdateBackend : public UpdateBackend {
	public:
		explicit AndroidUpdateBackend(android_app& app);
		~AndroidUpdateBackend() override;

		[[nodiscard]] int8 installedVersionCode() override;
		[[nodiscard]] std::optional<std::string> fetchText(const std::string& url) override;
		[[nodiscard]] std::optional<std::string>
		download(const Release& release, const std::function<void(int8, int8)>& onProgress) override;
		[[nodiscard]] std::optional<std::string> install() override;
		void wakeMainThread() override;

	private:
		android_app& mApp;
		void* mUpdater = nullptr;
	};
}
