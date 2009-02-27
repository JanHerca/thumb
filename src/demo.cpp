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

#include <SDL/SDL_keyboard.h>

#include "ogl-opengl.hpp"
#include "ogl-uniform.hpp"

#include "demo.hpp"

#include "app-event.hpp"
#include "app-conf.hpp"
#include "app-user.hpp"
#include "app-host.hpp"
#include "app-glob.hpp"

#include "wrl-world.hpp"
#include "uni-universe.hpp"

#include "dev-mouse.hpp"
/*
#include "dev-gamepad.hpp"
#include "dev-tracker.hpp"
#include "dev-wiimote.hpp"
*/

#include "mode-edit.hpp"
#include "mode-play.hpp"
#include "mode-info.hpp"

//-----------------------------------------------------------------------------

demo::demo(int w, int h) :
    universe(0), world(0), edit(0), play(0), info(0), curr(0), input(0)
{
    std::string input_mode = conf->get_s("input_mode");

    // Initialize the input handler.
/*
    if      (input_mode == "gamepad") input = new dev::gamepad();
    else if (input_mode == "tracker") input = new dev::tracker();
    else if (input_mode == "wiimote") input = new dev::wiimote();
    else */                           input = new dev::mouse  ();

    // Initialize the uniforms.

    uniform_light_position = ::glob->load_uniform("light_position", 3);
    uniform_view_matrix    = ::glob->load_uniform("view_matrix",   16);
    uniform_view_inverse   = ::glob->load_uniform("view_inverse",  16);
    uniform_view_position  = ::glob->load_uniform("view_position",  3);
    uniform_time           = ::glob->load_uniform("time",           1);

    // Initialize attract mode.

    attr_time = conf->get_f("attract_delay");
    attr_rate = conf->get_f("attract_speed");
    attr_curr = 0.0;
    attr_sign = 1.0;
    attr_mode = false;
    attr_stop = false;

    // Initialize the application state.

    key_edit  = conf->get_i("key_edit");
    key_play  = conf->get_i("key_play");
    key_info  = conf->get_i("key_info");

//  universe = new uni::universe(w, h);
    world    = new wrl::world();

    edit = new mode::edit(world);
    play = new mode::play(world);
    info = new mode::info(world);

    goto_mode(info);

    attr_step(0.0);
}

demo::~demo()
{
    ::glob->free_uniform(uniform_time);
    ::glob->free_uniform(uniform_view_position);
    ::glob->free_uniform(uniform_view_inverse);
    ::glob->free_uniform(uniform_view_matrix);
    ::glob->free_uniform(uniform_light_position);

    if (info) delete info;
    if (play) delete play;
    if (edit) delete edit;

    if (world)    delete world;
    if (universe) delete universe;
    if (input)    delete input;
}

//-----------------------------------------------------------------------------

void demo::goto_mode(mode::mode *next)
{
    // Synthesize CLOSE and START events for the mode transition.

    app::event E;

    if (curr) { E.mk_close(); curr->process_event(&E); }
    if (next) { E.mk_start(); next->process_event(&E); }

    curr = next;
}

void demo::attr_on()
{
    attr_curr = 0.0;
    attr_mode = true;
    attr_stop = false;
}

void demo::attr_off()
{
    attr_curr = 0.0;
    attr_mode = false;
}

void demo::attr_step(double dt)
{
    // Move the camera forward.

    if (attr_mode)
    {
        int opts = 0;

        if (user->dostep(attr_rate * attr_sign * dt, opts))
        {
            if (attr_stop)
                attr_off();
        }
        set_options(opts);
    }
}

void demo::attr_next()
{
    // Play forward to the next key.

    attr_sign =  1.0;
    attr_curr =  0.0;
    attr_stop = true;
    attr_mode = true;
}

void demo::attr_prev()
{
    // Play backward to the previous key.

    attr_sign = -1.0;
    attr_curr =  0.0;
    attr_stop = true;
    attr_mode = true;
}

void demo::attr_ins()
{
    // Insert a new key here.

    ::user->insert(get_options());
    attr_step(0.0);
}

void demo::attr_del()
{
    // Remove the current key.

    ::user->remove();
    attr_step(0.0);
}

//-----------------------------------------------------------------------------

void demo::next()
{
    attr_next();
}

void demo::prev()
{
    attr_prev();
}

//-----------------------------------------------------------------------------

bool demo::process_keybd(app::event *E)
{
    const bool d = E->data.keybd.d;
    const int  k = E->data.keybd.k;
    const int  m = E->data.keybd.m;

    // Handle application mode transitions.

    if (d)
    {
        if (k == key_edit && curr != edit) { goto_mode(edit); return true; }
        if (k == key_play && curr != play) { goto_mode(play); return true; }
        if (k == key_info && curr != info) { goto_mode(info); return true; }
    }

    // Handle attract mode controls.

    if (d && (m & KMOD_SHIFT))
    {
        if      (k == SDLK_PAGEUP)   { attr_next(); return true; }
        else if (k == SDLK_PAGEDOWN) { attr_prev(); return true; }
        else if (k == SDLK_END)      { attr_ins();  return true; }
        else if (k == SDLK_HOME)     { attr_del();  return true; }

        else if (k == SDLK_SPACE)
        {
            if (m & KMOD_CTRL)
            {
                ::host->set_bench_mode(1);
                ::host->set_movie_mode(2);
            }
            attr_on();
            return true;
        }
    }

    return false;
}

bool demo::process_timer(app::event *E)
{
    double dt = E->data.timer.dt * 0.001;

    // Step attract mode, if enabled.

    if (attr_mode)
        attr_step(dt);

    // Otherwise, if the attract delay has expired, enable attract mode.

    else
    {
        attr_curr += dt;

        if (attr_curr > attr_time)
            attr_on();
    }

    // Always return IGNORED to allow other objects to process time.

    return false;
}

bool demo::process_input(app::event *E)
{
    // Assume all script inputs are meaningful.  Pass them to the universe.

    if (universe)
        universe->script(E->data.input.src, NULL);

    return true;
}

bool demo::process_event(app::event *E)
{
    bool R = false;

    // Attempt to process the given event.

    switch (E->get_type())
    {
    case E_KEYBD: R = process_keybd(E); break;
    case E_INPUT: R = process_input(E); break;
    case E_TIMER: R = process_timer(E); break;
    }

    if (R) return true;

    // Allow the application mode, the device, or the base to handle the event.

    if ((curr  &&  curr->process_event(E)) ||
        (input && input->process_event(E)) ||
        (          prog::process_event(E)))

        // If the event was handled, disable the attract mode.
    {
        ::host->set_bench_mode(0);
        ::host->set_movie_mode(0);

        attr_off();
        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------

ogl::range demo::prep(int frusc, app::frustum **frusv)
{
    double t = SDL_GetTicks() * 0.001f;

    // Set the frame-constant uniforms.

    uniform_light_position->set(::user->get_L());
    uniform_view_matrix   ->set(::user->get_I());
    uniform_view_inverse  ->set(::user->get_M());
    uniform_view_position ->set(::user->get_M() + 12);

    uniform_time->set(&t);

    // Prep the current mode, giving the view range.

    ogl::range r;

    if (curr)
        r = curr->prep(frusc, frusv);
    else
        r = ogl::range();

    ::glob->prep();

    return r;
}

void demo::draw(int frusi, app::frustum *frusp)
{
    // Clear the render target.

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT);

    if (curr)
        curr->draw(frusi, frusp);
}

//-----------------------------------------------------------------------------

