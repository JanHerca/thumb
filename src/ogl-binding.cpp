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

#include <cmath>

#include "app-conf.hpp"
#include "app-glob.hpp"
#include "ogl-frame.hpp"
#include "ogl-texture.hpp"
#include "ogl-program.hpp"
#include "ogl-binding.hpp"

//-----------------------------------------------------------------------------

double ogl::binding::split_depth[4];

std::vector<ogl::frame *> ogl::binding::shadow;

double ogl::binding::split(int i, double n, double f)
{
    double k = double(i) / double(shadow.size());

    double c = (n * pow(f / n, k) + n + (f - n) * k) * 0.5;

    split_depth[i] = (1 - n / c) * f / (f - n);

    return (c - n) / (f - n);
}

bool ogl::binding::init_shadow()
{
    // Initialize the shadow maps if necessary.

    if (shadow.empty())
    {
        if (ogl::do_shadows)
        {
            int n = ::conf->get_i("shadow_map_splits");
            int s = ::conf->get_i("shadow_map_resolution");

            shadow.reserve(n);

            for (int i = 0; i < n; ++i)
                shadow.push_back(::glob->new_frame(s, s));
        }
    }
    return !shadow.empty();
}

int ogl::binding::shadow_count()
{
    if (init_shadow())
        return shadow.size();
    else
        return 0;
}

bool ogl::binding::bind_shadow_frame(int i)
{
    if (init_shadow() && shadow[i])
    {
        shadow[i]->bind();
        return true;
    }
    return false;
}

bool ogl::binding::bind_shadow_color(int i, GLenum unit)
{
    if (init_shadow() && shadow[i])
    {
        shadow[i]->bind_color(unit);
        return true;
    }
    return false;
}

bool ogl::binding::bind_shadow_depth(int i, GLenum unit)
{
    if (init_shadow() && shadow[i])
    {
        shadow[i]->bind_depth(unit);
        return true;
    }
    return false;
}

void ogl::binding::draw_shadow_color(int i)
{
    if (shadow[i])
        shadow[i]->draw(i, shadow.size());
}

void ogl::binding::free_shadow_frame(int i)
{
    if (shadow[i])
        shadow[i]->free();
}

void ogl::binding::free_shadow_color(int i)
{
    if (shadow[i])
        shadow[i]->free_color();
}

void ogl::binding::free_shadow_depth(int i)
{
    if (shadow[i])
        shadow[i]->free_depth();
}

//-----------------------------------------------------------------------------

ogl::binding::binding(std::string name) :
    name(name),
    depth_program(0),
    color_program(0)
{
    std::string path = "material/" + name + ".xml";

    app::serial file(path.c_str());
    app::node   root;
    app::node   node;

    init_shadow();

    if ((root = app::find(file.get_head(), "material")))
    {
        // Load the depth-mode program.

        if ((node = app::find(root, "program", "mode", "depth")))
            depth_program = glob->load_program(app::get_attr_s(node, "file"));

        // Load the color-mode program.

        if ((node = app::find(root, "program", "mode", "color")))
            color_program = glob->load_program(app::get_attr_s(node, "file"));

        // Determine the shadow map bindings.  TODO: generalize this.

        GLenum unit;

        if (0 < shadow.size() && (unit = color_program->unit("shadow0")))
            light_texture[unit] = shadow[0];
        if (1 < shadow.size() && (unit = color_program->unit("shadow1")))
            light_texture[unit] = shadow[1];
        if (2 < shadow.size() && (unit = color_program->unit("shadow2")))
            light_texture[unit] = shadow[2];

        // Load all textures.

        for (node = app::find(root,       "texture"); node;
             node = app::next(root, node, "texture"))
        {
            std::string name(app::get_attr_s(node, "name"));
            std::string file(app::get_attr_s(node, "file"));

            std::string path = "texture/" + file;

            // Determine the texture unit binding for each texture.

            GLenum depth_unit = depth_program ? depth_program->unit(name) : 0;
            GLenum color_unit = color_program ? color_program->unit(name) : 0;

            // Load the texture (with reference count +2).

            if (depth_unit)
                depth_texture[depth_unit] = ::glob->load_texture(path);
            if (color_unit)
                color_texture[color_unit] = ::glob->load_texture(path);
        }
    }
}

ogl::binding::~binding()
{
    // Free all textures.
    
    unit_texture::iterator i;

    for (i = color_texture.begin(); i != color_texture.end(); ++i)
        ::glob->free_texture(i->second);

    for (i = depth_texture.begin(); i != depth_texture.end(); ++i)
        ::glob->free_texture(i->second);

    color_texture.clear();
    depth_texture.clear();
    light_texture.clear();

    // Free all programs.

    if (depth_program) glob->free_program(depth_program);
    if (color_program) glob->free_program(color_program);
}

//-----------------------------------------------------------------------------

// Determine whether two bindings are equivalent in depth rendering mode.

bool ogl::binding::depth_eq(const binding *that) const
{
    // If the pointers are the same then they are trivially equivalent.

    if (that == this)
        return true;

    // If both bindings are opaque then they are functionally equivalent.

    if (opaque() && that->opaque())
        return true;

    // If the programs differ then the bindings are not equivalent.

    if (depth_program != that->depth_program)
        return false;

    // If the textures differ then the bindings are not equivalent.

    if (depth_texture != that->depth_texture)
        return false;

    return true;
}

// Determine whether two bindings are equivalent in color rendering mode.

bool ogl::binding::color_eq(const binding *that) const
{
    // If the pointers are the same then they are trivially equivalent.

    if (that == this)
        return true;

    // If the programs differ then the bindings are not equivalent.

    if (color_program != that->color_program)
        return false;

    // If the textures differ then the bindings are not equivalent.

    if (color_texture != that->color_texture)
        return false;

    return true;
}

// Determine whether the material binding is opaque.

bool ogl::binding::opaque() const
{
    // HACK: test lowest-unit-numbered texture for opacity.

    if (!color_texture.empty())
        return (*color_texture.begin()).second->opaque();

    return true;
}

//-----------------------------------------------------------------------------

void ogl::binding::bind(bool c) const
{
    // TODO: Minimize rebindings

    unit_texture::const_iterator ti;
    unit_frame  ::const_iterator fi;

    if (c)
    {
        if (color_program)
        {
            color_program->bind();

            // TODO: there's a chance this can be lofted.

            color_program->uniform("split", split_depth[0],
                                            split_depth[1],
                                            split_depth[2],
                                            split_depth[3]);
        }

        for (ti = color_texture.begin(); ti != color_texture.end(); ++ti)
            ti->second->bind(ti->first);

        for (fi = light_texture.begin(); fi != light_texture.end(); ++fi)
            fi->second->bind_depth(fi->first);
    }
    else
    {
        if (depth_program)
            depth_program->bind();

        for (ti = depth_texture.begin(); ti != depth_texture.end(); ++ti)
            ti->second->bind(ti->first);
    }
}

//-----------------------------------------------------------------------------

void ogl::binding::init()
{
}

void ogl::binding::fini()
{
}

//-----------------------------------------------------------------------------
