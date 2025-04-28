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

#include "Registry.h"
#include "concepts.h"
#include "../details/Registry.h"

#include <Windows.h>
#include <minwindef.h>
#include <winerror.h>
#include <winnt.h>
#include <winreg.h>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace registry {

template <String PATH>
static constexpr auto
MakePath() {
   return PATH;
}

namespace details {
template <Store STORE>
static auto store_name_s = []() {
   using enum Store;

   if constexpr (STORE == EHKEY_CURRENT_USER) {
      return std::string{MakePath<"HKEY_CURRENT_USER">().value_.data()};
   } else if constexpr (STORE == EHKEY_LOCAL_MACHINE) {
      return std::string{MakePath<"HKEY_LOCAL_MACHINE">().value_.data()};
   } else {
      static_assert(false, std::format("Unexpected store {}", static_cast<std::size_t>(STORE)));
   }
}();
}  // namespace details

using details::store_name_s;

template <Store STORE, String PATH, class PARENT_KEY, bool OWNED>
template <class SELF>
constexpr void
Key<STORE, PATH, PARENT_KEY, OWNED>::Apply(this SELF&& self, auto&& apply) {
   std::apply(
      [&]<_key... KEY>(KEY&&... ptr) constexpr { (apply((self.*ptr).key_), ...); },
      std::remove_cvref_t<SELF>::KEYS
   );
}

template <Store STORE, String PATH, class PARENT_KEY, bool OWNED>
template <class SELF>
constexpr void
Key<STORE, PATH, PARENT_KEY, OWNED>::ApplyToOwned(this SELF&& self, auto&& apply) {
   std::apply(
      [&]<_key... KEY>(KEY&&... ptr) constexpr {
         (
            [&]<_key KEY2>(KEY2&& member) constexpr {
               if constexpr (std::_Remove_cvref_t<KEY2>::IS_OWNED) {
                  apply(member);
               }
            }((self.*ptr).key_),
            ...
         );
      },
      std::remove_cvref_t<SELF>::keys_
   );
}

template <Store STORE, String PATH, class PARENT_KEY, bool OWNED>
constexpr std::string
Key<STORE, PATH, PARENT_KEY, OWNED>::FullPath() {
   if constexpr (!std::is_void_v<PARENT_KEY>) {
      if (PARENT_KEY::FullPath().size()) {
         return std::string{PARENT_KEY::FullPath()} + "\\" + std::string{KEY_PATH.data()};
      }
   }

   return KEY_PATH.data();
}

template <Store STORE, String PATH, class PARENT_KEY, bool OWNED>
constexpr std::string
Key<STORE, PATH, PARENT_KEY, OWNED>::ParentPath() {
   auto const full_path = FullPath();
   return full_path.substr(0, full_path.find_last_of("\\"));
}

template <Store STORE, String PATH, class PARENT_KEY, bool OWNED>
constexpr std::string
Key<STORE, PATH, PARENT_KEY, OWNED>::KeyName() {
   auto const full_path = FullPath();
   return full_path.substr(full_path.find_last_of("\\") + 1);
}

template <Store STORE, String PATH, class PARENT_KEY, bool OWNED>
constexpr std::pair<LSTATUS, details::Key>
Key<STORE, PATH, PARENT_KEY, OWNED>::Open(REGSAM acces, uint32_t ulOptions) {
   details::Key hkey{};
   auto const   result =
      hkey.Open(store_value<Key::STORE_VALUE>, Key::FullPath().c_str(), ulOptions, acces);
   return {result, hkey};
}

template <Store STORE, String PATH, class PARENT_KEY, bool OWNED>
constexpr std::pair<LSTATUS, details::Key>
Key<STORE, PATH, PARENT_KEY, OWNED>::Ensure() {
   if constexpr (!std::is_void_v<PARENT_KEY>) {
      PARENT_KEY::Ensure();
   }

   if constexpr (OWNED) {
      details::Key hkey{};

      auto const parent = ParentPath();
      if (auto const status =
             hkey.Open(store_value<STORE_VALUE>, parent.c_str(), 0, KEY_ALL_ACCESS);
          status != ERROR_SUCCESS) {
         return {status, hkey};
      }

      auto const KEY_PATH = Key::FullPath();
      auto const status   = hkey.Create(
         store_value<STORE_VALUE>, KEY_PATH.c_str(), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS
      );
      return {status, hkey};
   } else {
      return Key::Open(KEY_READ);
   }
}

