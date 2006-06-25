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

#ifndef SOLID_HPP
#define SOLID_HPP

#include "opengl.hpp"
#include "entity.hpp"
#include "param.hpp"

//-----------------------------------------------------------------------------

namespace ent
{
    class solid : public entity
    {
    protected:

        dGeomID tran;
        dMass   mass;

        const ogl::shader *lite_prog;
        const ogl::shader *dark_prog;

    public:

        solid(int f=-1);

        virtual int join(int)    { return 0; }
        virtual int join() const { return 0; }

        virtual void play_init(dBodyID);
        virtual void play_tran(dBodyID);
        virtual void play_fini();

        virtual int  draw_prio(bool) { return 2; }
        virtual void draw_dark();
        virtual void draw_lite();
        virtual void step_post();

        virtual void         load(mxml_node_t *);
        virtual mxml_node_t *save(mxml_node_t *);

        virtual ~solid() { }
    };
}

//-----------------------------------------------------------------------------

namespace ent
{
    class free : public solid
    {
    public:
        virtual free *clone() const { return new free(*this); }
        free(int f=-1)    : solid(f) { }

        void edit_init() { }
    };

    class box : public solid
    {
    protected:
        void draw_geom() const;
    public:
        virtual box *clone() const { return new box(*this); }
        box(int f=-1)     : solid(f) { }

        void edit_init();
        void play_init(dBodyID);

        virtual mxml_node_t *save(mxml_node_t *);
    };

    class sphere : public solid
    {
    protected:
        void draw_geom() const;
    public:
        virtual sphere *clone() const { return new sphere(*this); }
        sphere(int f=-1)  : solid(f) { }

        void edit_init();
        void play_init(dBodyID);

        virtual mxml_node_t *save(mxml_node_t *);
    };

    class capsule : public solid
    {
    protected:
        void draw_geom() const;
    public:
        virtual capsule *clone() const { return new capsule(*this); }
        capsule(int f=-1) : solid(f) { }

        void edit_init();
        void play_init(dBodyID);

        virtual mxml_node_t *save(mxml_node_t *);
    };

    class plane : public solid
    {
    public:
        virtual plane *clone() const { return new plane(*this); }
        plane(int f=-1)   : solid(f) { }

        virtual void edit_init();
    };
}

//-----------------------------------------------------------------------------

#endif
