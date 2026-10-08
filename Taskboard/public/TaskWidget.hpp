#pragma once

#include "AppStorage.hpp"
#include "Containers.hpp"
#include "Label.hpp"
#include "PageStack.hpp"
#include "SessionPanel.hpp"
#include "SessionPresets.hpp"
#include "TaskBoard.hpp"
#include "TaskPage.hpp"
#include "TaskPanels.hpp"
#include "Views.hpp"

#include <optional>

namespace tp::taskboard {

	[[nodiscard]] AppStorage taskboardStorage();

	enum class TaskWidgetPage : size_t { Task = 0, Session = 1, Workdir = 2, Links = 3 };

	class TaskWidget : public ui::Widget {
	public:
		TaskWidget(TaskBoard& board, ui::FileSystem& files, const AppStorage& storage);

		void showTask(std::optional<ui::NodeId> id);
		void setReadOnly(bool readOnly);
		void showPage(TaskWidgetPage page) { carousel.setCurrentIndex(size_t(page)); }
		[[nodiscard]] ui::Panel& body() { return mBody; }
		[[nodiscard]] TaskWidgetPage currentPage() const { return TaskWidgetPage(carousel.currentIndex()); }

	private:
		SessionPresets mPresets;
		ui::Widget mSessionBody;
		ui::Panel mBody{ ui::Panel::Surface::Base };

	public:
		ui::TabView carousel{ ui::TabView::Style::Carousel };
		TaskDescriptionPanel description;
		TaskWorkspacePanel workspace;
		ui::ScrollView sessionScroll{ ui::Axis::Vertical };
		SessionTab session;
		ui::Label sessionHint{ "Open a task to start or follow its agent session." };
		ui::PageStack taskPages;
		TaskPage page;

	private:
		void buildSessionPage();
		void buildTaskPage();
	};
}
