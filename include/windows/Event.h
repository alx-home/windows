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

#include <Windows.h>
#include <handleapi.h>
#include <winnt.h>

#include <string_view>

#undef CreateEvent

namespace win32 {

class Event {
public:
   Event() = default;
   Event(HANDLE handle);

   Event(Event const&)                = delete;
   Event(Event&&) noexcept            = delete;
   Event& operator=(Event const&)     = delete;
   Event& operator=(Event&&) noexcept = delete;

   ~Event();

   operator bool() const;
   operator HANDLE const() const;

private:
   HANDLE handle_{INVALID_HANDLE_VALUE};
};

Event
CreateEvent(bool manual_reset = false, bool initial_state = false, std::string_view name = "");

}  // namespace win32