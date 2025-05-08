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

#include "windows/Files.h"

#include <Windows.h>
#include <minwindef.h>
#include <bit>
#include <chrono>
#include <filesystem>

namespace win32 {
std::chrono::system_clock::time_point
CreationTime(std::filesystem::path const& path) {
   auto const handle = CreateFileW(
     path.c_str(),
     GENERIC_READ,
     FILE_SHARE_READ,
     nullptr,
     OPEN_EXISTING,
     FILE_ATTRIBUTE_NORMAL,
     nullptr
   );

   if (handle == INVALID_HANDLE_VALUE) {
      return {};
   }

   FILETIME creation_time;
   if (!GetFileTime(handle, &creation_time, nullptr, nullptr)) {
      CloseHandle(handle);
      return {};
   }
   CloseHandle(handle);

   return std::chrono::system_clock::time_point{
     std::chrono::duration<int64_t, std::ratio<1, 10'000'000>>{std::bit_cast<int64_t>(creation_time)
     }
   };
}
}  // namespace win32