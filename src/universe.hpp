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

#ifndef UNIVERSE_HPP
#define UNIVERSE_HPP

#include "sphere.hpp"
#include "geomap.hpp"
#include "texture.hpp"

//-----------------------------------------------------------------------------

namespace uni
{
    class universe
    {
        geodat *D;
        georen *R;
        geomap *color;
        geomap *normal;
        geomap *height;
        sphere *S[3];

    public:

        universe();
       ~universe();

        void prep(const double *, int);
        void draw(const double *, const double *);

        double rate() const;
        void turn(int);
    };
}

//-----------------------------------------------------------------------------

#endif
