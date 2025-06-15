/*
MIT License

Copyright (c) 2025 Alexandre GARCIN

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

#include <Windows.h>
#include <minwindef.h>
#include <winerror.h>
#include <winreg.h>
#include <cassert>
#include <memory>
#include <utility>

namespace registry::details {

struct KeyHandler {
   KeyHandler(KeyHandler const&) = delete;
   KeyHandler(KeyHandler&& r) noexcept
      : key_(std::move(r.key_)) {
      assert(!r.key_);
   }

   KeyHandler& operator=(KeyHandler const&)     = delete;
   KeyHandler& operator=(KeyHandler&&) noexcept = delete;

   KeyHandler() = default;

   ~KeyHandler() { Close(); }

   HKEY const& Handle() const noexcept { return key_; }

   explicit operator bool() const noexcept { return key_; }

   void Close() {
      if (key_) {
         RegCloseKey(key_);
         key_ = nullptr;
      }
   }

   LSTATUS Open(HKEY hKey, LPCSTR lpSubKey, uint32_t ulOptions, REGSAM access) {
      Close();

      if (auto const status = RegOpenKeyExA(hKey, lpSubKey, ulOptions, access, &key_);
          status != ERROR_SUCCESS) {
         key_ = nullptr;
         return status;
      }

      return ERROR_SUCCESS;
   }

   LSTATUS Create(HKEY hKey, LPCSTR lpSubKey, uint32_t option, REGSAM access) {
      Close();

      if (auto const status =
            RegCreateKeyExA(hKey, lpSubKey, 0, nullptr, option, access, nullptr, &key_, nullptr);
          status != ERROR_SUCCESS) {
         key_ = nullptr;
         return status;
      }

      return ERROR_SUCCESS;
   }

private:
   HKEY key_{nullptr};
};

struct Key {
   HKEY const& Handle() const noexcept { return handler_->Handle(); }

   explicit operator bool() const noexcept { return handler_->operator bool(); }

   void Close() { handler_->Close(); }

   LSTATUS Open(HKEY hKey, LPCSTR lpSubKey, uint32_t ulOptions, REGSAM access) {
      return handler_->Open(hKey, lpSubKey, ulOptions, access);
   }

   LSTATUS Create(HKEY hKey, LPCSTR lpSubKey, uint32_t option, REGSAM access) {
      return handler_->Create(hKey, lpSubKey, option, access);
   }

private:
   std::shared_ptr<KeyHandler> handler_{std::make_shared<KeyHandler>()};
};

}  // namespace registry::details
