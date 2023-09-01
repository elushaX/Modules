
#include "ClientGui.hpp"
#include "imgui.h"

using namespace tp;
using namespace ImGui;

void ClientGUI::draw(ChatAPI& api) {

	if (Begin("window_name")) {

		if (api.isLogged()) {

			static int item_current1 = 0;
			const char *items1[] = {"Never", "Gonna", "Give", "You", "Up"};
			Combo("User", &item_current1, items1, IM_ARRAYSIZE(items1));

			BeginListBox("History");
			Text("as");
			Text("asa");
			EndListBox();

			static char buff[1000];
			InputTextMultiline("Message", buff, 1000);

			if (Button("Send")) {

			}

		} else {

			InputText("Name", mCredentials.name, ChatAPI::Credentials::SIZE);
			InputText("Pass", mCredentials.pass, ChatAPI::Credentials::SIZE);

			if (mInvalidCredentials) {
				Text("Invalid Credentials");
			}

			if (Button("Login")) {
				api.login(mCredentials);
				mInvalidCredentials = !api.isLogged();
			}
		}

	} End();
}