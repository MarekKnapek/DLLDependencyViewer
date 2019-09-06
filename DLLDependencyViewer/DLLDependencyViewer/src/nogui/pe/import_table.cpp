#include "import_table.h"

#include "mz.h"
#include "coff_full.h"
#include "../assert.h"

#include <algorithm>


template<typename T>
auto pe_min(T const& a, T const& b)
{
	return b < a ? b : a;
}


bool operator==(pe_import_directory_entry const& a, pe_import_directory_entry const& b)
{
	return
		a.m_import_lookup_table == b.m_import_lookup_table &&
		a.m_date_time == b.m_date_time &&
		a.m_forwarder_chain == b.m_forwarder_chain &&
		a.m_name == b.m_name &&
		a.m_import_adress_table == b.m_import_adress_table;
}

bool operator==(pe_import_lookup_entry_32 const& a, pe_import_lookup_entry_32 const& b)
{
	return a.m_value == b.m_value;
}

bool operator==(pe_import_lookup_entry_64 const& a, pe_import_lookup_entry_64 const& b)
{
	return a.m_value == b.m_value;
}

bool operator==(pe_delay_load_descriptor const& a, pe_delay_load_descriptor const& b)
{
	return
		a.m_attributes == b.m_attributes &&
		a.m_dll_name_rva == b.m_dll_name_rva &&
		a.m_module_handle_rva == b.m_module_handle_rva &&
		a.m_import_address_table_rva == b.m_import_address_table_rva &&
		a.m_import_name_table_rva == b.m_import_name_table_rva &&
		a.m_bound_import_address_table_rva == b.m_bound_import_address_table_rva &&
		a.m_unload_information_table_rva == b.m_unload_information_table_rva &&
		a.m_time_date_stamp == b.m_time_date_stamp;
}


bool pe_parse_import_table(void const* const& fd, int const& file_size, pe_import_directory_table& idt)
{
	char const* const file_data = static_cast<char const*>(fd);
	pe_dos_header const& dos_hdr = *reinterpret_cast<pe_dos_header const*>(file_data + 0);
	pe_coff_full_32_64 const& coff_hdr = *reinterpret_cast<pe_coff_full_32_64 const*>(file_data + dos_hdr.m_pe_offset);
	bool const is_32 = coff_hdr.m_32.m_standard.m_signature == s_pe_coff_optional_sig_32;
	std::uint32_t const dir_tbl_cnt = is_32 ? coff_hdr.m_32.m_windows.m_data_directory_count : coff_hdr.m_64.m_windows.m_data_directory_count;
	if(!(static_cast<int>(pe_e_directory_table::import_table) < dir_tbl_cnt))
	{
		idt.m_table = nullptr;
		idt.m_count = 0;
		return true;
	}
	pe_data_directory const* const dir_tbl = reinterpret_cast<pe_data_directory const*>(file_data + dos_hdr.m_pe_offset + (is_32 ? sizeof(pe_coff_full_32) : sizeof(pe_coff_full_64)));
	pe_data_directory const& imp_tbl = dir_tbl[static_cast<int>(pe_e_directory_table::import_table)];
	if(imp_tbl.m_va == 0 || imp_tbl.m_size == 0)
	{
		idt.m_table = nullptr;
		idt.m_count = 0;
		return true;
	}
	pe_section_header const* sct;
	std::uint32_t const imp_dir_tbl_raw = pe_find_object_in_raw(file_data, file_size, imp_tbl.m_va, imp_tbl.m_size, sct);
	WARN_M_R(imp_dir_tbl_raw != 0, L"Import directory table not found in any section.", false);
	std::uint32_t const imp_dir_tbl_cnt_max = pe_min(1u * 1024u * 1024u, imp_tbl.m_size / static_cast<int>(sizeof(pe_import_directory_entry)));
	pe_import_directory_entry const* const d_tbl = reinterpret_cast<pe_import_directory_entry const*>(file_data + imp_dir_tbl_raw);
	pe_import_directory_entry const* const d_tbl_end_max = d_tbl + imp_dir_tbl_cnt_max;
	auto const it = std::find(d_tbl, d_tbl_end_max, pe_import_directory_entry{});
	WARN_M_R(it != d_tbl_end_max, L"Could not found import directory table size.", false);
	int const imp_dir_tbl_cnt = static_cast<int>(it - d_tbl);
	idt.m_table = d_tbl;
	idt.m_count = imp_dir_tbl_cnt;
	return true;
}

