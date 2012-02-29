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

#include <app-default.hpp>
#include <ogl-opengl.hpp>
#include <etc-math.hpp>
#include <app-user.hpp>
#include <app-data.hpp>
#include <app-conf.hpp>

// TODO: The use of set() is haphazzard.  current_M/I are accessed directly.
// Clarify this interface.

//-----------------------------------------------------------------------------

static double cubic(double t)
{
    if (t < 0.0) return 0.0;
    if (t > 1.0) return 1.0;

    return 3 * t * t - 2 * t * t * t;
}

//-----------------------------------------------------------------------------

app::user::user() :
    move_rate(1.0),
    turn_rate(1.0),
    file(DEFAULT_DEMO_FILE),
    root(0),
    prev(0),
    curr(0),
    next(0),
    pred(0),
    tt(0),
    stopped(true)
{
    const double S[16] = {
        0.5, 0.0, 0.0, 0.0,
        0.0, 0.5, 0.0, 0.0,
        0.0, 0.0, 0.5, 0.0,
        0.5, 0.5, 0.5, 1.0,
    };
    load_mat(current_S, S);

    move_rate = ::conf->get_f("view_move_rate");
    turn_rate = ::conf->get_f("view_turn_rate");

    // Initialize the demo input.

    root = file.get_root().find("demo");
    curr = root.find("key");

    prev = cycle_prev(curr);
    next = cycle_next(curr);
    pred = cycle_next(next);

    // Initialize the transformation using the initial state.

    int opts;

    set_state(curr, opts);
}

void app::user::set(const double *p, const double *q, double t)
{
    // Compute the current transform and inverse from the given values.

    if (q)
        quat_to_mat(current_M, q);

    if (p)
    {
        current_M[12] = p[0];
        current_M[13] = p[1];
        current_M[14] = p[2];
    }

    if (p || q)
    {
        orthonormalize(current_M);
        load_inv(current_I, current_M);
    }

    // Compute the current lighting vector from the given time.
    // TODO: compute this from lat/lon/date/time.

    if (t >= 0.0)
    {
        current_t = t;

        double M[16], L[3] = { 0.0, 0.0, 8192.0 }; // HACK

        load_rot_mat(M, 1.0, 0.0, 0.0, 120.0);
        Rmul_rot_mat(M, 0.0, 1.0, 0.0, 360.0 * t / (24.0 * 60.0 * 60.0));

        mult_mat_vec3(current_L, M, L);
    }
}

//-----------------------------------------------------------------------------

void app::user::get_point(double *P, const double *p,
                          double *V, const double *q) const
{
    double M[16], v[3];

    // Determine the point direction of the given quaternion.

    quat_to_mat(M, q);

    v[0] = -M[ 8];
    v[1] = -M[ 9];
    v[2] = -M[10];

    // Transform the point position and direction to world space.

    mult_mat_vec3(P, current_M, p);
    mult_xps_vec3(V, current_I, v);
}

//-----------------------------------------------------------------------------

app::node app::user::cycle_next(app::node n)
{
    // Return the next key, or the first key if there is no next.  O(1).

    app::node c = root.next(n, "key");
    if (!c)   c = root.find(   "key");

    return c;
}

app::node app::user::cycle_prev(app::node n)
{
    // Return the previous key, or the last key if there is no previous.  O(n).

    app::node l(0);
    app::node c(0);

    for (c = root.find("key"); c; c = root.next(c, "key"))

        if (cycle_next(c) == n)
            return c;
        else
            l = c;

    return l;
}

//-----------------------------------------------------------------------------

void app::user::set_t(double t)
{
    current_t = t;
    set(0, 0, current_t);
}

void app::user::turn(double rx, double ry, double rz, const double *R)
{
    // Turn in the given coordinate system.

    double T[16];

    load_xps(T, R);

#if CONFIG_EASY_FLIGHT
    turn(0, ry, 0);
#else
    mult_mat_mat(current_M, current_M, R);
    turn(rx, ry, rz);
    mult_mat_mat(current_M, current_M, T);
#endif

    orthonormalize(current_M);

    load_inv(current_I, current_M);
}

void app::user::turn(double rx, double ry, double rz)
{
    // Grab basis vectors (which change during transform).

    const double xx = current_M[ 0];
    const double xy = current_M[ 1];
    const double xz = current_M[ 2];

    const double yx = current_M[ 4];
    const double yy = current_M[ 5];
    const double yz = current_M[ 6];

    const double zx = current_M[ 8];
    const double zy = current_M[ 9];
    const double zz = current_M[10];

    const double px = current_M[12];
    const double py = current_M[13];
    const double pz = current_M[14];

    // Apply a local rotation transform.

    Lmul_xlt_inv(current_M, px, py, pz);
    Lmul_rot_mat(current_M, xx, xy, xz, rx);
    Lmul_rot_mat(current_M, yx, yy, yz, ry);
    Lmul_rot_mat(current_M, zx, zy, zz, rz);
    Lmul_xlt_mat(current_M, px, py, pz);

    orthonormalize(current_M);

    load_inv(current_I, current_M);
}

void app::user::move(double dx, double dy, double dz)
{
    Rmul_xlt_mat(current_M, dx, dy, dz);

    load_inv(current_I, current_M);
}

void app::user::look(double dt, double dp)
{
    const double x = current_M[12];
    const double y = current_M[13];
    const double z = current_M[14];

    // Apply a mouselook-style local rotation transform.

    Lmul_xlt_inv(current_M, x, y, z);
    Lmul_rot_mat(current_M, 0, 1, 0, dt);
    Lmul_xlt_mat(current_M, x, y, z);
    Rmul_rot_mat(current_M, 1, 0, 0, dp);

    orthonormalize(current_M);

    load_inv(current_I, current_M);
}

