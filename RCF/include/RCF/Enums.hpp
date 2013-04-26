
//******************************************************************************
// RCF - Remote Call Framework
//
// Copyright (c) 2005 - 2013, Delta V Software. All rights reserved.
// http://www.deltavsoft.com
//
// RCF is distributed under dual licenses - closed source or GPL.
// Consult your particular license for conditions of use.
//
// If you have not purchased a commercial license, you are using RCF 
// under GPL terms.
//
// Version: 2.0
// Contact: support <at> deltavsoft.com 
//
//******************************************************************************

#ifndef INCLUDE_RCF_ENUMS_HPP
#define INCLUDE_RCF_ENUMS_HPP

namespace RCF {

    /// Describes the transport types used by a RCF connection.
    enum TransportType
    {
        /// Unknown
        Tt_Unknown,

        /// TCP transport
        Tt_Tcp,

        /// UDP transport
        Tt_Udp,

        /// Win32 named pipe transport
        Tt_Win32NamedPipe,

        /// UNIX local domain socket transport
        Tt_UnixNamedPipe,

        /// HTTP/TCP transport
        Tt_Http,

        /// HTTPS/TCP transport
        Tt_Https,

        /// In-process transport
        Tt_InProcess
    };

    /// Describes the transport protocols used by a RCF connection. Transport
    /// protocols are layered on top of the transport type.
    enum TransportProtocol
    {
        /// Unspecified
        Tp_Unspecified,

        /// Clear text
        Tp_Clear,

        /// Windows NTLM
        Tp_Ntlm,

        /// Windows Kerberos
        Tp_Kerberos,

        /// Windows Negotiate (Kerberos or NTLM)
        Tp_Negotiate,

        /// SSL
        Tp_Ssl
    };







    enum RemoteCallSemantics
    {
        Oneway,
        Twoway
    };

    enum WireProtocol
    {
        Wp_None,
        Wp_Http,
        Wp_Https
    };



} // namespace RCF

#endif // ! INCLUDE_RCF_ENUMS_HPP
