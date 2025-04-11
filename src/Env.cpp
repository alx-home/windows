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

#include "windows/Env.h"

#include <stdlib.h>
#include <regex>
#include <string>

std::string
GetEnv(std::string const& value) {
   std::size_t requiredSize;
   getenv_s(&requiredSize, nullptr, 0, value.c_str());

   if (!requiredSize) {
      return "";
   }

   std::string result;
   result.resize(requiredSize - 1);
   getenv_s(&requiredSize, result.data(), result.size() + 1, value.c_str());

   return result;
}

std::string
GetAppData() {
   return GetEnv("APPDATA");
}

std::string
GetLocalAppData() {
   return GetEnv("LocalAppData");
}

std::string
ReplaceAppData(std::string const& path) {
   static std::regex const reg{"%AppData%"};
   return std::regex_replace(path, reg, GetAppData());
}

std::string
ReplaceLocalAppData(std::string const& path) {
   static std::regex const& reg{"%LocalAppData%"};
   return std::regex_replace(path, reg, GetAppData());
}

std::string
ReplaceEnv(std::string const& path) {
   return ReplaceLocalAppData(ReplaceAppData(path));
}