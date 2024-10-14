#pragma once

#include "SpringAnimations.hpp"

#include "Layout.hpp"

#include "EventHandler.hpp"
#include "Graphics.hpp"

#include <vector>
#include <functional>

namespace tp {
	class WidgetLayout;

	class LayoutManager;
	class UpdateManager;
	class DebugManager;

	class WidgetManagerInterface;
	class RootWidget;

	class Widget {
		friend RootWidget;

		friend LayoutManager;
		friend UpdateManager;
		friend DebugManager;

		using BitField = Bits<ualni>;

		using DFSAction = std::function<void(Widget*)>;

		enum Flags : int1 {
			ENABLED = 0,
			NEEDS_UPDATE,
			IN_FOCUS,
			TRIGGERED,
		};

	public:
		Widget();
		virtual ~Widget();

		void addChild(Widget* child, bool front = false);
		void removeChild(Widget* child);

		void bringToFront();
		void bringToBack();

		void setLayout(WidgetLayout* layout);
		void setSizePolicy(SizePolicy x, SizePolicy y);

		void setEnabled(bool val) { mFlags.set(ENABLED, val); }

		WidgetLayout* getLayout();
		[[nodiscard]] const WidgetLayout* getLayout() const;

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

		[[nodiscard]] RectF getRelativeArea() const;
		[[nodiscard]] RectF getRelativeAreaT() const;

		void setArea(const RectF& area);
		void setAreaCache(const RectF& area);

	private:
		[[nodiscard]] bool isUpdate() const { return mFlags.get(ENABLED) && mFlags.get(NEEDS_UPDATE); }
		[[nodiscard]] bool isDraw() const { return mFlags.get(ENABLED); }

		static void dfs(Widget* iter, const DFSAction& before, const DFSAction& after = [](auto){}) {
			if (!iter->isUpdate()) return;

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

		BitField mFlags;

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
		static WidgetLayout* defaultLayout(Widget* widget);
	};
}