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

#include <iostream>
#include <cstring>

#include "ogl-opengl.hpp"
#include "demo.hpp"
#include "app-conf.hpp"
#include "app-user.hpp"
#include "app-host.hpp"
#include "dev-mouse.hpp"
#include "mode-edit.hpp"
#include "mode-play.hpp"
#include "mode-info.hpp"

//-----------------------------------------------------------------------------

demo::demo() : edit(0), play(0), info(0), curr(0), input(0)
{
    std::string input_mode = conf->get_s("input_mode");

    // Initialize the demo configuration.

/*
    tracker_head_sensor = conf->get_i("tracker_head_sensor");
    tracker_hand_sensor = conf->get_i("tracker_hand_sensor");
*/

    // Initialize the input handler.

    if (input_mode == "mouse") input = new dev::mouse(universe);

    // Initialize attract mode.

    attr_time = conf->get_f("attract_delay");
    attr_curr = 0;
    attr_mode = false;

    // Initialize the application state.

    key_edit   = conf->get_i("key_edit");
    key_play   = conf->get_i("key_play");
    key_info   = conf->get_i("key_info");

//  edit = new mode::edit(world);
//  play = new mode::play(world);
//  info = new mode::info(world);

//  goto_mode(play);
}

demo::~demo()
{
    if (input) delete input;

    if (info) delete info;
    if (play) delete play;
    if (edit) delete edit;
}

//-----------------------------------------------------------------------------

void demo::goto_mode(mode::mode *next)
{
    if (curr) curr->leave();
    if (next) next->enter();

    curr = next;
}

void demo::attr_on()
{
    attr_mode = true;
    attr_stop = false;
    ::user->gonext(5.0);
}

void demo::attr_off()
{
    attr_mode = false;
    attr_curr = 0;
}

void demo::attr_next()
{
    attr_mode = true;
    attr_stop = true;
    ::user->gonext(2.0);
}

void demo::attr_prev()
{
    attr_mode = true;
    attr_stop = true;
    ::user->goprev(2.0);
}

//-----------------------------------------------------------------------------

void demo::point(int i, const double *p, const double *q)
{
    if (input && input->point(i, p, q))
        attr_off();
}

void demo::click(int i, int b, int m, bool d)
{
    if (input && input->click(i, b, m, d))
        attr_off();

    if (curr)
    {
        if (curr->click(i, b, m, d) == false)
            prog::click(i, b, m, d);
    }
}

void demo::keybd(int c, int k, int m, bool d)
{
    if (input && input->keybd(c, k, m, d))
        attr_off();
    else
    {
        prog::keybd(c, k, m, d);

        // Handle mode transitions.

        if      (d && k == key_edit && curr != edit) goto_mode(edit);
        else if (d && k == key_play && curr != play) goto_mode(play);
        else if (d && k == key_info && curr != info) goto_mode(info);

        // Let the current mode take it.

        if (curr == 0 || curr->keybd(c, k, m, d) == false)
        {
            // Handle guided view keys.

            if (d && (m & KMOD_SHIFT))
            {
                if      (k == SDLK_PAGEUP)   attr_next();
                else if (k == SDLK_PAGEDOWN) attr_prev();
                else if (k == SDLK_INSERT)   ::user->insert(universe.get_a(),
                                                            universe.get_t());
                else if (k == SDLK_DELETE)   ::user->remove();
                else if (k == SDLK_SPACE)    attr_on();
            }
        }
    }
}

void demo::timer(int t)
{
    double dt = t / 1000.0;

    if (attr_mode)
    {
        double axis = 0.0;
        double tilt = 0.0;

        if (user->dostep(dt, universe.get_p(), axis, tilt))
        {
            if (attr_stop)
                attr_off();
            else
                user->gonext(10.0, 5.0);
        }
        else
        {
            universe.set_a(axis);
            universe.set_t(tilt);
        }
    }
    else
    {
        if (input && input->timer(t))
            attr_off();
        else
        {
            attr_curr += dt;

            if (attr_curr > attr_time)
                attr_on();
        }
    }
/*
        double kp = dt * universe.rate();
        double kr = dt * view_turn_rate;

        double dP[3];
        double dR[3];
        double dz[3];
        double dy[3];

        dP[0] = curr_P[ 0] - init_P[ 0];
        dP[1] = curr_P[ 1] - init_P[ 1];
        dP[2] = curr_P[ 2] - init_P[ 2];
        
        dy[0] = init_R[ 4] - curr_R[ 4];
        dy[1] = init_R[ 5] - curr_R[ 5];
        dy[2] = init_R[ 6] - curr_R[ 6];

        dz[0] = init_R[ 8] - curr_R[ 8];
        dz[1] = init_R[ 9] - curr_R[ 9];
        dz[2] = init_R[10] - curr_R[10];

        if (button[1])
        {
            dR[0] =  DOT3(dz, init_R + 4);
            dR[1] = -DOT3(dz, init_R + 0);
            dR[2] =  DOT3(dy, init_R + 0);

            user->turn(dR[0] * kr, dR[1] * kr, dR[2] * kr, curr_R);
            user->move(dP[0] * kp, dP[1] * kp, dP[2] * kp);
        }
        else if (button[3])
        {
            dR[0] =  DOT3(dz, init_R + 4);
            dR[1] =  0;
            dR[2] = -DOT3(dz, init_R + 0);

            user->turn(dR[0] * kr, dR[1] * kr, dR[2] * kr, curr_R);
            user->move(dP[0] * kp, dP[1] * kp, dP[2] * kp);
        }
        else
*/

    if (curr)
        curr->timer(t);
    else
        prog::timer(t);
}

void demo::value(int d, int a, double v)
{
    if (input && input->value(d, a, v))
        attr_off();
}

//-----------------------------------------------------------------------------

void demo::prep(app::frustum_v& frusta)
{
    universe.prep(frusta);
}

void demo::draw(int i)
{
/*
    GLfloat A[4] = { 0.45f, 0.50f, 0.55f, 0.0f };
    GLfloat D[4] = { 1.00f, 1.00f, 0.90f, 0.0f };
*/

    GLfloat A[4] = { 0.25f, 0.25f, 0.25f, 0.0f };
    GLfloat D[4] = { 1.00f, 1.00f, 1.00f, 0.0f };

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, A);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, D);

    glPushAttrib(GL_ENABLE_BIT);
    {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_NORMALIZE);
        glEnable(GL_LIGHTING);

        glClearColor(0.0f, 0.1f, 0.2f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        universe.draw(i);
    }
    glPopAttrib();
}

//-----------------------------------------------------------------------------

