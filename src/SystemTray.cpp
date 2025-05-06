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

#include <WinSock2.h>
#include <afxtempl.h>
#include <afxdisp.h>

#include "windows/SystemTray.h"
#include "windows/Window.h"

#include <string_view>
#include <utils/String.inl>

#include <libloaderapi.h>
#include <windef.h>
#include <winuser.h>

#include <cassert>
#include <cstdint>
#include <string>

namespace win32 {

HWND           SystemTray::s__message_window{nullptr};
uint32_t const SystemTray::TASKBAR_CREATED_MSG  = ::RegisterWindowMessage("TaskbarCreated");
uint32_t const SystemTray::TASKBAR_CALLBACK_MSG = ::RegisterWindowMessage("TaskbarCallback");

uint32_t SystemTray::s__uid = 0;

SystemTray::SystemTray(
  std::string_view                       tool_tip,
  std::optional<std::string_view> const& name,
  HICON                                  icon,
  bool                                   hidden,
  uint32_t                               uid
)
   : hidden_{hidden} {
   assert(tool_tip.size() < sizeof(notify_data_.szTip));

   // Create an invisible window
   message_window_ = CreateMessageWindow<"system_tray">(*this, name);

   notify_data_.cbSize           = sizeof(NOTIFYICONDATA);
   notify_data_.hWnd             = message_window_.get();
   notify_data_.uID              = uid;
   notify_data_.hIcon            = icon;
   notify_data_.uFlags           = NIF_MESSAGE | NIF_TIP | (icon != 0 ? NIF_ICON : 0);
   notify_data_.uCallbackMessage = TASKBAR_CALLBACK_MSG;

   std::ranges::copy(tool_tip, notify_data_.szTip);

   creation_flags_ = notify_data_.uFlags;

   if (!hidden_) {
      if (!Shell_NotifyIcon(NIM_ADD, &notify_data_)) {
         hidden_ = true;
      } else {
         show_icon_pending_ = false;
      }
   }
}

SystemTray::~SystemTray() { RemoveIcon(); }

bool
SystemTray::MoveToRight() {
   return AddIcon();
}

bool
SystemTray::AddIcon() {
   if (!removed_) {
      RemoveIcon();
   }

   notify_data_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
   if (!Shell_NotifyIcon(NIM_ADD, &notify_data_)) {
      show_icon_pending_ = true;
   } else {
      hidden_  = false;
      removed_ = false;
   }

   return !removed_;
}

bool
SystemTray::RemoveIcon() {
   show_icon_pending_ = false;

   if (removed_) {
      return true;
   }

   notify_data_.uFlags = 0;
   if (Shell_NotifyIcon(NIM_DELETE, &notify_data_)) {
      removed_ = true;
      hidden_  = true;
   }

   return !removed_;
}

bool
SystemTray::HideIcon() {
   if (removed_ || hidden_) {
      return true;
   }

   RemoveIcon();
   return hidden_;
}

bool
SystemTray::ShowIcon() {
   if (removed_) {
      return AddIcon();
   }

   if (!hidden_) {
      return true;
   }

   AddIcon();

   return !hidden_;
}

bool
SystemTray::SetIcon(HICON icon) {
   notify_data_.uFlags = NIF_ICON;
   notify_data_.hIcon  = icon;

   if (hidden_) {
      return true;
   } else {
      return Shell_NotifyIcon(NIM_MODIFY, &notify_data_);
   }
}

bool
SystemTray::SetIcon(std::string_view name) {
   auto const hIcon = static_cast<HICON>(
     ::LoadImage(GetModuleHandle(nullptr), name.data(), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR)
   );
   if (!hIcon) {
      return false;
   }

   auto const success = SetIcon(hIcon);
   ::DestroyIcon(hIcon);

   return success;
}

bool
SystemTray::SetIcon(uint32_t nIDResource) {
   auto const hIcon = static_cast<HICON>(::LoadImage(
     GetModuleHandle(nullptr), MAKEINTRESOURCE(nIDResource), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR
   ));
   if (!hIcon) {
      return false;
   }

   auto const success = SetIcon(hIcon);
   ::DestroyIcon(hIcon);

   return success;
}

bool
SystemTray::SetStandardIcon(std::string_view lpIconName) {
   auto const hIcon = LoadIcon(GetModuleHandle(nullptr), lpIconName.data());
   return SetIcon(hIcon);
}

bool
SystemTray::SetStandardIcon(uint32_t nIDResource) {
   auto const hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(nIDResource));
   return SetIcon(hIcon);
}

