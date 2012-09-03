//  Copyright (C) 2005-2012 Robert Kooima
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

#ifndef VIEW_PATH_HPP
#define VIEW_PATH_HPP

#include <vector>
#include <string>

#include "view-step.hpp"

//------------------------------------------------------------------------------

class view_path
{
public:

    view_path();

    void clear();

    void fore(bool);
    void back(bool);
    void stop();
    void next();
    void prev();
    void home();
    void jump();

    void get(view_step&);
    void set(view_step&);
    void ins(view_step&);
    void add(view_step&);
    void del();

    void faster();
    void slower();
    void inc_tens();
    void dec_tens();
    void inc_bias();
    void dec_bias();

    void save();
    void load();

    void time(double);
    void draw() const;

    bool       playing() const { return (head_d != 0.0); }
    view_step& current()       { return step[curr];      }

private:

    view_step erp(double) const;
    view_step now()       const { return erp(head_t); }

    double head_t;
    double head_d;
    int    curr;

    std::string filename;

    std::vector<view_step> step;
};

//------------------------------------------------------------------------------

#endif
