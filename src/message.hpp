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

#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <stdexcept>
#include <errno.h>

#include "socket.hpp"

//-----------------------------------------------------------------------------

#define E_REPLY 0
#define E_POINT 1
#define E_CLICK 2
#define E_KEYBD 3
#define E_TIMER 4
#define E_TRACK 5
#define E_STICK 6
#define E_PAINT 7
#define E_FLEEP 8
#define E_CLOSE 9

namespace app
{
    //-------------------------------------------------------------------------

    class host_error : public std::runtime_error
    {
        std::string mesg(const char *s) {
            return std::string(s) + ": " + hstrerror(h_errno);
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
                 unsigned char data[256]; } payload;

        int index;

        const char *tag() const;

    public:

        message(unsigned char);

        // Data marshalling

        void   put_double(double);
        void   put_bool  (bool);
        void   put_int   (int);

        double get_double();
        bool   get_bool  ();
        int    get_int   ();

        // Network IO

        void send(SOCKET);
        void recv(SOCKET);

        unsigned char type() const { return payload.type; }
    };
}

//-----------------------------------------------------------------------------

#endif