void app::user::pass(double dt)
{
    current_t += dt;
    set(0, 0, current_t);
}

void app::user::home()
{
    load_idt(current_M);
    load_idt(current_I);

    set(0, 0, 86400);
}

void app::user::set_M(const double *M)
{
    load_mat(current_M, M);
    load_inv(current_I, M);
}

void app::user::tumble(const double *A,
                       const double *B)
{
    double T[16];

    load_xps(T, B);

    mult_mat_mat(current_M, current_M, A);
    mult_mat_mat(current_M, current_M, T);

    orthonormalize(current_M);

    load_inv(current_I, current_M);
}

//-----------------------------------------------------------------------------

double app::user::interpolate(app::node A,
                              app::node B, const char *name, double t)
{
    // Cubic interpolator.

    const double y1 = A.get_f(name, 0);
    const double y2 = B.get_f(name, 0);

    double k = cubic(cubic(t));

    return y1 * (1.0 - k) + y2 * k;
}

void app::user::erp_state(app::node A,
                          app::node B, double tt, int &opts)
{
    // Apply the interpolation of the given state nodes.

    double p[3];
    double q[4];
    double time;

    p[0] = interpolate(A, B, "x", tt);
    p[1] = interpolate(A, B, "y", tt);
    p[2] = interpolate(A, B, "z", tt);

    q[0] = interpolate(A, B, "t", tt);
    q[1] = interpolate(A, B, "u", tt);
    q[2] = interpolate(A, B, "v", tt);
    q[3] = interpolate(A, B, "w", tt);

    time = interpolate(A, B, "time", tt);

    if (tt < 0.5)
        opts = A.get_i("opts", 0);
    else
        opts = B.get_i("opts", 0);

    set(p, q, time);
}

void app::user::set_state(app::node A, int &opts)
{
    // Apply the given state node.

    double p[3];
    double q[4];
    double time;

    p[0] = A.get_f("x", 0);
    p[1] = A.get_f("y", 0);
    p[2] = A.get_f("z", 0);

    q[0] = A.get_f("t", 0);
    q[1] = A.get_f("u", 0);
    q[2] = A.get_f("v", 0);
    q[3] = A.get_f("w", 0);

    time = A.get_i("time", 0);
    opts = A.get_i("opts", 0);

    set(p, q, time);
}

bool app::user::dostep(double dt, int &opts)
{
    // If we're starting backward, advance to the previous state.

    if (stopped && dt < 0.0)
    {
        if (tt == 0.0)
        {
            stopped = false;
            goprev();
            tt = 1.0;
        }
    }

    // If we're starting foreward, advance to the next state.

    if (stopped && dt > 0.0)
    {
        if (tt == 1.0)
        {
            stopped = false;
            gonext();
            tt = 0.0;
        }
    }

    tt += dt;

    // If we're going backward, stop at the current state.

    if (dt < 0.0 && tt < 0.0)
    {
        stopped = true;
        set_state(curr, opts);
        tt = 0.0;
        return true;
    }

    // If we're going forward, stop at the next state.

    if (dt > 0.0 && tt > 1.0)
    {
        stopped = true;
        set_state(next, opts);
        tt = 1.0;
        return true;
    }

    // Otherwise just interpolate the two.

    erp_state(curr, next, tt, opts);

    return false;
}

void app::user::gohalf()
{
    // Teleport half way to the next key.

    tt += (1.0 - tt) / 2.0;
}

void app::user::gonext()
{
    // Teleport to the next key.

    curr = cycle_next(curr);
    prev = cycle_prev(curr);
    next = cycle_next(curr);
    pred = cycle_next(next);
}

void app::user::goprev()
{
    // Teleport to the previous key.

    curr = cycle_prev(curr);
    prev = cycle_prev(curr);
    next = cycle_next(curr);
    pred = cycle_next(next);
}

void app::user::insert(int opts)
{
    if (stopped)
    {
        // If we're waiting at the end of a step, advance.

        if (tt == 1.0)
            gonext();

        double q[4];

        mat_to_quat(q, current_M);

        // Insert a new key after the current key.

        app::node c("key");

        c.set_f("x", current_M[12]);
        c.set_f("y", current_M[13]);
        c.set_f("z", current_M[14]);

        c.set_f("t", q[0]);
        c.set_f("u", q[1]);
        c.set_f("v", q[2]);
        c.set_f("w", q[3]);

        c.set_f("time", current_t);
        c.set_i("opts", opts);

        c.insert(root, curr);

        curr = c;
        prev = cycle_prev(curr);
        next = cycle_next(curr);
        pred = cycle_next(next);

        tt = 0.0;
    }
}

void app::user::remove()
{
    if (stopped)
    {
        // If we're waiting at the end of a step, advance.

        if (tt == 1.0)
            gonext();

        // Remove the current key (if it's not the only one left).

        app::node node = cycle_next(curr);

        if (node != curr)
        {
            curr.remove();

            curr = node;
            prev = cycle_prev(curr);
            next = cycle_next(curr);
            pred = cycle_next(next);

            tt = 0.0;
        }
    }
}

//-----------------------------------------------------------------------------

void app::user::draw() const
{
    // This is a view matrix rather than a model matrix.  It must be inverse.

    glLoadMatrixd(current_I);
}

//-----------------------------------------------------------------------------
