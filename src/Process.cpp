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

#include "windows/Process.h"

#include <errhandlingapi.h>
#include <memoryapi.h>
#include <minwindef.h>
#include <utils/String.h>
#include <synchapi.h>
#include <winbase.h>
#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <iterator>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>

#undef min

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
  std::optional<std::string_view> const& cwd,
  bool                                   suspended,
  bool                                   no_window
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
   if (!CreateProcess(
         name.size() ? name.data() : nullptr,
         args ? cmd_line.data() : nullptr,
         nullptr,
         nullptr,
         FALSE,
         (suspended ? CREATE_SUSPENDED : 0) | (no_window ? CREATE_NO_WINDOW : 0),
         nullptr,
         cwd ? cwd->data() : nullptr,
         &si,
         &pi
       )) {
      throw std::runtime_error(
        "Couldn't create process \"" + cmd_line + "\" (" + std::to_string(GetLastError()) + ") !"
      );
   }

   return {.handle_ = pi.hProcess, .thread_ = pi.hThread};
}

namespace {

template <class T>
concept span_convertible = requires(T t) { std::span{t}; };

constexpr auto
CopySpan(
  std::span<std::byte const>                 span,
  std::span<std::byte const>::const_iterator it,
  auto*                                      out
) {
   return std::ranges::copy_n(
     span.subspan(std::distance(span.begin(), it), sizeof(out)).begin(),
     sizeof(out),
     reinterpret_cast<std::byte*>(out)
   );
}

template <class T>
constexpr auto
CopySpan(
  std::span<std::byte const>                 span,
  std::span<std::byte const>::const_iterator it,
  std::span<T>                               out
) {
   auto const size = sizeof(T) * out.size();

   return std::ranges::copy_n(
     span.subspan(std::distance(span.begin(), it), size).begin(),
     size,
     reinterpret_cast<std::byte*>(out.data())
   );
}

constexpr auto
CopySpan(std::span<std::byte const> span, std::size_t offset, auto* out) {
   return std::ranges::copy_n(
     span.subspan(offset, sizeof(out)).begin(), sizeof(out), reinterpret_cast<std::byte*>(out)
   );
}

template <class T>
constexpr auto
CopySpan(std::span<std::byte const> span, std::size_t offset, std::span<T> out) {
   auto const size = sizeof(T) * out.size();

   return std::ranges::copy_n(
     span.subspan(offset, size).begin(), size, reinterpret_cast<std::byte*>(out.data())
   );
}

struct ProcInfo {
   std::byte*  base_addr_{};
   std::size_t image_size_{};
};

std::size_t
Aligned(std::size_t size, std::size_t alignment) {
   return ((size + alignment - 1) / alignment) * alignment;
}

bool
LoadPe(
  std::span<std::byte const>     data,
  PeExtHeader const&             pe_ext_header,
  std::span<SectionHeader const> section_headers,
  std::span<std::byte>           addr
) {
   auto header_size = pe_ext_header.size_of_headers_;

   // certain PE files have sectionHeaderSize value > size of PE file itself.
   // this loop handles this situation by find the section that is nearest to the
   // PE header.
   for (auto const& section : section_headers) {
      if (section.pointer_to_raw_data_ < header_size) {
         header_size = section.pointer_to_raw_data_;
      }
   }

   auto const header_it = addr.subspan(0, header_size);
   CopySpan(data, 0, header_it);
   addr = addr.subspan(Aligned(pe_ext_header.size_of_headers_, pe_ext_header.section_alignment_));

   // read the sections
   for (auto const& section : section_headers) {
      if (section.size_of_raw_data_ > 0) {
         auto const section_it =
           addr.subspan(0, std::min(section.size_of_raw_data_, section.virtual_size_));
         CopySpan(data, section.pointer_to_raw_data_, section_it);

         addr = addr.subspan(Aligned(section.virtual_size_, pe_ext_header.section_alignment_));
      } else {
         // this handles the case where the PE file has an empty section. E.g. UPX0 section
         // in UPXed files.

         if (section.virtual_size_) {
            addr = addr.subspan(Aligned(section.virtual_size_, pe_ext_header.section_alignment_));
         }
      }
   }

   return true;
}

std::size_t
ImageSize(PeExtHeader const& pe_ext_header, std::span<SectionHeader const> section_headers) {
   auto const alignment = pe_ext_header.section_alignment_;
   auto       result = ((pe_ext_header.size_of_headers_ + alignment - 1) / alignment) * alignment;

   for (auto const& section_header : section_headers) {
      if (section_header.virtual_size_) {
         result += ((section_header.virtual_size_ + alignment - 1) / alignment) * alignment;
      }
   }

   return result;
}

bool
HasRelocationTable(PeExtHeader const& pe_ext_header) {
   return pe_ext_header.relocation_table_address_ && pe_ext_header.relocation_table_size_;
}

using PTRZwUnmapViewOfSection = DWORD(WINAPI*)(IN HANDLE ProcessHandle, IN PVOID BaseAddress);

struct FixupBlock {
   uint32_t page_rva_;
   uint32_t block_size_;
};

void
DoRelocation(
  PeExtHeader const&   pe_ext_header,
  std::span<std::byte> addr,
  std::span<std::byte> newBase
) {
   if (pe_ext_header.relocation_table_address_ && pe_ext_header.relocation_table_size_) {
      FixupBlock fix_blk{};
      CopySpan(addr, pe_ext_header.relocation_table_address_, &fix_blk);

      auto const delta = std::bit_cast<std::size_t>(newBase.data() - pe_ext_header.image_base_);

      while (fix_blk.block_size_) {
         auto const numEntries = (fix_blk.block_size_ - sizeof(FixupBlock)) >> 1;

         auto offset_ptr =
           addr.subspan(pe_ext_header.relocation_table_address_ + sizeof(FixupBlock));

         for (std::size_t i = 0; i < numEntries; ++i) {
            uint32_t code_location;
            uint16_t offset;
            CopySpan(offset_ptr, 0, &offset);
            CopySpan(addr, fix_blk.page_rva_ + (offset & 0x0FFF), &code_location);

            auto const reloc_type = (offset & 0xF000) >> 12;

            if (reloc_type == 3) {
               code_location += delta;  // @fixme
            } else {
               throw std::runtime_error(std::format("Unknown relocation type = {}", reloc_type));
            }

            offset_ptr = offset_ptr.subspan(sizeof(FixupBlock));
         }

         CopySpan(offset_ptr, 0, &fix_blk);
      }
   }
}

void
DoFork(
  MZHeader const&      mz_header,
  PeExtHeader const&   pe_ext_header,
  std::span<std::byte> addr,
  std::size_t          image_size,
  ProcInfo const&      proc_info,
  Process const&       proc
) {
   PROCESS_INFORMATION pi;
   CONTEXT             ctx;

   // printf("Original EXE loaded (PID = %d).\n", pi.dwProcessId);
   // printf("Original Base Addr = %X, Size = %X\n", childInfo.baseAddr, childInfo.imageSize);

   std::span new_addr{std::bit_cast<std::byte*>(proc_info.base_addr_), proc_info.image_size_};

   if (pe_ext_header.image_base_ == std::bit_cast<uint64_t>(proc_info.base_addr_)
       && image_size <= proc_info.image_size_) {
      // if new EXE has same baseaddr and is its size is <= to the original EXE, just
      // overwrite it in memory
      DWORD old_protect;
      VirtualProtectEx(
        proc.handle_,
        reinterpret_cast<void*>(proc_info.base_addr_),
        proc_info.image_size_,
        PAGE_EXECUTE_READWRITE,
        &old_protect
      );
   } else {
      // get address of ZwUnmapViewOfSection
      auto const pZwUnmapViewOfSection =
        reinterpret_cast<PTRZwUnmapViewOfSection>(reinterpret_cast<void*>(
          GetProcAddress(GetModuleHandle("ntdll.dll"), "ZwUnmapViewOfSection")
        ));

      // try to unmap the original EXE image
      if (pZwUnmapViewOfSection(pi.hProcess, reinterpret_cast<void*>(proc_info.base_addr_)) == 0) {
         // allocate memory for the new EXE image at the prefered imagebase.
         new_addr = {
           std::bit_cast<std::byte*>(VirtualAllocEx(
             proc.handle_,
             reinterpret_cast<void*>(pe_ext_header.image_base_),
             image_size,
             MEM_RESERVE | MEM_COMMIT,
             PAGE_EXECUTE_READWRITE
           )),
           image_size
         };
      }
   }

   if (new_addr.empty() && HasRelocationTable(pe_ext_header)) {
      // if unmap failed but EXE is relocatable, then we try to load the EXE at another
      // location
      new_addr = {
        std::bit_cast<std::byte*>(VirtualAllocEx(
          proc.handle_, nullptr, image_size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE
        )),
        image_size
      };

      if (!new_addr.empty()) {
         // printf("Allocated Mem for New EXE at %X. EXE will be relocated.\n", (unsigned long)v);

         // we've got to do the relocation ourself if we load the image at another
         // memory location
         DoRelocation(pe_ext_header, addr, new_addr);
      }
   }

   // printf("EIP = %X\n", ctx.Eip);
   // printf("EAX = %X\n", ctx.Eax);
   // printf("EBX = %X\n", ctx.Ebx);  // EBX points to PEB
   // printf("ECX = %X\n", ctx.Ecx);
   // printf("EDX = %X\n", ctx.Edx);

   if (new_addr.empty()) {
      throw std::runtime_error("ForkPe: Load failed.  Consider making this EXE relocatable.");
   } else {
      // printf("New EXE Image Size = %X\n", image_size);

      // patch the EXE base addr in PEB (PEB + 8 holds process base addr)
      auto const  pebInfo = std::bit_cast<std::byte*>(ctx.Rbx);
      std::size_t wrote;
      WriteProcessMemory(proc.handle_, pebInfo + 8, &new_addr, sizeof(uint32_t), &wrote);

      // patch the base addr in the PE header of the EXE that we load ourselves
      CopySpan(
        new_addr,
        0,
        reinterpret_cast<decltype(PeExtHeader::image_base_)*>(
          addr
            .subspan(
              mz_header.offset_to_pe_ + sizeof(PeHeader) + offsetof(PeExtHeader, image_base_),
              sizeof(PeExtHeader::image_base_)
            )
            .data()
        )
      );

      if (WriteProcessMemory(proc.handle_, new_addr.data(), addr.data(), image_size, nullptr)) {
         ctx.ContextFlags = CONTEXT_FULL;

         if (new_addr.data() == proc_info.base_addr_) {
            ctx.Rax = pe_ext_header.image_base_
                      + pe_ext_header.address_of_entry_point_;  // eax holds new entry point
         } else {
            // in this case, the DLL was not loaded at the baseaddr, i.e. manual relocation was
            // performed.
            ctx.Rax = std::bit_cast<uint64_t>(&new_addr[pe_ext_header.address_of_entry_point_]
            );  // eax holds new entry point
         }

         // printf("********> EIP = %X\n", ctx.Eip);
         // printf("********> EAX = %X\n", ctx.Eax);

         SetThreadContext(proc.thread_, &ctx);
         ResumeThread(proc.thread_);
      } else {
         throw std::runtime_error("ForkPE: WriteProcessMemory failed");
      }
   }
}

}  // namespace

