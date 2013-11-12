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

#include <cassert>

#include <SDL.h>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include <etc-math.hpp>
#include <app-conf.hpp>
#include <app-view.hpp>
#include <app-host.hpp>
#include <app-event.hpp>
#include <dev-mouse.hpp>

//-----------------------------------------------------------------------------

dev::mouse::mouse() :
    speed(5.0),
    dragging(false),
    modifier(0)
{
    key_move_F = conf->get_i("key_move_F", SDLK_w);
    key_move_L = conf->get_i("key_move_L", SDLK_a);
    key_move_R = conf->get_i("key_move_R", SDLK_d);
    key_move_B = conf->get_i("key_move_B", SDLK_s);
    key_move_U = conf->get_i("key_move_U", SDLK_e);
    key_move_D = conf->get_i("key_move_D", SDLK_q);

    motion[0] = 0;
    motion[1] = 0;
    motion[2] = 0;

    load_idt(init_R);
    load_idt(curr_R);
}

dev::mouse::~mouse()
{
}

//-----------------------------------------------------------------------------

bool dev::mouse::process_point(app::event *E)
{
    load_mat(init_R, curr_R);

    quat_to_mat(curr_R, E->data.point.q);

    if (dragging)
    {
        double p0 = DEG(atan2(init_R[9], init_R[10]));
        double p1 = DEG(atan2(curr_R[9], curr_R[10]));

        double t0 = DEG(atan2(init_R[8], init_R[10]));
        double t1 = DEG(atan2(curr_R[8], curr_R[10]));

        // ::view->look((t1 - t0) * 2.0, (p0 - p1) * 2.0);

        double M[16];
        double u[3];

        ::host->get_world_up_vector(u);

        load_rot_mat(M, u[0], u[1], u[2], (t1 - t0) * 2.0);
        Rmul_rot_mat(M, 1,    0,    0,    (p0 - p1) * 2.0);

        ::host->navigate(M);

        return true;
    }
    return false;
}

bool dev::mouse::process_click(app::event *E)
{
    const bool d = E->data.click.d;
    const int  b = E->data.click.b;
    const int  m = E->data.click.m;

    // Handle rotating the view.

    if (b == SDL_BUTTON_RIGHT && m == 0)
    {
        dragging = d;
        return true;
    }
    return false;
}

bool dev::mouse::process_key(app::event *E)
{
    const bool d = E->data.key.d;
    const int  k = E->data.key.k;
    const int  m = E->data.key.m;

    int dd = d ? +1 : -1;

    modifier = m;

    // Handle motion keys.

    if      (k == key_move_L) { motion[0] -= dd; return true; }
    else if (k == key_move_R) { motion[0] += dd; return true; }
    else if (k == key_move_D) { motion[1] -= dd; return true; }
    else if (k == key_move_U) { motion[1] += dd; return true; }
    else if (k == key_move_F) { motion[2] -= dd; return true; }
    else if (k == key_move_B) { motion[2] += dd; return true; }

    // Teleport home.

    else if (d)
    {
        if (k == SDLK_RETURN)
        {
            ::view->home();
            return true;
        }
    }

    return false;
}

bool dev::mouse::process_tick(app::event *E)
{
    double kp = E->data.tick.dt * speed;

    if (modifier & KMOD_SHIFT) kp *= 10.0;
    if (modifier & KMOD_CTRL)  kp *=  0.1;

    double M[16];

    load_xlt_mat(M, motion[0] * kp,
                    motion[1] * kp,
                    motion[2] * kp);

    ::host->navigate(M);

    return false;
}

bool dev::mouse::process_event(app::event *E)
{
    assert(E);

    bool R = false;

    switch (E->get_type())
    {
    case E_POINT: R |= process_point(E); break;
    case E_CLICK: R |= process_click(E); break;
    case E_TICK:  R |= process_tick(E);  break;
    case E_KEY:   R |= process_key(E);   break;
    }

    return R || dev::input::process_event(E);
}

//-----------------------------------------------------------------------------
