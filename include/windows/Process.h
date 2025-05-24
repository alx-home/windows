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

#include <Windows.h>
#include <winnt.h>
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace win32 {

struct Process {
   ~Process();

   HANDLE handle_;
   HANDLE thread_;
};

Process NewProcess(
  std::string_view                       name,
  std::optional<std::string>             args      = std::nullopt,
  std::optional<std::string_view> const& cwd       = std::nullopt,
  bool                                   suspended = false,
  bool                                   no_window = false
);

Process NewProcessFromMemory(
  std::span<std::byte const>             data,
  std::string_view                       name,
  std::optional<std::string>             args = std::nullopt,
  std::optional<std::string_view> const& cwd  = std::nullopt
);

std::string GetExecutablePath();

struct PeHeader {
   uint32_t signature_;
   uint16_t machine_;
   uint16_t num_sections_;
   uint32_t time_date_stamp_;
   uint32_t pointer_to_symbol_table_;
   uint32_t num_of_symbols_;
   uint16_t size_of_option_header_;
   uint16_t characteristics_;
};
static_assert(sizeof(PeHeader) == 24);

struct PeExtHeader {
   uint16_t magic_;
   uint8_t  major_linker_version_;
   uint8_t  minor_linker_version_;
   uint32_t size_of_code_;
   uint32_t size_of_initialized_data_;
   uint32_t size_of_uninitialized_data_;
   uint32_t address_of_entry_point_;
   uint32_t base_of_code_;
   uint32_t base_of_data_;
   uint32_t image_base_;
   uint32_t section_alignment_;
   uint32_t file_alignment_;
   uint16_t major_os_version_;
   uint16_t minor_os_version_;
   uint16_t major_image_version_;
   uint16_t minor_image_version_;
   uint16_t major_subsystem_version_;
   uint16_t minor_subsystem_version_;
   uint32_t reserved1_;
   uint32_t size_of_image_;
   uint32_t size_of_headers_;
   uint32_t checksum_;
   uint16_t subsystem_;
   uint16_t dll_characteristics_;
   uint32_t size_of_stack_reserve_;
   uint32_t size_of_stack_commit_;
   uint32_t size_of_heap_reserve_;
   uint32_t size_of_heap_commit_;
   uint32_t loader_flags_;
   uint32_t number_of_rva_and_sizes_;
   uint32_t export_table_address_;
   uint32_t export_table_size_;
   uint32_t import_table_address_;
   uint32_t import_table_size_;
   uint32_t resource_table_address_;
   uint32_t resource_table_size_;
   uint32_t exception_table_address_;
   uint32_t exception_table_size_;
   uint32_t cert_file_pointer_;
   uint32_t cert_table_size_;
   uint32_t relocation_table_address_;
   uint32_t relocation_table_size_;
   uint32_t debug_data_address_;
   uint32_t debug_data_size_;
   uint32_t arch_data_address_;
   uint32_t arch_data_size_;
   uint32_t global_ptr_address_;
   uint32_t global_ptr_size_;
   uint32_t tls_table_address_;
   uint32_t tls_table_size_;
   uint32_t load_config_table_address_;
   uint32_t load_config_table_size_;
   uint32_t bound_import_table_address_;
   uint32_t bound_import_table_size_;
   uint32_t import_address_table_address_;
   uint32_t import_address_table_size_;
   uint32_t delay_import_desc_address_;
   uint32_t delay_import_desc_size_;
   uint32_t com_header_address_;
   uint32_t com_header_size_;
   uint32_t reserved2_;
   uint32_t reserved3_;
};
static_assert(sizeof(PeExtHeader) == 224);

struct SectionHeader {
   std::array<uint8_t, 8> section_name_;
   uint32_t               virtual_size_;
   uint32_t               virtual_address_;
   uint32_t               size_of_raw_data_;
   uint32_t               pointer_to_raw_data_;
   uint32_t               pointer_to_relocations_;
   uint32_t               pointer_to_line_numbers_;
   uint16_t               number_of_relocations_;
   uint16_t               number_of_line_numbers_;
   uint32_t               characteristics_;
};
static_assert(sizeof(SectionHeader) == 40);

struct MZHeader {
   uint16_t                signature_;
   uint16_t                part_pag_;
   uint16_t                page_cnt_;
   uint16_t                relo_cnt_;
   uint16_t                hdr_size_;
   uint16_t                min_mem_;
   uint16_t                max_mem_;
   uint16_t                relo_ss_;
   uint16_t                exe_sp_;
   uint16_t                chksum_;
   uint16_t                exe_ip_;
   uint16_t                relo_cs_;
   uint16_t                tabl_off_;
   uint16_t                overlay_;
   std::array<uint8_t, 32> reserved_;
   uint32_t                offset_to_pe_;
};
static_assert(sizeof(MZHeader) == 64);

struct ImportDirEntry {
   uint32_t import_lookup_table_;
   uint32_t time_date_stamp_;
   uint32_t forwarder_chain_;
   uint32_t name_rva_;
   uint32_t import_address_table_;
};
static_assert(sizeof(ImportDirEntry) == 20);

std::tuple<MZHeader, PeHeader, PeExtHeader, std::vector<SectionHeader>>
  ReadPeInfo(std::span<std::byte const>);

}  // namespace win32