template <Store STORE, String PATH, class PARENT_KEY, bool OWNED>
constexpr std::pair<
   typename Key<STORE, PATH, PARENT_KEY, OWNED>::num_key_t,
   typename Key<STORE, PATH, PARENT_KEY, OWNED>::num_value_t>
Key<STORE, PATH, PARENT_KEY, OWNED>::Info() {
   auto [status, hKey] = Key::Open(KEY_QUERY_VALUE);

   if (status != ERROR_SUCCESS) {
      if (status == ERROR_FILE_NOT_FOUND) {
         return {0, 0};
      }
      throw AccessError{
         "Registry: Couldn't open key \"" + store_name_s<STORE>
         + "\\" + FullPath() + "\", error: " + std::to_string(status) + "!"
      };
   }

   assert(hKey);

   DWORD subkeys;
   DWORD values;
   if (auto const status = RegQueryInfoKey(
          hKey.Handle(),
          nullptr,
          nullptr,
          nullptr,
          &subkeys,
          nullptr,
          nullptr,
          &values,
          nullptr,
          nullptr,
          nullptr,
          0
       );
       status != ERROR_SUCCESS) {
      throw AccessError{
         "Registry: Couldn't open key \"" + store_name_s<STORE>
         + "\\" + FullPath() + "\", error: " + std::to_string(status) + "!"
      };
   }

   return {subkeys, values};
}

template <Store STORE, String KEY_PATH, class PARENT_KEY, bool OWNED>
template <class SELF>
void
Key<STORE, KEY_PATH, PARENT_KEY, OWNED>::Clear(this SELF&& self) {
   if constexpr (IS_OWNED) {
      self.Delete();
   } else {
      self.Apply([](auto& key) constexpr { key.Clear(); });
   }
}

template <Store STORE, String PATH, class PARENT_KEY, bool OWNED>
template <class SELF>
void
Key<STORE, PATH, PARENT_KEY, OWNED>::Delete(this SELF&&) {
   auto const KEY_PATH{FullPath()};

   auto const parent = ParentPath();
   auto const name   = KeyName();

   details::Key key{};
   if (key.Open(
          store_value<STORE>, KEY_PATH.c_str(), 0, DELETE | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE
       )
       == ERROR_SUCCESS) {
      RegDeleteTree(key.Handle(), nullptr);
   }

   if (key.Open(store_value<STORE>, parent.c_str(), 0, DELETE) == ERROR_SUCCESS) {
      RegDeleteKey(key.Handle(), name.c_str());
   }
}

template <class TYPE, _key KEY, String KEY_NAME>
template <class...>
   requires(KEY::IS_OWNED)
LRESULT
Value<TYPE, KEY, KEY_NAME>::DeleteValue() {
   auto [status, hKey] = KEY::Open(KEY_SET_VALUE);

   if (status == ERROR_SUCCESS) {
      auto const name = std::string{KEY_NAME.value_.data()};
      return RegDeleteValue(hKey.Handle(), name.c_str());
   }

   return status;
}

template <class TYPE, _key KEY, String KEY_NAME>
template <class TYPE2>
   requires(KEY::IS_OWNED && std::is_constructible_v<TYPE, TYPE2>)