Process
NewProcessFromMemory(
  std::span<std::byte const>             data,
  std::string_view                       name,
  std::optional<std::string>             args,
  std::optional<std::string_view> const& cwd
) {
   auto const proc = NewProcess("", std::string{name} + (args ? " " + *args : ""), cwd, true);
   auto const throw_error = [&]() constexpr {
      throw std::runtime_error(
        std::format(
          "Couldn't create process \"{}{}\" ({}) !", name, args ? (" " + *args) : "", GetLastError()
        )
          .c_str()
      );
   };

   try {
      CONTEXT ctx{};
      ctx.ContextFlags = CONTEXT_FULL;
      GetThreadContext(proc.thread_, &ctx);

      auto const pebInfo = std::bit_cast<std::byte*>(ctx.Rbx);

      ProcInfo info{};

      std::size_t read;
      ReadProcessMemory(proc.handle_, pebInfo + 8, &info.base_addr_, sizeof(uint16_t), &read);

      if (read != sizeof(uint16_t)) {
         throw_error();
      }

      auto                     cur_addr = info.base_addr_;
      MEMORY_BASIC_INFORMATION mem_info;
      while (VirtualQueryEx(proc.handle_, &cur_addr, &mem_info, sizeof(mem_info))) {
         if (mem_info.State == MEM_FREE) {
            break;
         }
         cur_addr += mem_info.RegionSize;
      }
      info.image_size_ = cur_addr - info.base_addr_;

      auto const [mz_header, pe_header, pe_ext_header, section_headers] = ReadPeInfo(data);
      auto const image_size = ImageSize(pe_ext_header, section_headers);

      std::span const addr{
        reinterpret_cast<std::byte*>(
          VirtualAlloc(nullptr, image_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE)
        ),
        image_size
      };

      if (addr.empty()) {
         throw_error();
      } else {
         LoadPe(data, pe_ext_header, section_headers, addr);
         DoFork(mz_header, pe_ext_header, addr, image_size, info, proc);
      }
   } catch (...) {
      TerminateProcess(proc.handle_, EXIT_FAILURE);
      throw;
   }

   return proc;
}

