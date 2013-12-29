/*
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(ABOUT_DLG_H)
#define ABOUT_DLG_H

#pragma once

#include <atlctrlx.h>

static const TCHAR g_thanks[] =
    _T("http://www.flockline.ru (logo)\r\n")
    _T("http://code.google.com/p/flylinkdc/people/list (FlylinkDC++ Team)\r\n")
    _T("reg <entry.reg@gmail.com>\r\n")

    ;
class AboutDlg : public CDialogImpl<AboutDlg>
{
    public:
        enum { IDD = IDD_ABOUTBOX };
        
        AboutDlg() { }
        virtual ~AboutDlg() { }
        
        BEGIN_MSG_MAP(AboutDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
        COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
        COMMAND_ID_HANDLER(IDC_LINK_BLOG, onBlogLink)
        END_MSG_MAP()
        
        LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
        {
            SetDlgItemText(IDC_VERSION,
                           _T("*!is not public release!\r\ndo not use this client is not for debugging or testing\r\n") T_VERSIONSTRING
                           _T("\r\n\r\nBased on: StrongDC++ sqlite (FlylinkDC++ Team)"));
            CEdit ctrlThanks(GetDlgItem(IDC_THANKS));
            ctrlThanks.FmtLines(TRUE);
            ctrlThanks.AppendText(g_thanks, TRUE);
            ctrlThanks.Detach();
#ifdef FLY_INCLUDE_EXE_TTH
            SetDlgItemText(IDC_TTH, WinUtil::tth.c_str());
#endif
            SetDlgItemText(IDC_TOTALS, (_T("Upload: ") + Util::formatBytesW(CFlylinkDBManager::getInstance()->m_global_ratio.m_upload) + _T(", Download: ") +
                                        Util::formatBytesW(CFlylinkDBManager::getInstance()->m_global_ratio.m_download)).c_str());
                                        
            SetDlgItemText(IDC_LINK_BLOG, _T("http://flylinkdc.blogspot.com"));
            m_url_blog.SubclassWindow(GetDlgItem(IDC_LINK_BLOG));
            m_url_blog.SetHyperLinkExtendedStyle(HLINK_COMMANDBUTTON | HLINK_UNDERLINEHOVER);
            
            if (SETTING(TOTAL_DOWNLOAD) > 0)
            {
                TCHAR buf[64];
                buf[0] = 0;
                snwprintf(buf, _countof(buf), _T("Ratio (up/down): %.2f"), CFlylinkDBManager::getInstance()->get_ratio());
                
                SetDlgItemText(IDC_RATIO, buf);
                /*  sprintf(buf, "Uptime: %s", Util::formatTime(Util::getUptime()));
                    SetDlgItemText(IDC_UPTIME, Text::toT(buf).c_str());*/
            }
            CenterWindow(GetParent());
            return TRUE;
        }
        
        LRESULT onVersionData(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
        {
            tstring* x = (tstring*) wParam;
            SetDlgItemText(IDC_LATEST, x->c_str());
            delete x;
            return 0;
        }
        LRESULT onBlogLink(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
        {
            WinUtil::openLink(_T("http://flylinkdc.blogspot.com"));
            return 0;
        }
        
        LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
        {
            EndDialog(wID);
            return 0;
        }
        
    private:
        CHyperLink m_url_blog;
        AboutDlg(const AboutDlg&)
        {
            dcassert(0);
        }
        
};

#endif // !defined(ABOUT_DLG_H)

/**
 * @file
 * $Id: AboutDlg.h 568 2011-07-24 18:28:43Z bigmuscle $
 */