Value<TYPE, KEY, KEY_NAME>&
Value<TYPE, KEY, KEY_NAME>::operator=(TYPE2&& value) {
   auto [status, hKey] = KEY::Ensure();
   if (!hKey) {
      throw AccessError{
         "Registry: Couldn't ensure key \"" + store_name_s<KEY::STORE_VALUE>
         + "\\" + KEY::FullPath() + "\", error: " + std::to_string(status) + "!"
      };
   }

   std::string const name{VALUE_NAME.value_.data()};

   if constexpr (std::is_constructible_v<std::string_view, TYPE2>) {
      std::string_view value2{value};
      if (auto const status = RegSetValueExA(
             hKey.Handle(),
             name.c_str(),
             0,
             REG_SZ,
             reinterpret_cast<BYTE const*>(value2.data()),
             value2.size()
          );
          status != ERROR_SUCCESS) {
         throw AccessError{
            "Registry: Couldn't set key value \"" + store_name_s<KEY::STORE_VALUE>
            + "\\" + KEY::FullPath() + "\\" + name + ", error: " + std::to_string(status) + "!"
         };
      }
   } else if constexpr (std::is_constructible_v<std::span<std::string_view>, TYPE2>) {
      std::string value2;

      for (auto& elem : std::span<std::string_view>{value}) {
         value2 += std::string{elem} + "\0";
      }

      if (auto const status = RegSetValueExA(
             hKey.Handle(), name.c_str(), 0, REG_MULTI_SZ, value2.data(), value2.size()
          );
          status != ERROR_SUCCESS) {
         throw AccessError{
            "Registry: Couldn't set key value \"" + store_name_s<KEY::STORE_VALUE>
            + "\\" + KEY::FullPath() + "\\" + name + ", error: " + std::to_string(status) + "!"
         };
      }
   } else if constexpr (std::is_constructible_v<uint32_t, TYPE2>
                        && (sizeof(TYPE) <= sizeof(uint32_t))) {
      uint32_t value2{value};

      if (auto const status = RegSetValueExA(
             hKey.Handle(),
             name.c_str(),
             0,
             REG_DWORD,
             reinterpret_cast<BYTE const*>(&value2),
             sizeof(value2)
          );
          status != ERROR_SUCCESS) {
         throw AccessError{
            "Registry: Couldn't set key value \"" + store_name_s<KEY::STORE_VALUE>
            + "\\" + KEY::FullPath() + "\\" + name + ", error: " + std::to_string(status) + "!"
         };
      }
   } else if constexpr (std::is_constructible_v<uint64_t, TYPE2>
                        && (sizeof(TYPE) <= sizeof(uint64_t))) {
      uint64_t value2{value};

      if (auto const status = RegSetValueExA(
             hKey.Handle(),
             name.c_str(),
             0,
             REG_QWORD,
             reinterpret_cast<BYTE const*>(&value2),
             sizeof(value2)
          );
          status != ERROR_SUCCESS) {
         throw AccessError{
            "Registry: Couldn't set key value \"" + store_name_s<KEY::STORE_VALUE>
            + "\\" + KEY::FullPath() + "\\" + name + ", error: " + std::to_string(status) + "!"
         };
      }
   } else if constexpr (std::is_constructible_v<std::span<std::byte>, TYPE2>) {
      std::span<std::byte> value2{value};

      if (auto const status = RegSetValueExA(
             hKey.Handle(),
             name.c_str(),
             0,
             REG_BINARY,
             reinterpret_cast<BYTE const*>(value2.data()),
             value2.size()
          );
          status != ERROR_SUCCESS) {
         throw AccessError{
            "Registry: Couldn't set key value \"" + store_name_s<KEY::STORE_VALUE>
            + "\\" + KEY::FullPath() + "\\" + name + ", error: " + std::to_string(status) + "!"
         };
      }
   } else {
      static_assert(false);
   }

   return *this;
}

template <class TYPE, _key KEY, String KEY_NAME> Value<TYPE, KEY, KEY_NAME>::operator bool() const {
   auto [status, hKey] = KEY::Open(KEY_QUERY_VALUE);
   if (!hKey) {
      if (status == ERROR_FILE_NOT_FOUND) {
         return false;
      }

      throw AccessError{
         "Registry: Couldn't open key \"" + store_name_s<KEY::STORE_VALUE>
         + "\\" + KEY::FullPath() + ", error: " + std::to_string(status) + "!"
      };
   }

   std::string const name{VALUE_NAME.value_.data()};
   DWORD             size{};
   if (auto const status =
          RegGetValueA(hKey.Handle(), "", name.c_str(), RRF_RT_ANY, NULL, nullptr, &size);
       status != ERROR_SUCCESS) {
      if (status == ERROR_FILE_NOT_FOUND) {
         return false;
      }
      throw AccessError{
         "Registry: Couldn't get key value \"" + store_name_s<KEY::STORE_VALUE>
         + "\\" + KEY::FullPath() + "\\" + name + "\", error: " + std::to_string(status) + "!"
      };
   }

   return true;
}

