/*
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_SPEAKER_H
#define DCPLUSPLUS_DCPP_SPEAKER_H

#include <boost/range/algorithm/find.hpp>
#include <utility>
#include <vector>
#include "Thread.h"
#include "noexcept.h"
template<typename Listener>
class Speaker
{
        typedef std::vector<Listener*> ListenerList;
#ifdef _DEBUG
# define  _DEBUG_SPEAKER_LISTENER_LIST_LEVEL_1 // Only critical event debug.
# ifdef _DEBUG_SPEAKER_LISTENER_LIST_LEVEL_1
//#  define  _DEBUG_SPEAKER_LISTENER_LIST_LEVEL_2 // Show all events in debug window.
# endif
#endif

#ifdef _DEBUG_SPEAKER_LISTENER_LIST_LEVEL_2
        void log_listener_list(const ListenerList& p_list, const char* p_log_message)
        {
            dcdebug("[log_listener_list][%s][tid = %u] [this=%p] count = %d ", p_log_message, GetSelfThreadID(), this, p_list.size());
            for (size_t i = 0; i != p_list.size(); ++i)
            {
                dcdebug("[%u] = %p, ", i, p_list[i]);
            }
            dcdebug("\r\n");
        }
#endif // _DEBUG_SPEAKER_LISTENER_LIST_LEVEL_2
    public:
        explicit Speaker() noexcept
        {
        }
        virtual ~Speaker()
        {
            dcassert(listeners.empty());
        }
        
        /// @todo simplify when we have variadic templates
        
        template<typename T0>
        void fire(T0 && type) noexcept
        {
            Lock l(listenerCS);
            tmp = listeners;
            for (auto i = tmp.cbegin(); i != tmp.cend(); ++i)
                (*i)->on(std::forward<T0>(type));
        }
        template<typename T0, typename T1>
        void fire(T0 && type, T1 && p1) noexcept
        {
            Lock l(listenerCS);
            tmp = listeners;
            for (auto i = tmp.cbegin(); i != tmp.cend(); ++i)
                (*i)->on(std::forward<T0>(type), std::forward<T1>(p1)); // [2] https://www.box.net/shared/da9ee6ddd7ec801b1a86
        }
        template<typename T0, typename T1, typename T2>
        void fire(T0 && type, T1 && p1, T2 && p2) noexcept
        {
            Lock l(listenerCS);
            tmp = listeners;
            for (auto i = tmp.cbegin(); i != tmp.cend(); ++i)
                (*i)->on(std::forward<T0>(type), std::forward<T1>(p1), std::forward<T2>(p2)); // Venturi Firewall 2012-04-23_22-28-18_A6JRQEPFW5263A7S7ZOBOAJGFCMET3YJCUYOVCQ_0E0D7D71_crash-stack-r501-build-9812.dmp.bz2
        }
        template<typename T0, typename T1, typename T2, typename T3>
        void fire(T0 && type, T1 && p1, T2 && p2, T3 && p3) noexcept
        {
            Lock l(listenerCS);
            tmp = listeners;
            for (auto i = tmp.cbegin(); i != tmp.cend(); ++i)
                (*i)->on(std::forward<T0>(type), std::forward<T1>(p1), std::forward<T2>(p2), std::forward<T3>(p3));
        }
        template<typename T0, typename T1, typename T2, typename T3, typename T4>
        void fire(T0 && type, T1 && p1, T2 && p2, T3 && p3, T4 && p4) noexcept
        {
            Lock l(listenerCS);
            tmp = listeners;
            for (auto i = tmp.cbegin(); i != tmp.cend(); ++i)
                (*i)->on(std::forward<T0>(type), std::forward<T1>(p1), std::forward<T2>(p2), std::forward<T3>(p3), std::forward<T4>(p4));
        }
        template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
        void fire(T0 && type, T1 && p1, T2 && p2, T3 && p3, T4 && p4, T5 && p5) noexcept
        {
            Lock l(listenerCS);
            tmp = listeners;
            for (auto i = tmp.cbegin(); i != tmp.cend(); ++i)
                (*i)->on(std::forward<T0>(type), std::forward<T1>(p1), std::forward<T2>(p2), std::forward<T3>(p3), std::forward<T4>(p4), std::forward<T5>(p5));
        }
        template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
        void fire(T0 && type, T1 && p1, T2 && p2, T3 && p3, T4 && p4, T5 && p5, T6 && p6) noexcept
        {
            Lock l(listenerCS);
            tmp = listeners;
            for (auto i = tmp.cbegin(); i != tmp.cend(); ++i)
                (*i)->on(std::forward<T0>(type), std::forward<T1>(p1), std::forward<T2>(p2), std::forward<T3>(p3), std::forward<T4>(p4), std::forward<T5>(p5), std::forward<T6>(p6));
        }
        template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
        void fire(T0 && type, T1 && p1, T2 && p2, T3 && p3, T4 && p4, T5 && p5, T6 && p6, T7 && p7) noexcept
        {
            Lock l(listenerCS);
            tmp = listeners;
            for (auto i = tmp.cbegin(); i != tmp.cend(); ++i)
                (*i)->on(std::forward<T0>(type), std::forward<T1>(p1), std::forward<T2>(p2), std::forward<T3>(p3), std::forward<T4>(p4), std::forward<T5>(p5), std::forward<T6>(p6), std::forward<T7>(p7));
        }
        
        void addListener(Listener* aListener)
        {
            Lock l(listenerCS);
            if (boost::range::find(listeners, aListener) == listeners.end())
            {
                listeners.push_back(aListener);
            }
#ifdef _DEBUG_SPEAKER_LISTENER_LIST_LEVEL_1
            else
            {
                dcassert(0);
# ifdef _DEBUG_SPEAKER_LISTENER_LIST_LEVEL_2
                log_listener_list(listeners, "addListener-twice!!!");
# endif
            }
#endif // _DEBUG_SPEAKER_LISTENER_LIST_LEVEL_1
        }
        
        void removeListener(Listener* aListener)
        {
            Lock l(listenerCS);
            auto it = boost::range::find(listeners, aListener);
            if (it != listeners.end())
            {
                listeners.erase(it);
            }
#ifdef _DEBUG_SPEAKER_LISTENER_LIST_LEVEL_1
            else
            {
                dcassert(0);
# ifdef _DEBUG_SPEAKER_LISTENER_LIST_LEVEL_2
                log_listener_list(listeners, "removeListener-zombie!!!");
# endif
            }
#endif // _DEBUG_SPEAKER_LISTENER_LIST_LEVEL_1
        }
        
        void removeListeners()
        {
            Lock l(listenerCS);
            listeners.clear();
        }
        
    private:
        ListenerList listeners;
        ListenerList tmp;
        CriticalSection listenerCS;
        
};

#endif // !defined(SPEAKER_H)

/**
 * @file
 * $Id: Speaker.h 568 2011-07-24 18:28:43Z bigmuscle $
 */
