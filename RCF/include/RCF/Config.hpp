
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

#ifndef INCLUDE_RCF_CONFIG_HPP
#define INCLUDE_RCF_CONFIG_HPP

#include <boost/config.hpp>
#include <boost/mpl/int.hpp>

#ifndef RCF_MAX_METHOD_COUNT
#define RCF_MAX_METHOD_COUNT 100
#endif

#if !defined(RCF_USE_SF_SERIALIZATION) && !defined(RCF_USE_BOOST_SERIALIZATION) && !defined(RCF_USE_BOOST_XML_SERIALIZATION)
#define RCF_USE_SF_SERIALIZATION
#endif

// Detect TR1 availability.
#ifndef RCF_USE_TR1

    // MSVC
    #if defined(_MSC_VER) && (_MSC_VER >= 1600 || (_MSC_VER == 1500 && _MSC_FULL_VER >= 150030729))
    #define RCF_USE_TR1
    #define RCF_TR1_HEADER(x) <x>
    #endif

    // GCC
    #if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3))
    #define RCF_USE_TR1
    #define RCF_TR1_HEADER(x) <tr1/x>
    #endif

#endif // RCF_USE_TR1

// Detect hash_map/hash_set availability.
#ifndef RCF_USE_HASH_MAP

    #if (defined(_MSC_VER) && _MSC_VER >= 1310) || (defined(__GNUC__) && __GNUC__ == 3)
        #define RCF_USE_HASH_MAP
        #if defined(_MSC_VER)
            #define RCF_HASH_MAP_HEADER(x) <x>
            #define RCF_HASH_MAP_NS stdext
        #elif defined(__GNUC__)
            #define RCF_HASH_MAP_HEADER(x) <ext/x>
            #define RCF_HASH_MAP_NS __gnu_cxx
        #endif
    #endif

#endif // RCF_USE_HASH_MAP

#ifdef RCF_USE_BOOST_THREADS
#error RCF_USE_BOOST_THREADS is no longer supported. RCF now uses an internal threading library.
#endif

// Need to use external Boost.Asio when building on Cygwin.
#if defined(__CYGWIN__) && !defined(RCF_USE_BOOST_ASIO)
#define RCF_USE_BOOST_ASIO
#endif

namespace RCF {

    #ifndef RCF_USE_SF_SERIALIZATION
    typedef boost::mpl::int_<0> RcfConfig_SF;
    #else
    typedef boost::mpl::int_<1> RcfConfig_SF;
    #endif

    #ifndef RCF_USE_BOOST_SERIALIZATION
    typedef boost::mpl::int_<0> RcfConfig_BSer;
    #else
    typedef boost::mpl::int_<1> RcfConfig_BSer;
    #endif

    #ifndef RCF_USE_BOOST_FILESYSTEM
    typedef boost::mpl::int_<0> RcfConfig_BFs;
    #else
    typedef boost::mpl::int_<1> RcfConfig_BFs;
    #endif

    #ifndef RCF_USE_JSON
    typedef boost::mpl::int_<0> RcfConfig_Json;
    #else
    typedef boost::mpl::int_<1> RcfConfig_Json;
    #endif

    template<int N1, int N2, int N3, int N4>
    struct RcfConfigurationMismatchDetected
    {
    };

    typedef RcfConfigurationMismatchDetected<
        RcfConfig_SF::value, 
        RcfConfig_BSer::value, 
        RcfConfig_BFs::value, 
        RcfConfig_Json::value> RcfConfigT;

} // namespace RCF

#endif // ! INCLUDE_RCF_CONFIG_HPP
