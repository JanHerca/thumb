//  Copyright (C) 2007 Robert Kooima
//
//  THUMB is free software; you can redistribute it and/or modify it under
//  the terms of  the GNU General Public License as  published by the Free
//  Software  Foundation;  either version 2  of the  License,  or (at your
//  option) any later version.
//
//  This program  is distributed in the  hope that it will  be useful, but
//  WITHOUT   ANY  WARRANTY;   without  even   the  implied   warranty  of
//  MERCHANTABILITY  or FITNESS  FOR A  PARTICULAR PURPOSE.   See  the GNU
//  General Public License for more details.

#ifndef APP_MESSAGE_H
#define APP_MESSAGE_H

#include <string>
#include <cstring>
#include <stdexcept>
#include <errno.h>

#include "socket.hpp"

//-----------------------------------------------------------------------------

#define DATAMAX 256

#define E_REPLY 0
#define E_POINT 1
#define E_CLICK 2
#define E_KEYBD 3
#define E_VALUE 4
#define E_INPUT 5
#define E_TIMER 6
#define E_PAINT 7
#define E_FRAME 8
#define E_START 9
#define E_CLOSE 10

namespace app
{
    //-------------------------------------------------------------------------

    class host_error : public std::runtime_error
    {
        std::string mesg(const char *s) {
/* W32 HACK
            return std::string(s) + ": " + hstrerror(h_errno);
*/
            return std::string(s) + ": " + strerror(errno);
        }

    public:
        host_error(const char *s) : std::runtime_error(mesg(s)) { }
    };

    class sock_error : public std::runtime_error
    {
        std::string mesg(const char *s) {
            return std::string(s) + ": " + strerror(errno);
        }

    public:
        sock_error(const char *s) : std::runtime_error(mesg(s)) { }
    };

    //-------------------------------------------------------------------------

    class message
    {
        union swap { double d; int i; uint32_t l[2]; };

        struct { unsigned char type;
                 unsigned char size;
                 char          data[DATAMAX]; } payload;

        int index;

        const char *tag() const;

    public:

        message();

        // Data marshalling

        void   put_text(const char *);
        void   put_real(double);
        void   put_bool(bool);
        void   put_byte(int);
        void   put_word(int);

        const char *get_text();
        double      get_real();
        bool        get_bool();
        int         get_byte();
        int         get_word();

        // Network IO

        void send(SOCKET);
        void recv(SOCKET);

        unsigned char type() const { return payload.type; }
    };
}

//-----------------------------------------------------------------------------

#endif