bool pe_parse_import_dll_name(void const* const& fd, int const& file_size, pe_import_directory_entry const& ide, pe_string& str)
{
	char const* const file_data = static_cast<char const*>(fd);
	pe_dos_header const& dos_hdr = *reinterpret_cast<pe_dos_header const*>(file_data + 0);
	pe_coff_full_32_64 const& coff_hdr = *reinterpret_cast<pe_coff_full_32_64 const*>(file_data + dos_hdr.m_pe_offset);
	WARN_M_R(ide.m_name != 0, L"Import directory entry has no DLL name.", false);
	pe_section_header const* sct;
	std::uint32_t const dll_name_raw = pe_find_object_in_raw(file_data, file_size, ide.m_name, 2, sct);
	WARN_M_R(dll_name_raw != 0, L"Import DLL name not found in any section.", false);
	std::uint32_t const dll_name_len_max = pe_min(256u, sct->m_raw_ptr + sct->m_raw_size - dll_name_raw);
	char const* const dll_name = reinterpret_cast<char const*>(file_data + dll_name_raw);
	char const* const dll_name_end_max = dll_name + dll_name_len_max;
	auto const it = std::find(dll_name, dll_name_end_max, '\0');
	WARN_M_R(it != dll_name && it != dll_name_end_max, L"Could not find import DLL name length.", false);
	int const dll_name_len = static_cast<int>(it - dll_name);
	WARN_M_R(pe_is_ascii(dll_name, dll_name_len), L"Import DLL name shall be ASCII.", false);
	str.m_str = dll_name;
	str.m_len = dll_name_len;
	return true;
}

bool pe_parse_import_address_table(void const* const& fd, int const& file_size, pe_import_directory_entry const& ide, pe_import_address_table& iat_out)
{
	char const* const file_data = static_cast<char const*>(fd);
	pe_dos_header const& dos_hdr = *reinterpret_cast<pe_dos_header const*>(file_data + 0);
	pe_coff_full_32_64 const& coff_hdr = *reinterpret_cast<pe_coff_full_32_64 const*>(file_data + dos_hdr.m_pe_offset);
	bool const is_32 = coff_hdr.m_32.m_standard.m_signature == s_pe_coff_optional_sig_32;
	std::uint32_t const iat_rva = ide.m_import_lookup_table != 0 ? ide.m_import_lookup_table : ide.m_import_adress_table;
	WARN_M_R(iat_rva != 0, L"Import address table not found.", false);
	pe_section_header const* sct;
	std::uint32_t const iat_raw = pe_find_object_in_raw(file_data, file_size, iat_rva, is_32 ? sizeof(pe_import_lookup_entry_32) : sizeof(pe_import_lookup_entry_64), sct);
	WARN_M_R(iat_raw != 0, L"Could not find import address table in any section.", false);
	if(is_32)
	{
		std::uint32_t const iat_cnt_max = pe_min(64u * 1024u, (sct->m_raw_ptr + sct->m_raw_size - iat_raw) / static_cast<int>(sizeof(pe_import_lookup_entry_32)));
		pe_import_lookup_entry_32 const* const iat = reinterpret_cast<pe_import_lookup_entry_32 const*>(file_data + iat_raw);
		pe_import_lookup_entry_32 const* const iat_end_max = iat + iat_cnt_max;
		auto const it = std::find(iat, iat_end_max, pe_import_lookup_entry_32{});
		WARN_M_R(it != iat_end_max, L"Could not find import address table size.", false);
		int const iat_cnt = static_cast<int>(it - iat);
		iat_out.m_raw = iat_raw;
		iat_out.m_count = iat_cnt;
		return true;
	}
	else
	{
		std::uint32_t const iat_cnt_max = pe_min(64u * 1024u, (sct->m_raw_ptr + sct->m_raw_size - iat_raw) / static_cast<int>(sizeof(pe_import_lookup_entry_64)));
		pe_import_lookup_entry_64 const* const iat = reinterpret_cast<pe_import_lookup_entry_64 const*>(file_data + iat_raw);
		pe_import_lookup_entry_64 const* const iat_end_max = iat + iat_cnt_max;
		auto const it = std::find(iat, iat_end_max, pe_import_lookup_entry_64{});
		WARN_M_R(it != iat_end_max, L"Could not find import address table size.", false);
		int const iat_cnt = static_cast<int>(it - iat);
		iat_out.m_raw = iat_raw;
		iat_out.m_count = iat_cnt;
		return true;
	}
}

