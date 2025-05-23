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

#include "windows/Lock.h"

#include <utils/String.h>
#include <synchapi.h>

namespace win32 {

Mutex::Mutex(HANDLE handle)
   : handle_(handle) {}

Mutex::Mutex(Mutex&& right) noexcept
   : handle_(std::move(right)) {
   right.handle_ = nullptr;
}

Mutex&
Mutex::operator=(Mutex&& right) noexcept {
   handle_       = std::move(right.handle_);
   right.handle_ = nullptr;
   return *this;
}

Mutex::~Mutex() {
   if (handle_) {
      // Release the mutex
      ReleaseMutex(handle_);
      CloseHandle(handle_);
   }
}

Mutex::operator bool() const { return handle_ != nullptr; }

Mutex::operator HANDLE() const { return handle_; }

Mutex
CreateLock(std::string_view name) {
   auto handle = CreateMutexW(nullptr, true, utils::WidenString(name).data());
   if (GetLastError() == ERROR_ALREADY_EXISTS) {
      ReleaseMutex(handle);
      CloseHandle(handle);
      handle = nullptr;
   }

   // Create a named mutex
   return handle;
}

}  // namespace win32