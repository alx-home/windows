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

#include "impl/Registry.h"

#include <Windows.h>
#include <winreg.h>
#include <cstddef>
#include <format>

namespace registry {

template <Store store>
static constexpr auto store_name = []() constexpr {
   using enum Store;

   if constexpr (store == EHKEY_CURRENT_USER) {
      return MakePath<"HKEY_CURRENT_USER">();
   } else if constexpr (store == EHKEY_LOCAL_MACHINE) {
      return MakePath<"HKEY_LOCAL_MACHINE">();
   } else {
      static_assert(false, std::format("Unexpected store {}", static_cast<std::size_t>(store)));
   }
}();

template <Store store, _key Impl>
class Registry : public Impl {
public:
   static constexpr auto KEY_PATH = store_name<store>.value_;

   static Registry& Get();

private:
   Registry() = default;
};

}  // namespace registry

#include "Registry.inl"

using registry::Store;