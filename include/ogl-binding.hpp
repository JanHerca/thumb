//  Copyright (C) 2007-2011 Robert Kooima
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

#ifndef OGL_BINDING_HPP
#define OGL_BINDING_HPP

#include <vector>
#include <string>
#include <map>

#include <app-file.hpp>

//-----------------------------------------------------------------------------

namespace ogl
{
    class pool;
    class node;
    class frame;
    class program;
    class texture;
}

//-----------------------------------------------------------------------------

namespace ogl
{
    class binding
    {
        typedef std::map<GLenum, const ogl::texture *> unit_texture;

        // Local binding attributes.

        std::string name;

        const ogl::program *depth_program;  // Depth mode shader program
        unit_texture        depth_texture;  // Depth mode texture bindings

        const ogl::program *color_program;  // Color mode shader program
        unit_texture        color_texture;  // Color mode texture bindings

        const ogl::program *init_program(app::node, unit_texture&);

    public:

        // Local binding methods.

        const std::string& get_name() const { return name; }

        binding(std::string);
       ~binding();

        bool depth_eq(const binding *) const;
        bool color_eq(const binding *) const;
        bool opaque() const;

        bool bind(bool) const;

        const ogl::texture *get_default_texture() const;
    };
}

//-----------------------------------------------------------------------------

#endif