bool pe_parse_import_address(void const* const& fd, int const& file_size, pe_import_address_table const& iat_in, int const& idx, bool& is_ordinal_out, std::uint16_t& ordinal_out, pe_hint_name& hint_name_out)
{
	char const* const file_data = static_cast<char const*>(fd);
	pe_dos_header const& dos_hdr = *reinterpret_cast<pe_dos_header const*>(file_data + 0);
	pe_coff_full_32_64 const& coff_hdr = *reinterpret_cast<pe_coff_full_32_64 const*>(file_data + dos_hdr.m_pe_offset);
	bool const is_32 = coff_hdr.m_32.m_standard.m_signature == s_pe_coff_optional_sig_32;
	if(is_32)
	{
		pe_import_lookup_entry_32 const* const iat = reinterpret_cast<pe_import_lookup_entry_32 const*>(file_data + iat_in.m_raw);
		pe_import_lookup_entry_32 const& ia = iat[idx];
		bool const is_ordinal = (ia.m_value & 0x80000000) != 0;
		if(is_ordinal)
		{
			WARN_M_R((ia.m_value & 0x7fff0000) == 0, L"Bits 30-15 must be 0.", false);
			std::uint16_t const ordinal = ia.m_value & 0x0000ffff;
			is_ordinal_out = true;
			ordinal_out = ordinal;
			return true;
		}
		else
		{
			std::uint32_t const hint_name_rva = ia.m_value & 0x7fffffff;
			pe_section_header const* sct;
			std::uint32_t const hint_name_raw = pe_find_object_in_raw(file_data, file_size, hint_name_rva, sizeof(std::uint16_t) + 2 * sizeof(char), sct);
			WARN_M_R(hint_name_raw != 0, L"Could not parse import address name.", false);
			std::uint16_t const hint = *reinterpret_cast<std::uint16_t const*>(file_data + hint_name_raw + 0);
			std::uint32_t const name_len_max = pe_min(64u * 1024u, (sct->m_raw_ptr + sct->m_raw_size - (hint_name_raw + static_cast<int>(sizeof(std::uint16_t)))) / static_cast<int>(sizeof(char)));
			char const* const name = reinterpret_cast<char const*>(file_data + hint_name_raw + sizeof(std::uint16_t));
			char const* const name_end_max = name + name_len_max;
			auto const it = std::find(name, name_end_max, '\0');
			WARN_M_R(it != name_end_max, L"Could not find import address name length.", false);
			int const name_len = static_cast<int>(it - name);
			WARN_M_R(pe_is_ascii(name, static_cast<int>(name_len)), L"Import name shall be ASCII.", false);
			is_ordinal_out = false;
			hint_name_out.m_hint = hint;
			hint_name_out.m_name.m_str = name;
			hint_name_out.m_name.m_len = name_len;
			return true;
		}
	}
	else
	{
		pe_import_lookup_entry_64 const* const iat = reinterpret_cast<pe_import_lookup_entry_64 const*>(file_data + iat_in.m_raw);
		pe_import_lookup_entry_64 const& ia = iat[idx];
		bool const is_ordinal = (ia.m_value & 0x8000000000000000ull) != 0;
		if(is_ordinal)
		{
			WARN_M_R((ia.m_value & 0x7fffffffffff0000ull) == 0, L"Bits 62-15 must be 0.", false);
			std::uint16_t const ordinal = ia.m_value & 0x000000000000ffffull;
			is_ordinal_out = true;
			ordinal_out = ordinal;
			return true;
		}
		else
		{
			WARN_M_R((ia.m_value & 0x7fffffff80000000ull) == 0, L"Bits 62-31 must be 0.", false);
			std::uint32_t const hint_name_rva = ia.m_value & 0x000000007fffffffull;
			pe_section_header const* sct;
			std::uint32_t const hint_name_raw = pe_find_object_in_raw(file_data, file_size, hint_name_rva, sizeof(std::uint16_t) + 2 * sizeof(char), sct);
			WARN_M_R(hint_name_raw != 0, L"Could not parse import address name.", false);
			std::uint16_t const hint = *reinterpret_cast<std::uint16_t const*>(file_data + hint_name_raw + 0);
			std::uint32_t const name_len_max = pe_min(64u * 1024u, (sct->m_raw_ptr + sct->m_raw_size - (hint_name_raw + static_cast<int>(sizeof(std::uint16_t)))) / static_cast<int>(sizeof(char)));
			char const* const name = reinterpret_cast<char const*>(file_data + hint_name_raw + sizeof(std::uint16_t));
			char const* const name_end_max = name + name_len_max;
			auto const it = std::find(name, name_end_max, '\0');
			WARN_M_R(it != name_end_max, L"Could not find import address name length.", false);
			int const name_len = static_cast<int>(it - name);
			WARN_M_R(pe_is_ascii(name, static_cast<int>(name_len)), L"Import name shall be ASCII.", false);
			is_ordinal_out = false;
			hint_name_out.m_hint = hint;
			hint_name_out.m_name.m_str = name;
			hint_name_out.m_name.m_len = name_len;
			return true;
		}
	}
}

