/*
  * Copyright (c) 2013 Developed by reg <entry.reg@gmail.com>
  * 
  * Distributed under the Boost Software License, Version 1.0
  * (see accompanying file LICENSE or a copy at http://www.boost.org/LICENSE_1_0.txt)
 */

// see detail in commit's on -> bitbucket.org/3F

#pragma once
#include "typedefs.h" //or use from sandbox

using namespace dcpp;

namespace reg { namespace text {

    class Wildcards
    {
    public:

        enum MetaOperation{
            FLUSH   = 0,
            ANY     = 1,
            ANYSP   = 2,
            SPLIT   = 4,
            ONE     = 8,
            START   = 16,
            END     = 32,
            EOL     = 64,
        };

        enum MetaSymbols{
            MS_ANY      = _T('*'),
            MS_ANYSP    = _T('>'), //as [^/\\]+
            MS_SPLIT    = _T('|'),
            MS_ONE      = _T('?'),
            MS_START    = _T('^'),
            MS_END      = _T('$'),
        };

        //static bool match(const tstring& text, const tstring& filter);

        //[+] :(
        static bool Wildcards::match(const wstring& text, const wstring& filter);

        //[+] :(
        static bool match(const string& text, const string& filter);

        /** main support */
        static void validate();

    protected:

        // :( this project uses a variety of string/wstring without unified typedef
        template<class tstring>
        static bool _match(const tstring& text, const tstring& filter);

        struct Mask{
            MetaOperation curr;
            MetaOperation prev;
            Mask(): curr(FLUSH), prev(FLUSH){};
        };

        /**
         * to wildcards
         */
        template<class tstring>
        struct Item{
            tstring curr;
            size_t pos;
            size_t left;
            size_t delta;
            Mask mask;
            tstring prev;
            Item(): pos(0), left(0), delta(0){};
        };

        /**
         * to words
         */
        struct Words{
            size_t found;
            size_t left;
            Words(): left(0){};
        };

        /**
         * Working with an interval:
         *      _______
         * {word} ... {word}
         */
        template<class tstring>
        static size_t _handlerInterval(Item<tstring>& item, Words& words, const tstring& text);

        #ifdef _DEBUG
            /** verify MetaOperation::ANY */
            static void _assertsAny();
            /** verify MetaOperation::SPLIT */
            static void _assertsSplit();
            /** verify MetaOperation::ONE */
            static void _assertsOne();
            /** verify MetaOperation::ANYSP */
            static void _assertsAnySP();
        #endif

        template<class tstring>
        inline static tstring _uppercase(tstring str) throw()
        {
            transform(str.begin(), str.end(), str.begin(), towupper);
            return str;
        };

        //[+]
        inline const static tstring _slashs(const tstring&) throw() { return _T("\\/"); }
        //[+]
        inline const static string _slashs(const string&) throw() { return "\\/"; }

        Wildcards(void){};
        ~Wildcards(void){};
    };

}};