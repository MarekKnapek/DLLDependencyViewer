#pragma once


#include "smart_menu.h"

#include <array>
#include <string>
#include <cstdint>

#include "../nogui/my_windows.h"


class main_window;


class tree_view
{
public:
	tree_view() = delete;
	tree_view(HWND const parent, main_window& mw);
	tree_view(tree_view const&) = delete;
	tree_view(tree_view&&) noexcept = delete;
	tree_view& operator=(tree_view const&) = delete;
	tree_view& operator=(tree_view&&) noexcept = delete;
	~tree_view();
public:
	HWND get_hwnd() const;
	void on_notify(NMHDR& nmhdr);
	void on_getdispinfow(NMHDR& nmhdr);
	void on_selchangedw(NMHDR& nmhdr);
	void on_context_menu(LPARAM const lparam);
	void on_menu(std::uint16_t const menu_id);
	void on_menu_orig();
	void on_accel_orig();
private:
	smart_menu create_menu();
	void select_original_instance();
private:
	HWND const m_hwnd;
	main_window& m_main_window;
	smart_menu const m_menu;
	std::array<std::wstring, 4> m_tmp_strings;
	unsigned m_tmp_string_idx;
};