bool pe_parse_delay_import_table(void const* const& fd, int const& file_size, pe_delay_import_table& dlit)
{
	char const* const file_data = static_cast<char const*>(fd);
	pe_dos_header const& dos_hdr = *reinterpret_cast<pe_dos_header const*>(file_data + 0);
	pe_coff_full_32_64 const& coff_hdr = *reinterpret_cast<pe_coff_full_32_64 const*>(file_data + dos_hdr.m_pe_offset);
	bool const is_32 = coff_hdr.m_32.m_standard.m_signature == s_pe_coff_optional_sig_32;
	std::uint32_t const dir_tbl_cnt = is_32 ? coff_hdr.m_32.m_windows.m_data_directory_count : coff_hdr.m_64.m_windows.m_data_directory_count;
	if(!(static_cast<int>(pe_e_directory_table::delay_import_descriptor) < dir_tbl_cnt))
	{
		dlit.m_table = nullptr;
		dlit.m_count = 0;
		return true;
	}
	pe_data_directory const* const dir_tbl = reinterpret_cast<pe_data_directory const*>(file_data + dos_hdr.m_pe_offset + (is_32 ? sizeof(pe_coff_full_32) : sizeof(pe_coff_full_64)));
	pe_data_directory const& dimp_tbl = dir_tbl[static_cast<int>(pe_e_directory_table::delay_import_descriptor)];
	if(dimp_tbl.m_va == 0 || dimp_tbl.m_size == 0)
	{
		dlit.m_table = nullptr;
		dlit.m_count = 0;
		return true;
	}
	pe_section_header const* sct;
	std::uint32_t const dimp_dir_tbl_raw = pe_find_object_in_raw(file_data, file_size, dimp_tbl.m_va, dimp_tbl.m_size, sct);
	WARN_M_R(dimp_dir_tbl_raw != 0, L"Delay import directory table not found in any section.", false);
	std::uint32_t const dimp_dir_tbl_cnt_max = pe_min(1u * 1024u * 1024u, dimp_tbl.m_size / static_cast<int>(sizeof(pe_delay_load_descriptor)));
	pe_delay_load_descriptor const* const dld_tbl = reinterpret_cast<pe_delay_load_descriptor const*>(file_data + dimp_dir_tbl_raw);
	pe_delay_load_descriptor const* const dld_tbl_end_max = dld_tbl + dimp_dir_tbl_cnt_max;
	auto const it = std::find(dld_tbl, dld_tbl_end_max, pe_delay_load_descriptor{});
	WARN_M_R(it != dld_tbl_end_max, L"Could not found delay import directory table size.", false);
	int const dimp_dir_tbl_cnt = static_cast<int>(it - dld_tbl);
	dlit.m_table = dld_tbl;
	dlit.m_count = dimp_dir_tbl_cnt;
	return true;
}

