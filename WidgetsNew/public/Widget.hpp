#pragma once

#include "SpringAnimations.hpp"
#include "LayoutManager.hpp"

#include "EventHandler.hpp"
#include "Graphics.hpp"

#include <vector>
#include <functional>

namespace tp {
	class WidgetManagerInterface;
	class WidgetLayout;
	class LayoutManager;
	class RootWidget;

	class Widget {
		friend RootWidget;
		friend LayoutManager;

	public:
		Widget();
		virtual ~Widget();

		void addChild(Widget* child, bool front = false);
		void removeChild(Widget* child);

		void bringToFront();
		void bringToBack();

		void setLayout(WidgetLayout* layout);
		void setSizePolicy(SizePolicy x, SizePolicy y) { getLayout()->setSizePolicy(x, y); }

		WidgetLayout* getLayout();
		const WidgetLayout* getLayout() const;

		void triggerWidgetUpdate(const char* reason = nullptr);

	protected:
		virtual void process(const EventHandler& events) {}
		virtual void draw(Canvas& canvas) {}
		virtual void drawOverlay(Canvas& canvas) {}
		virtual void endAnimations();
		virtual void updateAnimations();

		[[nodiscard]] virtual bool processesEvents() const;
		[[nodiscard]] virtual bool propagateEventsToChildren() const;
		[[nodiscard]] virtual bool needsNextFrame() const;

		virtual void mouseEnter();
		virtual void mouseLeave();

	protected:
		void setDebug(const char* name, RGBA col);
		WidgetManagerInterface* getRoot();

	public:
		[[nodiscard]] RectF getArea() const;
		[[nodiscard]] RectF getAreaT() const;
		// [[nodiscard]] const RectF& getAreaCache() const;

		[[nodiscard]] RectF getRelativeArea() const;
		[[nodiscard]] RectF getRelativeAreaT() const;
		// [[nodiscard]] RectF getRelativeAreaCache() const;

		void setArea(const RectF& area);
		void setAreaCache(const RectF& area);

	private:
		using DFSAction = std::function<void(Widget*)>;

		static void dfs(Widget* iter, const DFSAction& before, const DFSAction& after = [](auto){}) {
			if (!iter->mNeedsProcessing) return;

			before(iter);

			for (auto child : iter->mDepthOrder) {
				dfs(child.data(), before, after);
			}

			after(iter);
		}

	protected:
		friend WidgetLayout;

		Widget* mParent = nullptr;

		std::vector<Widget*> mChildren;
		List<Widget*> mDepthOrder;

		// relative to the parent
		SpringRect mArea;
		RectF mAreaCache;

		WidgetLayout* mLayout = nullptr;

		// TODO : make bitfield processing flags
		bool mInFocus = false;
		bool mNeedsProcessing = false;
		bool mTriggered = false;

		// debug
		struct {
			std::string id = "widget base";
			RGBA col = { 1, 1, 1, 0.3 };
			std::string triggerReason = "none";
			Vec2F pLocal;
			Vec2F pGlobal;
		} mDebug;
	};

	struct WidgetManagerInterface : public Widget {
		virtual void updateWidget(Widget*, const char* reason) = 0;
	};
}