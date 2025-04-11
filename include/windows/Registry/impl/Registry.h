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

#include "concepts.h"

#include "../details/Registry.h"

#include <Windows.h>
#include <winreg.h>
#include <string>
#include <type_traits>

namespace registry {

struct Exception : std::runtime_error {
   using std::runtime_error::runtime_error;
};

struct AccessError : Exception {
   using Exception::Exception;
};

enum class Store : std::size_t;
template <Store STORE, String PATH, class PARENT_KEY, bool OWNED> struct Key {
public:
   static constexpr auto STORE_VALUE = STORE;
   static constexpr auto IS_OWNED    = OWNED;
   static constexpr auto KEY_PATH    = PATH.value_;

public:
   static constexpr std::string FullPath();
   static constexpr std::string ParentPath();
   static constexpr std::string KeyName();
   template <class SELF> void   Clear(this SELF&& self);

   template <class SELF> void Delete(this SELF&& self);

protected:
   template <class SELF> constexpr void Apply(this SELF&& self, auto&& apply);

   template <class SELF> constexpr void ApplyToOwned(this SELF&& self, auto&& apply);

   static constexpr std::pair<LSTATUS, details::Key> Ensure();
   using num_key_t   = uint32_t;
   using num_value_t = uint32_t;
   static constexpr std::pair<num_key_t, num_value_t> Info();
   static constexpr std::pair<LSTATUS, details::Key>  Open(REGSAM acces, uint32_t ulOptions = 0);

   template <class, _key, String> friend struct Value;
   template <Store, String, class, bool> friend struct Key;
};

template <class TYPE, _key KEY, String NAME> struct Value {
   static constexpr auto VALUE_NAME = NAME;

   template <class TYPE2>
      requires(KEY::IS_OWNED && std::is_constructible_v<TYPE, TYPE2>)
   Value& operator=(TYPE2&& value);

   template <class...>
      requires(KEY::IS_OWNED)
   LRESULT DeleteValue();

   explicit operator bool() const;
   TYPE     operator*() const;
   explicit operator TYPE() const;
};

template <_key KEY> class KeyPtr {
public:
   KeyPtr() = default;

   KEY const* operator->() const { return &key_; }

   KEY* operator->() { return &key_; }

   void Clear() const { key_.Clear(); }

private:
   KEY key_{};

   template <Store STORE, String PATH, class PARENT_KEY, bool OWNED> friend struct registry::Key;
};

}  // namespace registry

#include "Registry.inl"
