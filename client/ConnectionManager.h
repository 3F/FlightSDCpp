/*
 * Copyright (C) 2001-2011 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_CONNECTION_MANAGER_H
#define DCPLUSPLUS_DCPP_CONNECTION_MANAGER_H

#include "TimerManager.h"

#include "UserConnection.h"
#include "Singleton.h"
#include "Util.h"
#include "ConnectionManagerListener.h"
#include "HintedUser.h"

namespace dcpp
{

class SocketException;

class ConnectionQueueItem
#ifdef _DEBUG
    : boost::noncopyable
#endif
{
    public:
        typedef ConnectionQueueItem* Ptr;
        typedef vector<Ptr> List;
        typedef List::const_iterator Iter;
        
        enum State
        {
            CONNECTING,                 // Recently sent request to connect
            WAITING,                    // Waiting to send request to connect
            NO_DOWNLOAD_SLOTS,          // Not needed right now
            ACTIVE                      // In one up/downmanager
        };
        
        ConnectionQueueItem(const HintedUser& aUser, bool aDownload) : token(Util::toString(Util::rand())),
            lastAttempt(0), errors(0), state(WAITING), download(aDownload), user(aUser) { }
            
        GETSET(string, token, Token);
        GETSET(uint64_t, lastAttempt, LastAttempt);
        GETSET(int, errors, Errors); // Number of connection errors, or -1 after a protocol error
        GETSET(State, state, State);
        GETSET(bool, download, Download);
        
        const HintedUser& getUser() const
        {
            return user;
        }
        
    private:
        HintedUser user;
};

class ExpectedMap
{
    public:
        void add(const string& aNick, const string& aMyNick, const string& aHubUrl)
        {
            Lock l(cs);
            expectedConnections.insert(make_pair(aNick, make_pair(aMyNick, aHubUrl)));
        }
        
        StringPair remove(const string& aNick)
        {
            Lock l(cs);
            ExpectMap::iterator i = expectedConnections.find(aNick);
            
            if (i == expectedConnections.end())
                return make_pair(Util::emptyString, Util::emptyString);
                
            StringPair tmp = i->second;
            expectedConnections.erase(i);
            
            return tmp;
        }
        
    private:
        /** Nick -> myNick, hubUrl for expected NMDC incoming connections */
        typedef map<string, StringPair> ExpectMap;
        ExpectMap expectedConnections;
        
        CriticalSection cs;
};

// Comparing with a user...
inline bool operator==(ConnectionQueueItem::Ptr ptr, const UserPtr& aUser)
{
    return ptr->getUser() == aUser;
}

class ConnectionManager : public Speaker<ConnectionManagerListener>,
    public UserConnectionListener, TimerManagerListener,
    public Singleton<ConnectionManager>
{
    public:
        void nmdcExpect(const string& aNick, const string& aMyNick, const string& aHubUrl)
        {
            expectedConnections.add(aNick, aMyNick, aHubUrl);
        }
        
        void nmdcConnect(const string& aServer, uint16_t aPort, const string& aMyNick, const string& hubUrl, const string& encoding, bool stealth, bool secure);
        void nmdcConnect(const string& aServer, uint16_t aPort, uint16_t localPort, BufferedSocket::NatRoles natRole, const string& aNick, const string& hubUrl, const string& encoding, bool stealth, bool secure);
        void adcConnect(const OnlineUser& aUser, uint16_t aPort, const string& aToken, bool secure);
        void adcConnect(const OnlineUser& aUser, uint16_t aPort, uint16_t localPort, BufferedSocket::NatRoles natRole, const string& aToken, bool secure);
        
        void getDownloadConnection(const HintedUser& aUser);
        void force(const UserPtr& aUser);
        
        void disconnect(const UserPtr& aUser); // disconnect downloads and uploads
        void disconnect(const UserPtr& aUser, int isDownload);
        
        void shutdown();
        bool isShuttingDown() const
        {
            return shuttingDown;
        }
        
        /** Find a suitable port to listen on, and start doing it */
        void listen();
        void disconnect() noexcept;
        
        uint16_t getPort() const
        {
            return server ? static_cast<uint16_t>(server->getPort()) : 0;
        }
        uint16_t getSecurePort() const
        {
            return secureServer ? static_cast<uint16_t>(secureServer->getPort()) : 0;
        }
        static uint16_t iConnToMeCount;
    private:
    
        class Server : public Thread
        {
            public:
                Server(bool secure_, uint16_t port, const string& ip = "0.0.0.0");
                uint16_t getPort() const
                {
                    return port;
                }
                ~Server()
                {
                    die = true;
                    join();
                }
            private:
                int run() noexcept;
                
                Socket sock;
                uint16_t port;
                string ip;
                bool secure;
                bool die;
        };
        
        friend class Server;
        
        CriticalSection cs;
        
        /** All ConnectionQueueItems */
        ConnectionQueueItem::List downloads;
        ConnectionQueueItem::List uploads;
        
        /** All active connections */
        UserConnectionList userConnections;
        
        StringList features;
        StringList adcFeatures;
        
        ExpectedMap expectedConnections;
        
        uint64_t floodCounter;
        
        Server* server;
        Server* secureServer;
        
        bool shuttingDown;
        
        friend class Singleton<ConnectionManager>;
        ConnectionManager();
        
        ~ConnectionManager()
        {
            shutdown();
        }
        
        void store_last_ip(UserConnection* p_uc, ConnectionQueueItem* p_qi); // [+]PPA
        UserConnection* getConnection(bool aNmdc, bool secure) noexcept;
        void putConnection(UserConnection* aConn);
        
        void addUploadConnection(UserConnection* uc);
        void addDownloadConnection(UserConnection* uc);
        
        ConnectionQueueItem* getCQI(const HintedUser& aUser, bool download);
        void putCQI(ConnectionQueueItem* cqi);
        
        void accept(const Socket& sock, bool secure) noexcept;
        
        bool checkKeyprint(UserConnection *aSource);
        
        void failed(UserConnection* aSource, const string& aError, bool protocolError);
        
        bool checkIpFlood(const string& aServer, uint16_t aPort, const string& userInfo);
        
        // UserConnectionListener
        void on(Connected, UserConnection*) noexcept;
        void on(Failed, UserConnection*, const string&) noexcept;
        void on(ProtocolError, UserConnection*, const string&) noexcept;
        void on(CLock, UserConnection*, const string&, const string&) noexcept;
        void on(Key, UserConnection*, const string&) noexcept;
        void on(Direction, UserConnection*, const string&, const string&) noexcept;
        void on(MyNick, UserConnection*, const string&) noexcept;
        void on(Supports, UserConnection*, const StringList&) noexcept;
        
        void on(AdcCommand::SUP, UserConnection*, const AdcCommand&) noexcept;
        void on(AdcCommand::INF, UserConnection*, const AdcCommand&) noexcept;
        void on(AdcCommand::STA, UserConnection*, const AdcCommand&) noexcept;
        
        // TimerManagerListener
        void on(TimerManagerListener::Second, uint64_t aTick) noexcept;
        void on(TimerManagerListener::Minute, uint64_t aTick) noexcept;
        
};

} // namespace dcpp

#endif // !defined(CONNECTION_MANAGER_H)

/**
 * @file
 * $Id: ConnectionManager.h 568 2011-07-24 18:28:43Z bigmuscle $
 */