std::tuple<MZHeader, PeHeader, PeExtHeader, std::vector<SectionHeader>>
ReadPeInfo(std::span<std::byte const> data) {
   if (data.size() < sizeof(MZHeader)) {
      throw std::runtime_error("ReadPE: Data size too small");
   }

   MZHeader mz_header{};
   CopySpan(data, 0, &mz_header);

   if (mz_header.signature_ != 0x5a4d) {
      throw std::runtime_error("ReadPE: does not have MZ header");
   }

   if (data.size() < mz_header.offset_to_pe_ + sizeof(PeHeader)) {
      throw std::runtime_error("ReadPE: Data size too small");
   }

   PeHeader pe_header{};
   auto     it = CopySpan(data, mz_header.offset_to_pe_, &pe_header);

   if (pe_header.size_of_option_header_ != sizeof(PeExtHeader)) {
      throw std::runtime_error("ReadPE: Unexpected option header size");
   }

   PeExtHeader pe_ext_header{};
   it = CopySpan(data, it.in, &pe_ext_header);

   std::vector<SectionHeader> section_headers{};
   section_headers.resize(pe_header.num_sections_);

   CopySpan(data, it.in, std::span{section_headers});
   return {mz_header, pe_header, pe_ext_header, section_headers};
}

std::string
GetExecutablePath() {
   std::string result;
   result.resize(MAX_PATH);
   result.resize(GetModuleFileName(nullptr, result.data(), result.size()));

   return result;
}

}  // namespace win32