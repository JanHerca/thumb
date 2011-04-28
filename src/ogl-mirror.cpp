//  Copyright (C) 2010 Robert Kooima
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

#include <app-conf.hpp>
#include <app-glob.hpp>
#include <app-frustum.hpp>
#include <ogl-frame.hpp>
#include <ogl-binding.hpp>
#include <ogl-mirror.hpp>

//-----------------------------------------------------------------------------

ogl::mirror::mirror(std::string name, int w, int h) :
    binding(::glob->load_binding(name, name)),
    frame(new ogl::frame(w, h, GL_TEXTURE_RECTANGLE_ARB, GL_RGBA, true, true, false))
{
}

ogl::mirror::~mirror()
{
    ::glob->free_binding(binding);
}

//-----------------------------------------------------------------------------

void ogl::mirror::bind() const
{
    frame->bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    glScaled(1.0, -1.0, 1.0);
}

void ogl::mirror::free() const
{
    glPopMatrix();
    frame->free();
}

void ogl::mirror::draw(const app::frustum *frusp)
{
    frame->bind_color(GL_TEXTURE3);
    {
        const double *vp = frusp->get_view_pos();

        binding->bind(true);

        glEnable(GL_BLEND);
        glEnable(GL_POLYGON_OFFSET_FILL);
        {
            const double *v0 = frusp->get_points() + 0;
            const double *v1 = frusp->get_points() + 3;
            const double *v2 = frusp->get_points() + 6;
            const double *v3 = frusp->get_points() + 9;

            // Draw the far plane of the clip space, offset by one unit of
            // depth buffer distance.  Pass the world-space vectors from the
            // view position toward the screen corners for use in sky display.

            glPolygonOffset(0.0, -1.0);

            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glBegin(GL_QUADS);
            {
                glTexCoord2d(0, 0);
                glVertex3d(v0[0] - vp[0], v0[1] - vp[1], v0[2] - vp[2]);
                glTexCoord2d(1, 0);
                glVertex3d(v1[0] - vp[0], v1[1] - vp[1], v1[2] - vp[2]);
                glTexCoord2d(1, 1);
                glVertex3d(v3[0] - vp[0], v3[1] - vp[1], v3[2] - vp[2]);
                glTexCoord2d(0, 1);
                glVertex3d(v2[0] - vp[0], v2[1] - vp[1], v2[2] - vp[2]);
            }
            glEnd();
        }
        glDisable(GL_POLYGON_OFFSET_FILL);
        glDisable(GL_BLEND);
    }
    frame->free_color(GL_TEXTURE3);
}

//-----------------------------------------------------------------------------
