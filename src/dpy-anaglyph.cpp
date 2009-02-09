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

#include <cassert>

#include "matrix.hpp"
#include "app-glob.hpp"
#include "app-prog.hpp"
#include "app-event.hpp"
#include "app-frustum.hpp"
#include "ogl-program.hpp"
#include "dpy-channel.hpp"
#include "dpy-anaglyph.hpp"

//-----------------------------------------------------------------------------

dpy::anaglyph::anaglyph(app::node node) :
    display(node), frustL(0), frustR(0), P(0)
{
    app::node curr;

    // Check the display definition for a frustum, or create a default

    if ((curr = app::find(node, "frustum")))
    {
        frustL = new app::frustum(curr, viewport[2], viewport[3]);
        frustR = new app::frustum(curr, viewport[2], viewport[3]);
    }
    else
    {
        frustL = new app::frustum(0,    viewport[2], viewport[3]);
        frustR = new app::frustum(0,    viewport[2], viewport[3]);
    }
}

dpy::anaglyph::~anaglyph()
{
    delete frustR;
    delete frustL;
}

//-----------------------------------------------------------------------------

void dpy::anaglyph::get_frustums(app::frustum_v& frustums)
{
    assert(frustL);
    assert(frustR);

    // Add my frustums to the list.

    frustums.push_back(frustL);
    frustums.push_back(frustR);
}

void dpy::anaglyph::prep(int chanc, dpy::channel **chanv)
{
    assert(frustL);
    assert(frustR);

    // Apply the channel view positions to the frustums.

    if (chanc > 0) frustL->calc_user_planes(chanv[0]->get_p());
    if (chanc > 1) frustR->calc_user_planes(chanv[1]->get_p());
}

int dpy::anaglyph::draw(int chanc, dpy::channel **chanv,
                        int frusi, app::frustum  *frusp)
{
    if (chanc > 1)
    {
        assert(chanv[0]);
        assert(chanv[1]);
        assert(P);

        // Draw the scene to the off-screen buffer.

        chanv[0]->bind();
        {
            ::prog->draw(frusi + 0, frustL);
        }
        chanv[0]->free();
        chanv[1]->bind();
        {
            ::prog->draw(frusi + 1, frustR);
        }
        chanv[1]->free();

        // Draw the off-screen buffer to the screen.

        chanv[0]->bind_color(GL_TEXTURE0);
        chanv[1]->bind_color(GL_TEXTURE1);
        {
            P->bind();
            {
                fill(frustL->get_w(),
                     frustL->get_h(),
                     chanv[0]->get_w(),
                     chanv[0]->get_h());
            }
            P->free();
        }
        chanv[1]->free_color(GL_TEXTURE1);
        chanv[0]->free_color(GL_TEXTURE0);
    }

    return 2;  // Return the total number of frusta.
}

int dpy::anaglyph::test(int chanc, dpy::channel **chanv, int index)
{
    // PUNT

    return 2;
}

//-----------------------------------------------------------------------------

bool dpy::anaglyph::pointer_to_3D(app::event *E, int x, int y)
{
    assert(frustL);

    // Determine whether the pointer falls within the viewport.

    if (viewport[0] <= x && x < viewport[0] + viewport[2] &&
        viewport[1] <= y && y < viewport[1] + viewport[3])

        // Let the frustum project the pointer into space.

        return frustL->pointer_to_3D(E, x - viewport[0],
                          viewport[3] - y + viewport[1]);
    else
        return false;
}

bool dpy::anaglyph::process_start(app::event *E)
{
    // Initialize the shader.

    if ((P = ::glob->load_program("anaglyph.xml")))
    {
        P->bind();
        P->uniform("luma", 0.30, 0.59, 0.11, 0.00);
        P->free();
    }

    return false;
}

bool dpy::anaglyph::process_close(app::event *E)
{
    // Finalize the shader.

    ::glob->free_program(P);

    P = 0;

    return false;
}

bool dpy::anaglyph::process_event(app::event *E)
{
    assert(frustL);
    assert(frustR);

    // Do the local startup or shutdown.

    switch (E->get_type())
    {
    case E_START: process_start(E); break;
    case E_CLOSE: process_close(E); break;
    }

    // Let the frustum handle the event.

    return (frustL->process_event(E) |
            frustR->process_event(E));
}

//-----------------------------------------------------------------------------