bool pe_parse_delay_import_dll_name(void const* const& fd, int const& file_size, pe_delay_load_descriptor const& dld, pe_string& str)
{
	char const* const file_data = static_cast<char const*>(fd);
	pe_dos_header const& dos_hdr = *reinterpret_cast<pe_dos_header const*>(file_data + 0);
	pe_coff_full_32_64 const& coff_hdr = *reinterpret_cast<pe_coff_full_32_64 const*>(file_data + dos_hdr.m_pe_offset);
	WARN_M_R(dld.m_dll_name_rva != 0, L"Delay import directory entry has no DLL name.", false);
	bool const is_32 = coff_hdr.m_32.m_standard.m_signature == s_pe_coff_optional_sig_32;
	bool const delay_ver_2 = (dld.m_attributes & 1u) != 0;
	WARN_M_R(is_32 ? true : (delay_ver_2 || (coff_hdr.m_64.m_windows.m_image_base < 0x00000000ffffffffull)), L"Image base is damn too high.", false);
	std::uint32_t const delay_dll_name_rva = dld.m_dll_name_rva - (delay_ver_2 ? 0u : (is_32 ? coff_hdr.m_32.m_windows.m_image_base : static_cast<std::uint32_t>(coff_hdr.m_64.m_windows.m_image_base)));
	pe_section_header const* sct;
	std::uint32_t const dll_name_raw = pe_find_object_in_raw(file_data, file_size, delay_dll_name_rva, 2, sct);
	WARN_M_R(dll_name_raw != 0, L"Delay import DLL name not found in any section.", false);
	std::uint32_t const dll_name_len_max = pe_min(256u, sct->m_raw_ptr + sct->m_raw_size - dll_name_raw);
	char const* const dll_name = reinterpret_cast<char const*>(file_data + dll_name_raw);
	char const* const dll_name_end_max = dll_name + dll_name_len_max;
	auto const it = std::find(dll_name, dll_name_end_max, '\0');
	WARN_M_R(it != dll_name && it != dll_name_end_max, L"Could not find delay import DLL name length.", false);
	int const dll_name_len = static_cast<int>(it - dll_name);
	WARN_M_R(pe_is_ascii(dll_name, dll_name_len), L"Delay import DLL name shall be ASCII.", false);
	str.m_str = dll_name;
	str.m_len = dll_name_len;
	return true;
}

