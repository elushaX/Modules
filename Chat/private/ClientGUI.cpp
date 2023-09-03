
#include "ClientGui.hpp"
#include "imgui.h"
#include "imgui_internal.h"

using namespace tp;
using namespace ImGui;

void ClientGUI::draw(ChatAPI& api) {
  DockSpaceOverViewport(GetWindowViewport());

  if (api.isLogged()) {
    drawChat(api);

  } else {
    if (Begin("Authentication")) {
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
    End();
  }
}

void ClientGUI::drawChat(tp::ChatAPI& api) {
  if (Begin("Chats")) {
    if (Button("Logout", {GetContentRegionAvail().x, 0})) {
      api.logout();
      mCredentials.pass[0] = 0;
      mActiveChatName = "Not Selected";
    }

    if (BeginListBox("##chats", GetContentRegionAvail())) {
      auto chats = api.getChatsList();
      if (chats) {
        for (auto user : *chats) {
          if (Selectable(user->read())) {
            mActiveChatName = user.data();
          }
        }
      } else {
        Text("Error");
      }

      EndListBox();
    }
  }
  End();

  if (Begin("History")) {
    Text("%s", mActiveChatName.read());

    auto history = api.getChatHistory(mActiveChatName);

    if (history) {
      if (BeginListBox("##History", GetContentRegionAvail())) {
        for (auto message : *history) {
          Text("%s : %s", message->user.read(), message->text.read());
          Separator();
        }
      }

      EndListBox();
    } else {
      Text("No History");
    }
  }
  End();

  if (Begin("Message")) {
    if (Button("Send")) {
      if (String::Logic::calcLength(mMessageBuff) > 0) {
        api.send(mActiveChatName, {mCredentials.name, mMessageBuff});
      }
      mMessageBuff[0] = 0;
    }

    InputTextMultiline("##Message", mMessageBuff, MESSAGE_SIZE, GetContentRegionAvail());
  }
  End();
}
