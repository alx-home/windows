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

#include "utils/String.h"
#include <optional>
#include <string_view>
#ifdef _WIN32

#   include "Window.h"

namespace win32 {

template <String CLASS_NAME, message_handler SELF>
static constexpr WinPtr
CreateMessageWindow(SELF& self, std::optional<std::string_view> const& name, HINSTANCE parent) {
   static auto s__registered_class [[maybe_unused]]{[]() constexpr {
      auto const class_name{utils::WidenString(CLASS_NAME.value_.data())};

      WNDCLASSEXW message_class{};
      message_class.cbSize        = sizeof(WNDCLASSEX);
      message_class.hInstance     = GetModuleHandle(nullptr);
      message_class.lpszClassName = class_name.c_str();
      message_class.lpfnWndProc =
        (WNDPROC)(+[](HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) -> LRESULT {
           SELF* self{};

           if (msg == WM_NCCREATE) {
              auto* lpcs{reinterpret_cast<LPCREATESTRUCT>(lp)};
              self = static_cast<SELF*>(lpcs->lpCreateParams);
              SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
           } else {
              self = reinterpret_cast<SELF*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
           }

           if (!self) {
              return DefWindowProcW(hwnd, msg, wp, lp);
           }

           if (msg == WM_DESTROY) {
              SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
           }

           return self->OnMessage(hwnd, msg, wp, lp);
        });

      RegisterClassExW(&message_class);
      return class_name;
   }()};

   std::wstring wname;
   if (name) {
      wname = utils::WidenString(*name);
   }

   return std::unique_ptr<std::remove_pointer_t<HWND>, BOOL (*)(HWND)>{
     CreateWindowExW(
       0,
       s__registered_class.c_str(),
       name ? wname.data() : nullptr,
       0,
       0,
       0,
       0,
       0,
       HWND_MESSAGE,
       nullptr,
       parent,
       &self
     ),
     DestroyWindow
   };
}

}  // namespace win32

#endif