HICON
SystemTray::GetIcon() const { return notify_data_.hIcon; }

bool
SystemTray::SetTooltipText(std::string_view text) {
   assert(text.size() < sizeof(decltype(NOTIFYICONDATA::szTip)));

   notify_data_.uFlags = NIF_TIP;
   std::ranges::copy(text, notify_data_.szTip);

   if (hidden_) {
      return true;
   } else {
      return Shell_NotifyIcon(NIM_MODIFY, &notify_data_);
   }
}

bool
SystemTray::SetTooltipText(uint32_t uid) {
   return SetTooltipText(std::to_string(uid));
}

std::string_view
SystemTray::GetTooltipText() const {
   return notify_data_.szTip;
}

LRESULT
SystemTray::OnMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
   if (msg == SystemTray::TASKBAR_CREATED_MSG) {
      return OnTaskbarCreated(wParam, lParam);
   } else if (msg == notify_data_.uCallbackMessage) {
      return OnTrayNotification(wParam, lParam);
   } else if (msg == WM_APP) {
      if (auto f = reinterpret_cast<std::function<void()>*>(lParam); f) {
         (*f)();
         delete f;
      }
   }

   return OnMessageImpl(hwnd, msg, wParam, lParam);
}

LRESULT
SystemTray::OnMessageImpl(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
   return DefWindowProcW(hwnd, msg, wp, lp);
}

uint32_t
SystemTray::GetUid() const {
   return notify_data_.uID;
}

HWND
SystemTray::GetHandle() const {
   return message_window_.get();
}

// This is called whenever the taskbar is created (eg after explorer crashes
// and restarts. Please note that the WM_TASKBARCREATED message is only passed
// to TOP LEVEL windows (like WM_QUERYNEWPALETTE)
LRESULT
SystemTray::OnTaskbarCreated(WPARAM /*wParam*/, LPARAM /*lParam*/) {
   show_icon_pending_ = true;
   InstallIconPending();

   return 0L;
}

// void
// SystemTray::OnSettingChange(UINT uFlags, LPCTSTR lpszSection) {
//    CWnd::OnSettingChange(uFlags, lpszSection);

//    if (uFlags == SPI_SETWORKAREA) {
//       show_icon_pending_ = !hidden_;
//       InstallIconPending();
//    }
// }

void
SystemTray::InstallIconPending() {  //@todo
   // Is the icon display pending, and it's not been set as "hidden"?
   if (!show_icon_pending_ || hidden_) {
      return;
   }

   // Reset the flags to what was used at creation
   notify_data_.uFlags = creation_flags_;

   // Try and recreate the icon
   hidden_ = !Shell_NotifyIcon(NIM_ADD, &notify_data_);

   // If it's STILL hidden, then have another go next time...
   show_icon_pending_ = hidden_;

   assert(!hidden_);
}

// /////////////////////////////////////////////////////////////////////////////
// // For minimising/maximising from system tray

BOOL CALLBACK
FindTrayWnd(HWND handle, LPARAM lParam) {
   std::string class_name{};
   class_name.resize(255);
   GetClassName(handle, class_name.data(), class_name.size());
   class_name.resize(class_name.find_first_of('\0'));

   // Did we find the Main System Tray? If so, then get its size and keep going
   if (class_name == "TrayNotifyWnd") {
      ::GetWindowRect(handle, reinterpret_cast<CRect*>(lParam));

      EnumChildWindows(handle, FindTrayWnd, lParam);
      return true;
   }

   // Did we find the System Clock? If so, then adjust the size of the rectangle
   // we have and quit (clock will be found after the system tray)
   if (class_name == "TrayClockWClass") {
      auto* rect = reinterpret_cast<CRect*>(lParam);
      CRect rect_clock{};

      ::GetWindowRect(handle, rect_clock);

      // if clock is above system tray adjust accordingly
      if (rect_clock.bottom < rect->bottom - 5) {  // 10 = random fudge factor.
         rect->top = rect_clock.bottom;
      } else {
         rect->right = rect_clock.left;
      }
      return false;
   }

   return true;
}

