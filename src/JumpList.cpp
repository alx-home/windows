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

#include "utils/String.h"
#include "windows/Window.h"

#include <ObjectArray.h>
#include <combaseapi.h>
#include <errhandlingapi.h>
#include <intsafe.h>
#include <knownfolders.h>
#include <shlobj_core.h>
#include <shobjidl_core.h>
#include <winerror.h>
#include <winnt.h>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <Propkey.h>
#include <propvarutil.h>

namespace win32 {
JumpList::JumpList() {
   if (!list_ || !tasks_) {
      init_failed_ = true;
   } else {
      UINT          minslot;
      IObjectArray* removed;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
      if (auto hr = list_->BeginList(&minslot, IID_PPV_ARGS(&removed)); SUCCEEDED(hr)) {
#pragma clang diagnostic pop
         removed_.reset(removed);
      } else {
         init_failed_ = true;
         std::cerr << "Couldn't get jumpList removed objects (" << GetLastError() << ")"
                   << std::endl;
      }
   }
}

JumpList::~JumpList() {
   if (!init_failed_) {

      if (auto const hr = list_->CommitList(); !SUCCEEDED(hr)) {
         std::cerr << "Couldn't commit list (" << GetLastError() << ")" << std::endl;
      }
   }
}

bool
JumpList::IsRemoved(IShellItem* item) {
   assert(removed_);
   assert(!init_failed_);

   UINT num_items;
   if (SUCCEEDED(removed_->GetCount(&num_items))) {
      IShellItem* pcompare;
      auto        compare{Init<IShellItem>()};
      for (UINT i = 0; i < num_items; ++i) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
         if (SUCCEEDED(removed_->GetAt(i, IID_PPV_ARGS(&pcompare)))) {
#pragma clang diagnostic pop
            compare.reset(pcompare);

            int order;
            if (auto res = pcompare->Compare(item, SICHINT_CANONICAL, &order);
                SUCCEEDED(res) && (0 == order)) {
               return true;
            }
         }
      }
   }

   return false;
}

void
JumpList::AddCategory(std::string_view name, std::vector<std::string> const& items) {
   if (init_failed_) {
      return;
   }

   auto collection{
     Create<IObjectCollection>(CLSID_EnumerableObjectCollection, nullptr, CLSCTX_INPROC)
   };
   if (!collection) {
      return;
   }

   for (auto const& item : items) {
      std::wstring witem = utils::WidenString(item);

      IShellItem* psi;
      auto        shell_item{Init<IShellItem>()};
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
      if (auto const hr = SHCreateItemInKnownFolder(
            FOLDERID_Documents, KF_FLAG_DEFAULT, witem.data(), IID_PPV_ARGS(&psi)
          );
          !SUCCEEDED(hr)) {
#pragma clang diagnostic pop
         std::cerr << "Couldn't create ishellitem for item " << item << " (" << hr << ")"
                   << std::endl;
         continue;
      }
      shell_item.reset(psi);

      if (!IsRemoved(psi)) {
         collection->AddObject(psi);
      }
   }

   IObjectArray* pobject;
   auto          object = Init<IObjectArray>();
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
   if (auto const hr = collection->QueryInterface(IID_PPV_ARGS(&pobject)); !SUCCEEDED(hr)) {
#pragma clang diagnostic pop
      std::cerr << "Couldn't query category object interface (" << hr << ")" << std::endl;
      return;
   }
   object.reset(pobject);

   std::wstring wname{utils::WidenString(name)};
   if (auto const hr = list_->AppendCategory(wname.data(), pobject); !SUCCEEDED(hr)) {
      std::cerr << "Couldn't append category (" << hr << ")" << std::endl;
   }
}

