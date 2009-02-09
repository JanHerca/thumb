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

#include <cassert>

#include "wrl-world.hpp"
#include "app-user.hpp"
#include "app-frustum.hpp"
#include "mode-mode.hpp"

//-----------------------------------------------------------------------------

ogl::range mode::mode::prep(int frusc, app::frustum **frusv)
{
    assert(world);

    // Prep the world.

    return world->prep_fill(frusc, frusv);
}

void mode::mode::draw(int frusi, app::frustum *frusp)
{
    assert(world);

    // Draw the world.

     frusp->draw();
    ::user->draw();

    world->draw_fill(frusi, frusp);
}

//-----------------------------------------------------------------------------

