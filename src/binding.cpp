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

#include "binding.hpp"
#include "glob.hpp"

//-----------------------------------------------------------------------------

ogl::binding::binding(std::string name) : name(name)
{
    color = glob->load_program("object-color.vert", "object-color.frag");
    depth = glob->load_program("object-depth.vert", "object-depth.frag");

    diff = glob->load_texture("solid/metal_box_diffuse.png");
    bump = glob->load_texture("solid/metal_box_bump.png");

    init();
}

ogl::binding::~binding()
{
    glob->free_texture(bump);
    glob->free_texture(diff);

    glob->free_program(depth);
    glob->free_program(color);
}

//-----------------------------------------------------------------------------

bool ogl::binding::depth_eq(const binding *that) const
{
    return (depth == that->depth);
}

bool ogl::binding::color_eq(const binding *that) const
{
    return (color == that->color);
}

//-----------------------------------------------------------------------------

void ogl::binding::bind(bool c) const
{
    if (c)
    {
        color->bind();

        diff->bind(GL_TEXTURE0);
        bump->bind(GL_TEXTURE1);
    }
    else
    {
        depth->bind();

        diff->bind(GL_TEXTURE0);
    }
}

//-----------------------------------------------------------------------------

void ogl::binding::init()
{
    color->bind();
    {
        color->uniform("diffuse", 0);
        color->uniform("bump",    1);
        color->uniform("light",   2);
        color->uniform("shadow",  3);
    }
    color->free();

    depth->bind();
    {
        depth->uniform("diffuse", 0);
    }
    depth->free();
}

void ogl::binding::fini()
{
}

//-----------------------------------------------------------------------------
