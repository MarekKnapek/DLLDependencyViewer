#pragma once


#include <cstdint>

#include "../nogui/windows_my.h"


struct file_info;


class tree_window
{
public:
	enum class wm : std::uint32_t
	{
		wm_repaint = WM_USER + 1,
		wm_setfi,
		wm_getselection,
		wm_selectitem,
		wm_setfullpaths,
		wm_setonitemchanged,
		wm_setcmdmatching,
	};
	using onitemchanged_ctx_t = void*;
	using onitemchanged_fn_t = void(*)(onitemchanged_ctx_t const, file_info const* const&);
	using cmd_matching_ctx_t = void*;
	using cmd_matching_fn_t = void(*)(cmd_matching_ctx_t const, file_info const* const&);
public:
	tree_window() noexcept;
	tree_window(HWND const& parent);
	tree_window(tree_window const&) = delete;
	tree_window(tree_window&& other) noexcept;
	tree_window& operator=(tree_window const&) = delete;
	tree_window& operator=(tree_window&& other) noexcept;
	~tree_window();
	void swap(tree_window& other) noexcept;
public:
	static void init();
	static void deinit();
public:
	void repaint();
	void setfi(file_info* const& fi);
	file_info const* getselection();
	void selectitem(file_info const* const& fi);
	void setfullpaths(bool const& fullpaths);
	void setonitemchanged(onitemchanged_fn_t const& onitemchanged_fn, onitemchanged_ctx_t const& onitemchanged_ctx);
	void setcmdmatching(cmd_matching_fn_t const& cmd_matching_fn, cmd_matching_ctx_t const& cmd_matching_ctx);
public:
	HWND const& get_hwnd() const;
private:
	HWND m_hwnd;
};

inline void swap(tree_window& a, tree_window& b) noexcept { a.swap(b); }
