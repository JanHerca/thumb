//  Copyright (C) 2009 Robert Kooima
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

#include <cassert>

#include <app-conf.hpp>
#include <app-glob.hpp>
#include <ogl-frame.hpp>
#include <ogl-shadow.hpp>

//-----------------------------------------------------------------------------

ogl::shadow::shadow(const std::string& name, int i) :
    process(name),
    index(i),

    size(::conf->get_i("shadow_map_resolution")),
    buff(::glob->new_frame(size, size, GL_TEXTURE_2D,
                           GL_RGBA8, false, true, false))
{
    GLfloat C[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    buff->bind_depth(GL_TEXTURE0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, C);
}

ogl::shadow::~shadow()
{
    assert(buff);
    ::glob->free_frame(buff);
}

//-----------------------------------------------------------------------------

void ogl::shadow::bind_frame() const
{
    assert(buff);
    buff->bind();
}

void ogl::shadow::free_frame() const
{
    assert(buff);
    buff->free();
}

void ogl::shadow::bind(GLenum unit) const
{
    assert(buff);
    buff->bind_depth(unit);
}

//-----------------------------------------------------------------------------
