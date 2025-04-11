/*
MIT License

Copyright (c) 2025 alx-home

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include "Window.h"
#ifdef _WIN32

#   include <Windows.h>
#   include <shellapi.h>

#   include <string_view>

namespace win32 {

class SystemTray {
   // Construction/destruction
public:
   static uint32_t s__uid;

   SystemTray(
      std::string_view tool_tip,
      HICON            icon   = 0,
      bool             hidden = true,
      uint32_t         uid    = ++s__uid
   );

   virtual ~SystemTray();

   // Operations
   bool Visible() const { return !hidden_; }

   bool             SetTooltipText(std::string_view text);
   bool             SetTooltipText(uint32_t uid);
   std::string_view GetTooltipText() const;

   // Change or retrieve the icon displayed
   bool  SetIcon(HICON icon);
   bool  SetIcon(std::string_view name);
   bool  SetIcon(uint32_t resource_id);
   bool  SetStandardIcon(std::string_view name);
   bool  SetStandardIcon(uint32_t resource_id);
   HICON GetIcon() const;

   bool HideIcon();
   bool ShowIcon();
   bool AddIcon();
   bool RemoveIcon();
   bool MoveToRight();

protected:
   virtual LRESULT OnMessageImpl(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

   uint32_t GetUid() const;
   HWND     GetHandle() const;

private:
   void           InstallIconPending();
   WinPtr         message_window_{nullptr, DestroyWindow};
   NOTIFYICONDATA notify_data_{};
   bool           hidden_{true};
   bool           removed_{true};
   uint32_t       default_menu_id_{0};
   bool           default_menu_item_by_pos_{true};
   bool           show_icon_pending_{true};
   uint32_t       saved_icon_{0};
   uint32_t       creation_flags_{0};

   HWND target_window_{nullptr};

   static uint32_t const TASKBAR_CREATED_MSG;
   static HWND           s__message_window;

   // static void GetTrayWndRect(LPRECT lprect);

   LRESULT OnMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
   // Generated message map functions
   LRESULT OnTaskbarCreated(WPARAM wParam, LPARAM lParam);
   // Default handler for tray notification message
   virtual LRESULT OnTrayNotification(WPARAM uID, LPARAM lEvent) = 0;

   template <String, message_handler SELF>
   friend constexpr WinPtr CreateMessageWindow(SELF&, HINSTANCE);
};

}  // namespace win32

#endif