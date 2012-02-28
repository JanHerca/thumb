//  Copyright (C) 2005-2011 Robert Kooima
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

#ifndef SPH_VIEWER_HPP
#define SPH_VIEWER_HPP

#include <vector>

#include <app-prog.hpp>
#include <app-file.hpp>

#include "sph-cache.hpp"
#include "sph-model.hpp"
#include "sph-loader.hpp"

//-----------------------------------------------------------------------------

class sph_frame
{
public:

    sph_frame(sph_cache *, app::node);

    int get(int i) { return file.empty() ? 0 : file[i % file.size()]; }

private:

    std::vector<int> file;
};

//-----------------------------------------------------------------------------

class sph_viewer : public app::prog
{
public:

    sph_viewer(const std::string&, const std::string&);
   ~sph_viewer();

    ogl::range prep(int, const app::frustum * const *);
    void       lite(int, const app::frustum * const *);
    void       draw(int, const app::frustum *, int);

    virtual bool process_event(app::event *);

    void load(const std::string&);
    void unload();
    void cancel();

    void goto_next();
    void goto_prev();

protected:

    sph_cache *cache;
    sph_model *model;

private:

    // Sphere rendering state

    std::vector<sph_frame *> frame;

    float timer;
    float timer_d;
    float timer_e;
    float height;
    float radius;

    bool debug_cache;
    bool debug_color;

    // Sphere GUI State

    sph_loader *ui;

    bool gui_state;
    void gui_init();
    void gui_free();
    void gui_draw();
    bool gui_point(app::event *);
    bool gui_click(app::event *);
    bool gui_key  (app::event *);
};

//-----------------------------------------------------------------------------

#endif