template <class TYPE, _key KEY, String KEY_NAME>
TYPE
Value<TYPE, KEY, KEY_NAME>::operator*() const {
   auto [status, hKey] = KEY::Open(KEY_QUERY_VALUE);
   if (!hKey) {
      throw AccessError{
         "Registry: Couldn't open key \"" + store_name_s<KEY::STORE_VALUE>
         + "\\" + KEY::FullPath() + ", error: " + std::to_string(status) + "!"
      };
   }

   std::string const name{VALUE_NAME.value_.data()};
   if constexpr (std::is_constructible_v<std::string_view, TYPE>) {
      std::string value{};
      DWORD       size{};
      if (auto const status =
             RegGetValueA(hKey.Handle(), "", name.c_str(), RRF_RT_REG_SZ, NULL, nullptr, &size);
          status != ERROR_SUCCESS) {
         throw AccessError{
            "Registry: Couldn't get key value \"" + store_name_s<KEY::STORE_VALUE>
            + "\\" + KEY::FullPath() + "\\" + name + "\", error: " + std::to_string(status) + "!"
         };
      }

      value.resize(size - 1);
      if (auto const status = RegGetValueA(
             hKey.Handle(), "", name.c_str(), RRF_RT_REG_SZ, NULL, value.data(), &size
          );
          status != ERROR_SUCCESS) {
         throw AccessError{
            "Registry: Couldn't get key value \"" + store_name_s<KEY::STORE_VALUE>
            + "\\" + KEY::FullPath() + "\\" + name + "\", error: " + std::to_string(status) + "!"
         };
      }

      value.resize(size - 1);
      return value;
   } else if constexpr (std::is_constructible_v<std::span<std::string_view>, TYPE>) {
      std::string value{};
      DWORD       size{};

      if (auto const status = RegGetValueA(
             hKey.Handle(), "", name.c_str(), RRF_RT_REG_MULTI_SZ, NULL, nullptr, &size
          );
          status != ERROR_SUCCESS) {
         throw AccessError{
            "Registry: Couldn't get key value \"" + store_name_s<KEY::STORE_VALUE>
            + "\\" + KEY::FullPath() + "\\" + name + "\", error: " + std::to_string(status) + "!"
         };
      }

      value.resize(size - 1);
      if (auto const status = RegGetValueA(
             hKey.Handle(), "", name.c_str(), RRF_RT_REG_MULTI_SZ, NULL, value.data(), &size
          );
          status != ERROR_SUCCESS) {
         throw AccessError{
            "Registry: Couldn't get key value \"" + store_name_s<KEY::STORE_VALUE>
            + "\\" + KEY::FullPath() + "\\" + name + "\", error: " + std::to_string(status) + "!"
         };
      }

      value.resize(size - 1);
      std::vector<std::string> result{};

      auto last = value.begin();
      for (auto it = value.begin(); it != value.end(); ++it) {
         if (*it == '\0') {
            result.emplace_back(std::string_view{last, it});
            last = std::next(it);
         }
      }

      return result;
   } else if constexpr (std::is_constructible_v<uint32_t, TYPE>
                        && (sizeof(TYPE) <= sizeof(uint32_t))) {
      uint32_t value{};
      DWORD    size = sizeof(value);
      if (auto const status =
             RegGetValueA(hKey.Handle(), "", name.c_str(), RRF_RT_REG_DWORD, NULL, &value, &size);
          status != ERROR_SUCCESS) {
         throw AccessError{
            "Registry: Couldn't get key value \"" + store_name_s<KEY::STORE_VALUE>
            + "\\" + KEY::FullPath() + "\\" + name + "\", error: " + std::to_string(status) + "!"
         };
      }

      return value;
   } else if constexpr (std::is_constructible_v<uint64_t, TYPE>
                        && (sizeof(TYPE) <= sizeof(uint64_t))) {
      uint64_t value{};
      DWORD    size = sizeof(value);
      if (auto const status =
             RegGetValueA(hKey.Handle(), "", name.c_str(), RRF_RT_REG_QWORD, NULL, &value, &size);
          status != ERROR_SUCCESS) {
         throw AccessError{
            "Registry: Couldn't get key value \"" + store_name_s<KEY::STORE_VALUE>
            + "\\" + KEY::FullPath() + "\\" + name + "\", error: " + std::to_string(status) + "!"
         };
      }

      return value;
   } else if constexpr (std::is_constructible_v<std::span<std::byte>, TYPE>) {
      std::vector<std::byte> value{};
      DWORD                  size{};
      if (auto const status =
             RegGetValueA(hKey.Handle(), "", name.c_str(), RRF_RT_REG_BINARY, NULL, nullptr, &size);
          status != ERROR_SUCCESS) {
         throw AccessError{
            "Registry: Couldn't get key value \"" + store_name_s<KEY::STORE_VALUE>
            + "\\" + KEY::FullPath() + "\\" + name + "\", error: " + std::to_string(status) + "!"
         };
      }

      value.resize(size);
      if (auto const status = RegGetValueA(
             hKey.Handle(), "", name.c_str(), RRF_RT_REG_BINARY, NULL, value.data(), &size
          );
          status != ERROR_SUCCESS) {
         throw AccessError{
            "Registry: Couldn't get key value \"" + store_name_s<KEY::STORE_VALUE>
            + "\\" + KEY::FullPath() + "\\" + name + "\", error: " + std::to_string(status) + "!"
         };
      }

      value.resize(size);
      return value;
   } else {
      static_assert(false);
   }
}

}  // namespace registry