// // enhanced version by Matthew Ellis <m.t.ellis@bigfoot.com>
RECT
SystemTray::GetTrayWndRect() {
   static constexpr auto DEFAULT_RECT_WIDTH{150};
   static constexpr auto DEFAULT_RECT_HEIGHT{30};

   RECT rect{};

   auto tray_window = ::FindWindow(_T("Shell_TrayWnd"), nullptr);
   if (tray_window) {
      ::GetWindowRect(tray_window, &rect);
      EnumChildWindows(tray_window, FindTrayWnd, reinterpret_cast<LPARAM>(&rect));
      return rect;
   }
   // OK, we failed to get the rect from the quick hack. Either explorer isn't
   // running or it's a new version of the shell with the window class names
   // changed (how dare Microsoft change these undocumented class names!) So, we
   // try to find out what side of the screen the taskbar is connected to. We
   // know that the system tray is either on the right or the bottom of the
   // taskbar, so we can make a good guess at where to minimize to
   APPBARDATA appbar_data;
   appbar_data.cbSize = sizeof(appbar_data);
   if (SHAppBarMessage(ABM_GETTASKBARPOS, &appbar_data)) {
      // We know the edge the taskbar is connected to, so guess the rect of the
      // system tray. Use various fudge factor to make it look good
      switch (appbar_data.uEdge) {
         case ABE_LEFT:
         case ABE_RIGHT:
            // We want to minimize to the bottom of the taskbar
            rect.top    = appbar_data.rc.bottom - 100;
            rect.bottom = appbar_data.rc.bottom - 16;
            rect.left   = appbar_data.rc.left;
            rect.right  = appbar_data.rc.right;
            break;

         case ABE_TOP:
         case ABE_BOTTOM:
            // We want to minimize to the right of the taskbar
            rect.top    = appbar_data.rc.top;
            rect.bottom = appbar_data.rc.bottom;
            rect.left   = appbar_data.rc.right - 100;
            rect.right  = appbar_data.rc.right - 16;
            break;

         default:
            return rect;
      }
      return rect;
   }

   // Blimey, we really aren't in luck. It's possible that a third party shell
   // is running instead of explorer. This shell might provide support for the
   // system tray, by providing a Shell_TrayWnd window (which receives the
   // messages for the icons) So, look for a Shell_TrayWnd window and work out
   // the rect from that. Remember that explorer's taskbar is the Shell_TrayWnd,
   // and stretches either the width or the height of the screen. We can't rely
   // on the 3rd party shell's Shell_TrayWnd doing the same, in fact, we can't
   // rely on it being any size. The best we can do is just blindly use the
   // window rect, perhaps limiting the width and height to, say 150 square.
   // Note that if the 3rd party shell supports the same configuraion as
   // explorer (the icons hosted in NotifyTrayWnd, which is a child window of
   // Shell_TrayWnd), we would already have caught it above
   if (tray_window) {
      ::GetWindowRect(tray_window, &rect);
      if (rect.right - rect.left > DEFAULT_RECT_WIDTH) {
         rect.left = rect.right - DEFAULT_RECT_WIDTH;
      }
      if (rect.bottom - rect.top > DEFAULT_RECT_HEIGHT) {
         rect.top = rect.bottom - DEFAULT_RECT_HEIGHT;
      }

      return rect;
   }

   // OK. Haven't found a thing. Provide a default rect based on the current work
   // area
   SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
   rect.left = rect.right - DEFAULT_RECT_WIDTH;
   rect.top  = rect.bottom - DEFAULT_RECT_HEIGHT;

   return rect;
}

void
SystemTray::Dispatch(std::function<void()> func) {
   PostMessageW(
     message_window_.get(), WM_APP, 0, (LPARAM) new std::function<void()>(std::move(func))
   );
}
}  // namespace win32