bool pe_parse_delay_import_address_table(void const* const& fd, int const& file_size, pe_delay_load_descriptor const& dld, pe_delay_load_import_address_table& dliat_out)
{
	char const* const file_data = static_cast<char const*>(fd);
	pe_dos_header const& dos_hdr = *reinterpret_cast<pe_dos_header const*>(file_data + 0);
	pe_coff_full_32_64 const& coff_hdr = *reinterpret_cast<pe_coff_full_32_64 const*>(file_data + dos_hdr.m_pe_offset);
	WARN_M_R(dld.m_import_name_table_rva != 0, L"Delay import address table not found.", false);
	bool const delay_ver_2 = (dld.m_attributes & 1u) != 0;
	bool const is_32 = coff_hdr.m_32.m_standard.m_signature == s_pe_coff_optional_sig_32;
	std::uint32_t const dliat_rva = dld.m_import_name_table_rva - (delay_ver_2 ? 0u : (is_32 ? coff_hdr.m_32.m_windows.m_image_base : static_cast<std::uint32_t>(coff_hdr.m_64.m_windows.m_image_base)));
	pe_section_header const* sct;
	std::uint32_t const dliat_raw = pe_find_object_in_raw(file_data, file_size, dliat_rva, is_32 ? sizeof(pe_import_lookup_entry_32) : sizeof(pe_import_lookup_entry_64), sct);
	WARN_M_R(dliat_raw != 0, L"Could not find delay load import address table in any section.", false);
	if(is_32)
	{
		std::uint32_t const dliat_cnt_max = pe_min(64u * 1024u, (sct->m_raw_ptr + sct->m_raw_size - dliat_raw) / static_cast<int>(sizeof(pe_import_lookup_entry_32)));
		pe_import_lookup_entry_32 const* const dliat = reinterpret_cast<pe_import_lookup_entry_32 const*>(file_data + dliat_raw);
		pe_import_lookup_entry_32 const* const dliat_end_max = dliat + dliat_cnt_max;
		auto const it = std::find(dliat, dliat_end_max, pe_import_lookup_entry_32{});
		WARN_M_R(it != dliat_end_max, L"Could not find delay import address table size.", false);
		int const dliat_cnt = static_cast<int>(it - dliat);
		dliat_out.m_raw = dliat_raw;
		dliat_out.m_count = dliat_cnt;
		return true;
	}
	else
	{
		std::uint32_t const dliat_cnt_max = pe_min(64u * 1024u, (sct->m_raw_ptr + sct->m_raw_size - dliat_raw) / static_cast<int>(sizeof(pe_import_lookup_entry_64)));
		pe_import_lookup_entry_64 const* const dliat = reinterpret_cast<pe_import_lookup_entry_64 const*>(file_data + dliat_raw);
		pe_import_lookup_entry_64 const* const dliat_end_max = dliat + dliat_cnt_max;
		auto const it = std::find(dliat, dliat_end_max, pe_import_lookup_entry_64{});
		WARN_M_R(it != dliat_end_max, L"Could not find delay import address table size.", false);
		int const dliat_cnt = static_cast<int>(it - dliat);
		dliat_out.m_raw = dliat_raw;
		dliat_out.m_count = dliat_cnt;
		return true;
	}
}

