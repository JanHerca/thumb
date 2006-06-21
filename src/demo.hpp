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

#ifndef DEMO_HPP
#define DEMO_HPP

#include <set>

#include "opengl.hpp"
#include "camera.hpp"
#include "image.hpp"
#include "light.hpp"
#include "scene.hpp"
#include "earth.hpp"
#include "sky.hpp"
#include "edit.hpp"
#include "play.hpp"
#include "info.hpp"
#include "prog.hpp"

//-----------------------------------------------------------------------------

class demo : public app::prog
{
    static const int edit_mode = 0;
    static const int play_mode = 0;
    static const int info_mode = 0;

    // Configuration.

    int key_play;
    int key_info;

    int key_move_L;
    int key_move_R;
    int key_move_F;
    int key_move_B;
        
    float camera_move_rate;
    float camera_turn_rate;
    float camera_zoom;
    float camera_near;
    float camera_far;

    // Entity state.

    ent::camera camera;
    ent::light  sun;
    ent::sky    sky;
    ent::earth  earth;
    ops::scene  scene;

    // Editor mode.

    mode::edit  edit;
    mode::play  play;
    mode::info  info;
    mode::mode *curr;

    void goto_mode(mode::mode *);

    // Demo state.

    int last_x;
    int last_y;
    int button[4];
    int motion[3];

    void draw_all() const;

public:

    demo();
   ~demo();

    void point(int, int);
    void click(int, bool);
    void keybd(int, bool, int);
    void timer(float);

    void draw() const;
};

//-----------------------------------------------------------------------------

#endif
