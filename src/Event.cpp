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

#include "windows/Event.h"
#include <handleapi.h>
#include <synchapi.h>

namespace win32 {

Event
CreateEvent(bool manual_reset, bool initial_state, std::string_view name) {
   auto const handle =
     CreateEventA(nullptr, manual_reset, initial_state, name.size() ? name.data() : nullptr);
   if (handle == nullptr) {
      return {};
   }

   return handle;
}

Event::Event(HANDLE handle)
   : handle_(handle) {}

Event::~Event() {
   if (handle_ != INVALID_HANDLE_VALUE) {
      CloseHandle(handle_);
   }
}

Event::operator bool() const { return handle_ != INVALID_HANDLE_VALUE; }

Event::operator HANDLE const() const { return handle_; }

}  // namespace win32