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

#include <ObjectArray.h>
#include <shobjidl_core.h>
#include <iostream>
#include <ostream>
#include <string>
#include <utility>
#include <vector>
#ifdef _WIN32

#   include <optional>
#   include <string_view>
#   include <memory>
#   include <utils/String.h>

namespace win32 {
using namespace utils;

template <class TYPE>
concept message_handler = requires(TYPE elem, HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
   { elem.OnMessage(hwnd, msg, wp, lp) } -> std::same_as<LRESULT>;
};

using WinPtr = std::unique_ptr<std::remove_pointer_t<HWND>, BOOL (*)(HWND)>;

template <String CLASS_NAME, message_handler SELF>
static constexpr WinPtr CreateMessageWindow(
  SELF&                                  self,
  std::optional<std::string_view> const& name   = std::nullopt,
  HINSTANCE                              parent = GetModuleHandle(nullptr)
);

class JumpList {
public:
   JumpList();
   ~JumpList();

   void AddCategory(std::string_view name, std::vector<std::string> const& items);
   void AddTask(std::string_view title, std::string_view app, std::string_view args);
   void AddTaskSeparator();

   static void Delete();

private:
   template <class T>
   static constexpr void Release(T* obj) {
      obj->Release();
   }
   template <class T>
   static constexpr std::unique_ptr<T, void (*)(T*)> Init() {
      return {nullptr, Release<T>};
   }
   template <class T>
   using Ptr = std::unique_ptr<T, void (*)(T*)>;

   bool IsRemoved(IShellItem* item);

   template <class T, class... ARGS>
   static constexpr Ptr<T> Create(ARGS&&... args) {
      T*   presult;
      auto result{Init<T>()};

#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wlanguage-extension-token"
      if (auto hr = CoCreateInstance(std::forward<ARGS>(args)..., IID_PPV_ARGS(&presult));
          SUCCEEDED(hr)) {
#   pragma clang diagnostic pop
         result.reset(presult);
      } else {
         std::cerr << "Couldn't create object (" << GetLastError() << ")" << std::endl;
      }

      return result;
   }

   void CommitTasks();

   bool failed_{false};

   Ptr<ICustomDestinationList> list_{
     Create<ICustomDestinationList>(CLSID_DestinationList, nullptr, CLSCTX_INPROC_SERVER)
   };
   Ptr<IObjectCollection> tasks_{Init<IObjectCollection>()};
   Ptr<IObjectArray>      removed_{Init<IObjectArray>()};
};

}  // namespace win32

#   include "Window.inl"

#endif