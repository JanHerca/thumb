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

#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <string>

#include "opengl.hpp"

//-----------------------------------------------------------------------------

namespace ogl
{
    class texture
    {
        std::string name;

        GLenum target;
        GLenum format;
        GLuint object;

        void load_png(const void *, size_t);
        void load_jpg(const void *, size_t);
        void load_img(std::string);

    public:

        const std::string& get_name() const { return name; }

        texture(std::string);
       ~texture();

        void bind(GLenum=GL_TEXTURE0) const;
        void free(GLenum=GL_TEXTURE0) const;
    };
}

//-----------------------------------------------------------------------------

#endif