void
JumpList::AddTask(std::string_view title, std::string_view app, std::string_view args) {
   if (init_failed_) {
      return;
   }

   auto link = Create<IShellLink>(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER);
   if (!link) {
      return;
   }

   if (auto const hr = link->SetPath(app.data()); !SUCCEEDED(hr)) {
      std::cerr << "Couldn't set task path (" << hr << ")" << std::endl;
      return;
   }

   if (auto const hr = link->SetArguments(args.data()); !SUCCEEDED(hr)) {
      std::cerr << "Couldn't set task arguments (" << hr << ")" << std::endl;
      return;
   }

   IPropertyStore* pproperty_store;
   auto            property_store = Init<IPropertyStore>();
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
   if (auto const hr = link->QueryInterface(IID_PPV_ARGS(&pproperty_store)); !SUCCEEDED(hr)) {
#pragma clang diagnostic pop
      std::cerr << "Couldn't query task property store (" << hr << ")" << std::endl;
      return;
   }

   property_store.reset(pproperty_store);

   PROPVARIANT  propvar;
   std::wstring wtitle{utils::WidenString(title)};

   if (auto const hr = InitPropVariantFromString(wtitle.data(), &propvar); !SUCCEEDED(hr)) {
      std::cerr << "Couldn't init task title (" << hr << ")" << std::endl;
      return;
   }

   if (auto const hr = property_store->SetValue(PKEY_Title, propvar); !SUCCEEDED(hr)) {
      std::cerr << "Couldn't set task title (" << hr << ")" << std::endl;
      return;
   }

   struct ClearVariant {
      PROPVARIANT& propvar_;
      ~ClearVariant() { PropVariantClear(&propvar_); }
   } _{.propvar_ = propvar};

   if (auto const hr = property_store->Commit(); !SUCCEEDED(hr)) {
      std::cerr << "Couldn't commit task (" << hr << ")" << std::endl;
      return;
   }

   IShellLink* psl;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
   if (auto const hr = link->QueryInterface(IID_PPV_ARGS(&psl)); !SUCCEEDED(hr)) {
#pragma clang diagnostic pop
      std::cerr << "Couldn't query link interface (" << hr << ")" << std::endl;
      return;
   }

   tasks_->AddObject(psl);
   psl->Release();
}

void
JumpList::AddTaskSeparator() {
   if (init_failed_) {
      return;
   }

   auto property_store = Create<IPropertyStore>(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER);
   if (!property_store) {
      return;
   }

   PROPVARIANT propvar;
   if (auto const hr = InitPropVariantFromBoolean(TRUE, &propvar); !SUCCEEDED(hr)) {
      std::cout << "Couldn't init task property variant (" << GetLastError() << ")" << std::endl;
      return;
   }

   if (auto const hr = property_store->SetValue(PKEY_AppUserModel_IsDestListSeparator, propvar);
       !SUCCEEDED(hr)) {
      std::cout << "Couldn't set task property value (" << GetLastError() << ")" << std::endl;
      return;
   }
   struct ClearVariant {
      PROPVARIANT& propvar_;
      ~ClearVariant() { PropVariantClear(&propvar_); }
   } _{.propvar_ = propvar};

   if (auto const hr = property_store->Commit(); !SUCCEEDED(hr)) {
      std::cout << "Couldn't commit task property value (" << GetLastError() << ")" << std::endl;
      return;
   }

   IShellLink* psl;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
   if (auto const hr = property_store->QueryInterface(IID_PPV_ARGS(&psl)); !SUCCEEDED(hr)) {
#pragma clang diagnostic pop
      std::cout << "Couldn't query task property interface (" << GetLastError() << ")" << std::endl;
      return;
   }

   tasks_->AddObject(psl);
   psl->Release();
}

void
JumpList::CommitTasks() {
   if (init_failed_) {
      return;
   }

   IObjectArray* pobject_array;
   auto          object_array = Init<IObjectArray>();

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
   if (auto const hr = tasks_->QueryInterface(IID_PPV_ARGS(&pobject_array)); !SUCCEEDED(hr)) {
#pragma clang diagnostic pop
      std::cerr << "Couldn't query task property interface (" << GetLastError() << ")" << std::endl;
      return;
   }
   object_array.reset(pobject_array);

   if (auto const hr = list_->AddUserTasks(pobject_array); !SUCCEEDED(hr)) {
      std::cerr << "Couldn't add task to list (" << GetLastError() << ")" << std::endl;
      return;
   }
}

void
JumpList::Delete() {
   auto destination_list =
     Create<ICustomDestinationList>(CLSID_DestinationList, nullptr, CLSCTX_INPROC_SERVER);

   if (destination_list) {
      destination_list->DeleteList(nullptr);
   }
}

}  // namespace win32
