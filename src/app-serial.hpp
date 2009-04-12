//  Copyright (C) 2005 Robert Kooima
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

#ifndef APP_SERIAL_HPP
#define APP_SERIAL_HPP

#include <string>

//-----------------------------------------------------------------------------
// Forward-declare the Mini-XML node struct to eliminate the header dependency.

struct mxml_node_s;

namespace app
{
    //-------------------------------------------------------------------------
    // Serializable node

    class node
    {
        struct mxml_node_s *ptr;

    public:

        node(const std::string&);
        node(struct mxml_node_s * = 0);

        operator bool() const { return ptr; }

        void dirty();
        void clean();

        // File IO

        void read (const std::string&);
        void write(const std::string&);

        // Attribute manipulation

        void        set_i(const std::string&, int = 0);
        int         get_i(const std::string&, int = 0);
        void        set_f(const std::string&, double = 0);
        double      get_f(const std::string&, double = 0);
        void        set_s(const std::string&, const std::string& = "");
        std::string get_s(const std::string&, const std::string& = "");

        // Iteration

        node find(const std::string& = "",
                  const std::string& = "",
                  const std::string& = "");
        node next(node,
                  const std::string& = "",
                  const std::string& = "",
                  const std::string& = "");

        // Hierarchy manipulation

        void insert(node, node);
        void remove();
    };

    //-------------------------------------------------------------------------
    // Serializable file

    class file
    {
    private:

        std::string name;
        node        head;

        void load();
        void save();

    public:

        file(const std::string&);
       ~file();

        node get_head() { return head; }
    };
}

//-----------------------------------------------------------------------------

#endif