bool pe_parse_delay_import_address(void const* const& fd, int const& file_size, pe_delay_load_descriptor const& dld, pe_delay_load_import_address_table const& dliat_in, int const& idx, bool& is_ordinal_out, std::uint16_t& ordinal_out, pe_hint_name& hint_name_out)
{
	char const* const file_data = static_cast<char const*>(fd);
	pe_dos_header const& dos_hdr = *reinterpret_cast<pe_dos_header const*>(file_data + 0);
	pe_coff_full_32_64 const& coff_hdr = *reinterpret_cast<pe_coff_full_32_64 const*>(file_data + dos_hdr.m_pe_offset);
	bool const is_32 = coff_hdr.m_32.m_standard.m_signature == s_pe_coff_optional_sig_32;
	if(is_32)
	{
		pe_import_lookup_entry_32 const* const dliat = reinterpret_cast<pe_import_lookup_entry_32 const*>(file_data + dliat_in.m_raw);
		pe_import_lookup_entry_32 const& dlia = dliat[idx];
		bool const is_ordinal = (dlia.m_value & 0x80000000) != 0;
		if(is_ordinal)
		{
			WARN_M_R((dlia.m_value & 0x7fff0000) == 0, L"Bits 30-15 must be 0.", false);
			std::uint16_t const ordinal = dlia.m_value & 0x0000ffff;
			is_ordinal_out = true;
			ordinal_out = ordinal;
			return true;
		}
		else
		{
			bool const delay_ver_2 = (dld.m_attributes & 1u) != 0;
			std::uint32_t const hint_name_rva = (dlia.m_value & 0x7fffffff) - (delay_ver_2 ? 0u : coff_hdr.m_32.m_windows.m_image_base);
			pe_section_header const* sct;
			std::uint32_t const hint_name_raw = pe_find_object_in_raw(file_data, file_size, hint_name_rva, sizeof(std::uint16_t) + 2 * sizeof(char), sct);
			WARN_M_R(hint_name_raw != 0, L"Could not parse delay import address name.", false);
			std::uint16_t const hint = *reinterpret_cast<std::uint16_t const*>(file_data + hint_name_raw + 0);
			std::uint32_t const name_len_max = pe_min(64u * 1024u, (sct->m_raw_ptr + sct->m_raw_size - (hint_name_raw + static_cast<int>(sizeof(std::uint16_t)))) / static_cast<int>(sizeof(char)));
			char const* const name = reinterpret_cast<char const*>(file_data + hint_name_raw + sizeof(std::uint16_t));
			char const* const name_end_max = name + name_len_max;
			auto const it = std::find(name, name_end_max, '\0');
			WARN_M_R(it != name_end_max, L"Could not find delay import address name length.", false);
			int const name_len = static_cast<int>(it - name);
			WARN_M_R(pe_is_ascii(name, static_cast<int>(name_len)), L"Import name shall be ASCII.", false);
			is_ordinal_out = false;
			hint_name_out.m_hint = hint;
			hint_name_out.m_name.m_str = name;
			hint_name_out.m_name.m_len = name_len;
			return true;
		}
	}
	else
	{
		pe_import_lookup_entry_64 const* const dliat = reinterpret_cast<pe_import_lookup_entry_64 const*>(file_data + dliat_in.m_raw);
		pe_import_lookup_entry_64 const& dlia = dliat[idx];
		bool const is_ordinal = (dlia.m_value & 0x8000000000000000ull) != 0;
		if(is_ordinal)
		{
			WARN_M_R((dlia.m_value & 0x7fffffffffff0000ull) == 0, L"Bits 62-15 must be 0.", false);
			std::uint16_t const ordinal = dlia.m_value & 0x000000000000ffffull;
			is_ordinal_out = true;
			ordinal_out = ordinal;
			return true;
		}
		else
		{
			WARN_M_R((dlia.m_value & 0x7fffffff80000000ull) == 0, L"Bits 62-31 must be 0.", false);
			bool const delay_ver_2 = (dld.m_attributes & 1u) != 0;
			std::uint32_t const hint_name_rva = (dlia.m_value & 0x000000007fffffffull) - (delay_ver_2 ? 0u : static_cast<std::uint32_t>(coff_hdr.m_64.m_windows.m_image_base));
			pe_section_header const* sct;
			std::uint32_t const hint_name_raw = pe_find_object_in_raw(file_data, file_size, hint_name_rva, sizeof(std::uint16_t) + 2, sct);
			WARN_M_R(hint_name_raw != 0, L"Could not parse delay import address name.", false);
			std::uint16_t const hint = *reinterpret_cast<std::uint16_t const*>(file_data + hint_name_raw + 0);
			std::uint32_t const name_len_max = pe_min(64u * 1024u, (sct->m_raw_ptr + sct->m_raw_size - (hint_name_raw + static_cast<int>(sizeof(std::uint16_t)))) / static_cast<int>(sizeof(char)));
			char const* const name = reinterpret_cast<char const*>(file_data + hint_name_raw + sizeof(std::uint16_t));
			char const* const name_end_max = name + name_len_max;
			auto const it = std::find(name, name_end_max, '\0');
			WARN_M_R(it != name_end_max, L"Could not find import address name length.", false);
			int const name_len = static_cast<int>(it - name);
			WARN_M_R(pe_is_ascii(name, static_cast<int>(name_len)), L"Import name shall be ASCII.", false);
			is_ordinal_out = false;
			hint_name_out.m_hint = hint;
			hint_name_out.m_name.m_str = name;
			hint_name_out.m_name.m_len = name_len;
			return true;
		}
	}
}