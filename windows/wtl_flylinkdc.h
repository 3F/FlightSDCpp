/*
 * Copyright (C) 2012 FlylinkDC++ Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef DCPLUSPLUS_WTL_FLYLINKDC_H
#define DCPLUSPLUS_WTL_FLYLINKDC_H

#include <atlctrlx.h>
#include "../client/SettingsManager.h"
#include "../client/ResourceManager.h"

class CFlyTimerAdapter
{
		UINT_PTR m_timer_id;
		const HWND& m_hTimerWnd;
	public:
		CFlyTimerAdapter(const HWND& p_hWnd) : m_hTimerWnd(p_hWnd), m_timer_id(0)
		{
		}
		virtual ~CFlyTimerAdapter()
		{
			dcassert(!m_timer_id);
		}
	protected:
		UINT_PTR create_timer(UINT p_Elapse, UINT_PTR p_IDEvent = NULL)
		{
			ATLASSERT(::IsWindow(m_hTimerWnd));
			ATLASSERT(m_timer_id == NULL);
			if (m_timer_id == NULL) // � ����� �������� ����������� ����� SetTimer http://code.google.com/p/flylinkdc/issues/detail?id=1328
			{
				m_timer_id = SetTimer(m_hTimerWnd, p_IDEvent, p_Elapse, NULL);
			}
			return m_timer_id;
		}
		
		void safe_destroy_timer()
		{
			dcassert(m_timer_id);
			if (m_timer_id)
			{
				ATLASSERT(::IsWindow(m_hTimerWnd));
				KillTimer(m_hTimerWnd, m_timer_id);
				m_timer_id = 0;
			}
		}
#if 0 // TODO: needs review, see details here https://code.google.com/p/flylinkdc/source/detail?r=15539
		void safe_destroy_timer_if_exists()
		{
			if (m_timer_id)
				safe_destroy_timer();
		}
#endif
};

class CFlyHyperLink : public CHyperLink
#ifdef _DEBUG
	, private boost::noncopyable
#endif
{
#ifdef _DEBUG
		void Attach(_In_opt_ HWND hWndNew) noexcept // �������� ��� ������ ����� Attach - ��������.
{
		}
#endif
	public:
		void init(HWND p_dlg_window, const tstring& p_tool)
		{
			SubclassWindow(p_dlg_window);
			SetHyperLinkExtendedStyle(HLINK_COMMANDBUTTON | HLINK_UNDERLINEHOVER);
			if (BOOLSETTING(WINDOWS_STYLE_URL))
				m_clrLink = GetSysColor(COLOR_HOTLIGHT); // [~] JhaoDa
			ATLVERIFY(m_tip.AddTool(m_hWnd, p_tool.c_str(), &m_rcLink, 1));
		}
};

class CFlyToolTipCtrl : public CToolTipCtrl
#ifdef _DEBUG
	, private boost::noncopyable
#endif
{
	public:
		CFlyToolTipCtrl()
		{
		}
		void AddTool(HWND hWnd, const wchar_t* p_Text = LPSTR_TEXTCALLBACK)
		{
			CToolInfo l_ti(TTF_SUBCLASS, hWnd, 0, nullptr, (LPWSTR) p_Text);
			ATLVERIFY(CToolTipCtrl::AddTool(&l_ti));
		}
		void AddTool(HWND p_hWnd, ResourceManager::Strings p_ID)
		{
			AddTool(p_hWnd, ResourceManager::getStringW(p_ID).c_str());
		}
};

template<bool needsInvalidate = false>
class CLockRedraw
#ifdef _DEBUG
	: private boost::noncopyable
#endif
{
	public:
	CLockRedraw(const HWND p_hWnd) noexcept :
		m_hWnd(p_hWnd)
		{
			ATLASSERT(::IsWindow(m_hWnd));
			::SendMessage(m_hWnd, WM_SETREDRAW, (WPARAM)FALSE, 0);
		}
		~CLockRedraw() noexcept
		{
			ATLASSERT(::IsWindow(m_hWnd));
			::SendMessage(m_hWnd, WM_SETREDRAW, (WPARAM)TRUE, 0);
			if (needsInvalidate)
			{
				::InvalidateRect(m_hWnd, nullptr, TRUE);
			}
		}
	private:
		const HWND m_hWnd;
};
#if 0
class CFlyTimer // TODO ���� �� ������������
#ifdef _DEBUG
	: private boost::noncopyable
#endif
{
		TIMERPROC m_f;
		UINT_PTR m_Timer;
		int m_interval;
	public:
		CFlyTimer(TIMERPROC p_f, int p_interval) : m_f(p_f), m_interval(p_interval), m_Timer(NULL)
		{
		}
		void start()
		{
			dcassert(m_Timer == NULL);
			m_Timer = SetTimer(nullptr, 0, m_interval, m_f);
		}
		void stop()
		{
			if (m_Timer)
			{
				KillTimer(nullptr, m_Timer);
				m_Timer = NULL;
			}
		}
		virtual ~CFlyTimer()
		{
			stop();
		}
};
#endif

// copy-paste from wtl\atlwinmisc.h
// (����� ����� �������������� ������� warning C4245: 'argument' : conversion from 'int' to 'UINT_PTR', signed/unsigned mismatch )
class CFlyLockWindowUpdate
#ifdef _DEBUG
	: private boost::noncopyable
#endif
{
	public:
		CFlyLockWindowUpdate(HWND hWnd)
		{
			// NOTE: A locked window cannot be moved.
			//       See also Q270624 for problems with layered windows.
			ATLASSERT(::IsWindow(hWnd));
			::LockWindowUpdate(hWnd);
		}
		
		~CFlyLockWindowUpdate()
		{
			::LockWindowUpdate(NULL);
		}
};

#define ATTACH(p_id, p_var) \
	p_var.Attach(GetDlgItem(p_id));

#define ATTACH_AND_SET_TEXT(p_id, p_var, p_txt) \
	{ \
		p_var.Attach(GetDlgItem(p_id)); \
		p_var.SetWindowText(p_txt.c_str()); \
	}

#define GET_TEXT(id, var) \
	{ \
		var.resize(::GetWindowTextLength(GetDlgItem(id))); \
		if (var.size()) GetDlgItemText(id, &var[0], var.size() + 1); \
	}

#endif // DCPLUSPLUS_WTL_FLYLINKDC_H
