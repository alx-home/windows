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

#include <utils/Concepts.h>
#include <utils/String.inl>

#include <Windows.h>
#include <winreg.h>
#include <tuple>
#include <type_traits>

namespace registry {

using namespace utils;

enum class Store : std::size_t;
template <Store STORE, String PATH, class PARENT_KEY, bool OWNED = false>
struct Key;

template <class TYPE>
struct IsKey : std::false_type {};

template <Store STORE, String PATH, class PARENT_KEY, bool OWNED>
struct IsKey<Key<STORE, PATH, PARENT_KEY, OWNED>> : std::true_type {};

template <class TYPE>
concept _key = requires { IsKey<TYPE>::value; };

template <class TYPE, _key KEY, String NAME>
struct Value;

template <class TYPE>
struct IsValue : std::false_type {};

template <class VALUE_TYPE, class KEY_TYPE, String NAME>
struct IsValue<Value<VALUE_TYPE, KEY_TYPE, NAME>> : std::true_type {};

template <class TYPE>
concept _value = requires { IsValue<TYPE>::value; };

template <_value... VALUE>
using Values = std::tuple<VALUE...>;

template <_key KEY>
class KeyPtr;

template <class TYPE>
struct IsKeyPtr : std::false_type {};

template <_key KEY>
struct IsKeyPtr<KeyPtr<KEY>> : std::true_type {};

template <class TYPE>
concept _key_ptr = requires { IsKeyPtr<TYPE>::value; };

template <_key... KEY>
using Keys = std::tuple<KEY...>;

template <_key_ptr... KEY_PTR>
using KeysPtr = std::tuple<KEY_PTR...>;

enum class Store : std::size_t { EHKEY_CURRENT_USER, EHKEY_LOCAL_MACHINE };

template <Store STORE>
static auto store_value = []() constexpr {
   using enum Store;

   if constexpr (STORE == EHKEY_CURRENT_USER) {
      return HKEY_CURRENT_USER;
   } else if constexpr (STORE == EHKEY_LOCAL_MACHINE) {
      return HKEY_LOCAL_MACHINE;
   } else {
      static_assert(false, "Unexpected store");
   }
}();

}  // namespace registry
