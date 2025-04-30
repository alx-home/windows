#include "windows/Shortcuts.h"

#include <utils/String.h>
#include <shlguid.h>
#include <shobjidl_core.h>
#include <winnt.h>
#include <memory>
#include <string>

namespace win32 {
HRESULT
CreateLink(std::string_view origin, std::string_view link, std::string_view description) {
   HRESULT                                            hres;
   std::unique_ptr<IShellLink, void (*)(IShellLink*)> psl{nullptr, [](IShellLink* ptr) constexpr {
                                                             ptr->Release();
                                                          }};

   // Get a pointer to the IShellLink interface. It is assumed that CoInitialize
   // has already been called.
   {
      IShellLink* ptr;
      hres = CoCreateInstance(
         CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&ptr
      );
      psl.reset(ptr);
   }
   if (SUCCEEDED(hres)) {
      std::unique_ptr<IPersistFile, void (*)(IPersistFile*)> ppf{
         nullptr, [](IPersistFile* ppf) constexpr { ppf->Release(); }
      };

      // Set the path to the shortcut target and add the description.
      psl->SetPath(origin.data());
      psl->SetDescription(description.data());

      // Query IShellLink for the IPersistFile interface, used for saving the
      // shortcut in persistent storage.
      {
         IPersistFile* ptr;
         hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ptr);
         ppf.reset(ptr);
      }

      if (SUCCEEDED(hres)) {
         // Save the link by calling IPersistFile::Save.
         hres = ppf->Save(utils::WidenString(link).data(), true);
      }
   }
   return hres;
}
}  // namespace win32