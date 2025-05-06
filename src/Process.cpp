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

#include "windows/Process.h"

#include <utils/String.h>
#include <synchapi.h>
#include <optional>

namespace win32 {

Process::~Process() {
   // Close process and thread handles.
   CloseHandle(handle_);
   CloseHandle(thread_);
}

Process
NewProcess(
  std::string_view                       name,
  std::optional<std::string>             args,
  std::optional<std::string_view> const& cwd
) {

   // additional information
   STARTUPINFO si;
   ZeroMemory(&si, sizeof(si));
   PROCESS_INFORMATION pi;
   ZeroMemory(&pi, sizeof(pi));

   // set the size of the structures
   si.cb = sizeof(si);

   std::string cmd_line;
   if (args) {
      cmd_line = std::string{name} + " " + *args;
   }

   // start the program up
   CreateProcess(
     name.data(),
     args ? cmd_line.data() : nullptr,
     nullptr,
     nullptr,
     FALSE,
     0,
     nullptr,
     cwd ? cwd->data() : nullptr,
     &si,
     &pi
   );

   return {.handle_ = pi.hProcess, .thread_ = pi.hThread};
}

}  // namespace win32