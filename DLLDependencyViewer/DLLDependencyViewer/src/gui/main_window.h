#pragma once


#include "processor.h"
#include "smart_menu.h"
#include "splitter_window.h"
#include "tree_view.h"
#include "settings.h"
#include "export_view.h"

#include "../nogui/pe.h"

#include <array>
#include <queue>
#include <string>
#include <utility>

#include "../nogui/my_windows.h"


struct _TREEITEM;
typedef struct _TREEITEM* HTREEITEM;
class main_window;
typedef void* idle_task_param_t;
typedef void(* idle_task_t)(main_window&, idle_task_param_t const);
struct get_symbols_from_addresses_task_t;


#define wm_main_window_add_idle_task (WM_USER + 0)
#define wm_main_window_process_on_idle (WM_USER + 1)
#define wm_main_window_take_finished_dbg_task (WM_USER + 2)


class main_window
{
public:
	static void register_class();
	static void create_accel_table();
	static HACCEL get_accell_table();
	static void destroy_accel_table();
public:
	main_window();
	main_window(main_window const&) = delete;
	main_window(main_window&&) = delete;
	main_window& operator=(main_window const&) = delete;
	main_window& operator=(main_window&&) = delete;
	~main_window();
public:
	HWND get_hwnd() const;
private:
	static HMENU create_menu();
	static HMENU create_import_menu();
	static HWND create_toolbar(HWND const& parent);
	static LRESULT CALLBACK class_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
private:
	LRESULT on_message(UINT msg, WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_destroy(WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_size(WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_close(WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_notify(WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_contextmenu(WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_command(WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_dropfiles(WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_main_window_add_idle_task(WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_main_window_process_on_idle(WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_main_window_take_finished_dbg_task(WPARAM wparam, LPARAM lparam);
	void on_menu(WPARAM const wparam);
	void on_accelerator(WPARAM const wparam);
	void on_toolbar(WPARAM const wparam);
	void on_tree_selchangedw();
	void on_import_notify(NMHDR& nmhdr);
	void on_import_getdispinfow(NMHDR& nmhdr);
	wchar_t const* on_import_get_col_type(pe_import_entry const& import_entry);
	wchar_t const* on_import_get_col_ordinal(pe_import_entry const& import_entry, file_info const& fi);
	wchar_t const* on_import_get_col_hint(pe_import_entry const& import_entry, file_info const& fi);
	wchar_t const* on_import_get_col_name(pe_import_entry const& import_entry, file_info const& fi);
	void on_import_context_menu(WPARAM wparam, LPARAM lparam);
	void on_toolbar_notify(NMHDR& nmhdr);
	void on_menu_open();
	void on_menu_exit();
	void on_menu_paths();
	void on_import_menu_orig();
	void on_accel_open();
	void on_accel_exit();
	void on_accel_paths();
	void on_accel_import_orig();
	void on_toolbar_open();
	void on_toolbar_full_paths();
	void open();
	void open_file(wchar_t const* const file_path);
	void refresh(main_type&& mo);
	void refresh_view_recursive(file_info& parent_fi, HTREEITEM const& parent_ti);
	void full_paths();
	void import_select_original_instance();
	int get_import_type_column_max_width();
	int get_ordinal_column_max_width();
	std::pair<file_info const*, POINT> get_file_info_under_cursor();
	void add_idle_task(idle_task_t const task, idle_task_param_t const param);
	void on_idle();
	void process_command_line();
	void request_symbol_traslation(file_info& fi);
	void request_cancellation_of_all_dbg_tasks();
	void process_finished_dbg_task(get_symbols_from_addresses_task_t* const task);
private:
	static ATOM g_class;
	static HACCEL g_accel;
private:
	HWND m_hwnd;
	HWND m_toolbar;
	splitter_window_hor m_splitter_hor;
	tree_view m_tree_view;
	splitter_window_ver m_splitter_ver;
	HWND m_import_list;
	export_view m_export_view;
	smart_menu m_import_menu;
	std::queue<std::pair<idle_task_t, idle_task_param_t>> m_idle_tasks;
	std::deque<get_symbols_from_addresses_task_t*> m_symbol_tasks;
private:
	std::array<std::wstring, 4> m_import_tmp_strings;
	std::array<std::wstring, 4> m_export_tmp_strings;
	unsigned m_import_tmp_string_idx;
	unsigned m_export_tmp_string_idx;
private:
	main_type m_mo;
	settings m_settings;
private:
	friend class tree_view;
	friend class export_view;
};
