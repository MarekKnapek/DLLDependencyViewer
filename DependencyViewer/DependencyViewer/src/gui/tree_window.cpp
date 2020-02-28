#include "tree_window.h"

#include "main.h"
#include "tree_window_impl.h"

#include "../nogui/cassert_my.h"

#include <utility>


tree_window::tree_window() noexcept :
	m_hwnd()
{
}

tree_window::tree_window(HWND const& parent) :
	tree_window()
{
	assert(parent != nullptr);
	m_hwnd = CreateWindowExW(0, tree_window_impl::get_class_atom(), nullptr, WS_CLIPCHILDREN | WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, parent, nullptr, get_instance(), nullptr);
	assert(m_hwnd != nullptr);
}

tree_window::tree_window(tree_window&& other) noexcept :
	tree_window()
{
	swap(other);
}

tree_window& tree_window::operator=(tree_window&& other) noexcept
{
	swap(other);
	return *this;
}

tree_window::~tree_window()
{
}

void tree_window::swap(tree_window& other) noexcept
{
	using std::swap;
	swap(m_hwnd, other.m_hwnd);
}

void tree_window::init()
{
	tree_window_impl::init();
}

void tree_window::deinit()
{
	tree_window_impl::deinit();
}

void tree_window::repaint()
{
	UINT const msg = static_cast<std::uint32_t>(wm::wm_repaint);
	WPARAM const wparam = 0;
	LPARAM const lparam = 0;
	assert(m_hwnd != nullptr);
	[[maybe_unused]] LRESULT res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

bool tree_window::translateaccelerator(MSG& message)
{
	bool translated;
	UINT const msg = static_cast<std::uint32_t>(wm::wm_translateaccelerator);
	WPARAM const wparam = reinterpret_cast<WPARAM>(&translated);
	LPARAM const lparam = reinterpret_cast<LPARAM>(&message);
	assert(m_hwnd != nullptr);
	[[maybe_unused]] LRESULT res = SendMessageW(m_hwnd, msg, wparam, lparam);
	return translated;
}

void tree_window::setfi(file_info* const& fi)
{
	UINT const msg = static_cast<std::uint32_t>(wm::wm_setfi);
	WPARAM const wparam = 0;
	LPARAM const lparam = reinterpret_cast<LPARAM>(fi);
	assert(m_hwnd != nullptr);
	[[maybe_unused]] LRESULT res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

file_info const* tree_window::getselection()
{
	file_info const* fi;
	UINT const msg = static_cast<std::uint32_t>(wm::wm_getselection);
	WPARAM const wparam = 0;
	LPARAM const lparam = reinterpret_cast<LPARAM>(&fi);
	assert(m_hwnd != nullptr);
	[[maybe_unused]] LRESULT res = SendMessageW(m_hwnd, msg, wparam, lparam);
	return fi;
}

void tree_window::selectitem(file_info const* const& fi)
{
	UINT const msg = static_cast<std::uint32_t>(wm::wm_selectitem);
	WPARAM const wparam = 0;
	LPARAM const lparam = reinterpret_cast<LPARAM>(fi);
	assert(m_hwnd != nullptr);
	[[maybe_unused]] LRESULT res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

bool tree_window::iscmdpropertiesavail()
{
	bool avail;
	UINT const msg = static_cast<std::uint32_t>(wm::wm_iscmdpropertiesavail);
	WPARAM const wparam = 0;
	LPARAM const lparam = reinterpret_cast<LPARAM>(&avail);
	assert(m_hwnd != nullptr);
	[[maybe_unused]] LRESULT res = SendMessageW(m_hwnd, msg, wparam, lparam);
	return avail;
}

void tree_window::setfullpaths(bool const& fullpaths)
{
	UINT const msg = static_cast<std::uint32_t>(wm::wm_setfullpaths);
	WPARAM const wparam = fullpaths ? 1 : 0;
	LPARAM const lparam = 0;
	assert(m_hwnd != nullptr);
	[[maybe_unused]] LRESULT res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

void tree_window::setonitemchanged(onitemchanged_fn_t const& onitemchanged_fn, onitemchanged_ctx_t const& onitemchanged_ctx)
{
	static_assert(sizeof(onitemchanged_fn) == sizeof(WPARAM), "");
	static_assert(sizeof(onitemchanged_ctx) == sizeof(LPARAM), "");
	UINT const msg = static_cast<std::uint32_t>(wm::wm_setonitemchanged);
	WPARAM const wparam = reinterpret_cast<WPARAM>(onitemchanged_fn);
	LPARAM const lparam = reinterpret_cast<LPARAM>(onitemchanged_ctx);
	assert(m_hwnd != nullptr);
	LRESULT const res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

void tree_window::setcmdmatching(cmd_matching_fn_t const& cmd_matching_fn, cmd_matching_ctx_t const& cmd_matching_ctx)
{
	static_assert(sizeof(cmd_matching_fn) == sizeof(WPARAM), "");
	static_assert(sizeof(cmd_matching_ctx) == sizeof(LPARAM), "");
	UINT const msg = static_cast<std::uint32_t>(wm::wm_setcmdmatching);
	WPARAM const wparam = reinterpret_cast<WPARAM>(cmd_matching_fn);
	LPARAM const lparam = reinterpret_cast<LPARAM>(cmd_matching_ctx);
	assert(m_hwnd != nullptr);
	LRESULT const res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

HWND const& tree_window::get_hwnd() const
{
	return m_hwnd;
}
