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
#include <cstring>
#include <iostream>

#include "uni-geomap.hpp"
#include "uni-geocsh.hpp"
#include "app-file.hpp"
#include "app-glob.hpp"
#include "app-conf.hpp"
#include "matrix.hpp"
#include "util.hpp"

//-----------------------------------------------------------------------------

uni::page::page(int w, int h, int s,
                int x, int y, int d, int c,
                double _W, double _E, double _S, double _N) :
    st(page_default), d(d), c(c), a(0),
    visible_cache_serial(0),
    visible_cache_result(false)
{
    int t = s << d;

    // Compute the mipmap file indices.

    i = y / t;
    j = x / t;

    // Compute the texture boundries.

    double W, E, S, N;

    W = (_W + (_E - _W) * (x    ) / w);
    E = (_W + (_E - _W) * (x + t) / w);
    S = (_S + (_N - _S) * (y    ) / h);
    N = (_S + (_N - _S) * (y + t) / h);

    // Compute the cap normal.

    const double cW = std::max(W, -PI);
    const double cE = std::min(E,  PI);
    const double cS = std::max(S, -PI_2);
    const double cN = std::min(N,  PI_2);

    const double cH = (cW + cE) / 2;
    const double cV = (cS + cN) / 2;

    sphere_to_vector(n, cH, cV, 1.0);

    // Compute the cap angle.

    double v[3];

    if (cS < 0 && 0 < cN)
    {
        sphere_to_vector(v, cW, 0, 1.0); a = std::max(a, acos(DOT3(v, n)));
        sphere_to_vector(v, cE, 0, 1.0); a = std::max(a, acos(DOT3(v, n)));
    }
    /* else HACK? */
    {
        sphere_to_vector(v, cW, cS, 1.0); a = std::max(a, acos(DOT3(v, n)));
        sphere_to_vector(v, cW, cN, 1.0); a = std::max(a, acos(DOT3(v, n)));
        sphere_to_vector(v, cE, cS, 1.0); a = std::max(a, acos(DOT3(v, n)));
        sphere_to_vector(v, cE, cN, 1.0); a = std::max(a, acos(DOT3(v, n)));
    }

    f = ((cE - cW) * (cN - cS)) / ((E - W) * (N - S));

    // Create subpages as necessary.

    P[0] = 0;
    P[1] = 0;
    P[2] = 0;
    P[3] = 0;

    if (d > 0)
    {
        const int X = (x + t / 2);
        const int Y = (y + t / 2);

        /* Has children? */ P[0] = new page(w, h, s, x, y, d - 1, c + 1, _W, _E, _S, _N);
        if (X < w         ) P[1] = new page(w, h, s, X, y, d - 1, c + 1, _W, _E, _S, _N);
        if (         Y < h) P[2] = new page(w, h, s, x, Y, d - 1, c + 1, _W, _E, _S, _N);
        if (X < w && Y < h) P[3] = new page(w, h, s, X, Y, d - 1, c + 1, _W, _E, _S, _N);
    }
}

uni::page::~page()
{
    if (P[3]) delete P[3];
    if (P[2]) delete P[2];
    if (P[1]) delete P[1];
    if (P[0]) delete P[0];
}

bool uni::page::visible(int frusc, const app::frustum *const *frusv,
                        double r0, double r1, int N)
{
    if (visible_cache_serial != N)
    {
        bool r = false;

        // Check each frustum in turn.

        for (int frusi = 0; !r && frusi < frusc; ++frusi)
        {
            // Loosely test the page against the frustum.

            if (frusv[frusi]->test_cap(n, a, r0, r1) >= 0)
            {
                // Tighten the test by testing children.
/*
                r = ((P[0] && P[0]->visible(F, r0, r1, N)) ||
                     (P[1] && P[1]->visible(F, r0, r1, N)) ||
                     (P[2] && P[2]->visible(F, r0, r1, N)) ||
                     (P[3] && P[3]->visible(F, r0, r1, N)));
*/
                r = true;
            }
        }

        // Cache the result.  We'll be back here soon.
        
        visible_cache_serial = N;
        visible_cache_result = r;
    }

    return visible_cache_result;
}

bool uni::page::needed(int frusc, const app::frustum *const *frusv,
                       double r0, double r1, int N, int s, double cutoff)
{
    if (c <= 2) return true; // HACK

    // A page is visible if there exists a frustum that intersects it
    // and where it has a texel/pixel ratio of less than the cutoff.

    if (visible(frusc, frusv, r0, r1, N))
    {
        const double *vp = frusv[0]->get_view_pos();
        const double  sa = angle(vp, r0);

        // HACK: should use the frustum that sees the page.

        if (c < 2 || s * s / frusv[0]->pixels(sa) < cutoff)
            return true;
    }
    return false;
}

/*
void uni::page::draw(double r0, double r1)
{
    double rr = (r0 + r1) * 0.5;

    static const GLfloat color[8][3] = {
        { 1.0f, 0.0f, 0.0f },
        { 1.0f, 0.5f, 0.0f },
        { 1.0f, 1.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f },
        { 0.0f, 1.0f, 1.0f },
        { 0.0f, 0.0f, 1.0f },
        { 1.0f, 0.0f, 1.0f },
        { 0.0f, 0.0f, 0.0f },
    };

    glColor3fv(color[d]);

    // Draw the cap bound.

    double M[16];

    load_idt(M);

    M[8]  = n[0];
    M[9]  = n[1];
    M[10] = n[2];

    crossprod(M + 4, M + 8, M + 0);
    normalize(M + 4);
    crossprod(M + 0, M + 4, M + 8);
    normalize(M + 0);

    for (int c = 0; c < 128; ++c)
    {
        glPushMatrix();
        {
            glMultMatrixd(M);
            glRotated(360.0 * c / 128.0, 0.0, 0.0, 1.0);
            glRotated(DEG(a),            1.0, 0.0, 0.0);

            glBegin(GL_POINTS);
            {
                glVertex3d(0.0, 0.0, rr);
            }
            glEnd();
        }
        glPopMatrix();
    }

    // Draw the border.

    glBegin(GL_LINE_LOOP);
    {
        const double cW = std::max(W, -PI);
        const double cE = std::min(E,  PI);
        const double cS = std::max(S, -PI_2);
        const double cN = std::min(N,  PI_2);

        double v[3], k = 0.01, o = 0.001;

        for (double x = cW + o; x < cE - o; x += k)
        {
            sphere_to_vector(v, x, cS + o, rr);
            glVertex3dv(v);
        }
        for (double x = cS + o; x < cN - o; x += k)
        {
            sphere_to_vector(v, cE - o, x, rr);
            glVertex3dv(v);
        }
        for (double x = cE - o; x > cW + o; x -= k)
        {
            sphere_to_vector(v, x, cN - o, rr);
            glVertex3dv(v);
        }
        for (double x = cN - o; x > cS + o; x -= k)
        {
            sphere_to_vector(v, cW + o, x, rr);
            glVertex3dv(v);
        }
    }
    glEnd();
}
*/

double uni::page::angle(const double *v, double r)
{
    double aa = 0.0;

    // Leaves return zero because they can't be subdivided anyway.

    if (d > 0)
    {
        double D, R;

        // If a page covers most of the sphere, just use the sphere itself.

        if (a > PI_2)
        {
            D = sqrt(DOT3(v, v));
            R = r;
        }
        else
        {
            double p[3];

            p[0] = n[0] * r - v[0];
            p[1] = n[1] * r - v[1];
            p[2] = n[2] * r - v[2];

            D = sqrt(DOT3(p, p));
            R = tan(a) * r;
        }

        // Compute the solid angle of the given radius and distance.

        aa = 2.0 * PI * (1.0 - cos(atan(R / D)));
    }

    return aa / f;
}

//-----------------------------------------------------------------------------

uni::geomap::geomap(geocsh *cache, std::string name, double r0, double r1) :
    ext_W(-PI),
    ext_E( PI),
    ext_S(-PI_2),
    ext_N( PI_2),
    r0(r0),
    r1(r1),
    s(0),
    S(0),
    D(0),
    mip_w(1),
    mip_h(1),
    dirty(false),
    lsb(false),
    root(0),
    cache(cache),
    image(0),
    index(0)
{
    app::file file(name.c_str());
    
    if (app::node n = file.get_root().find("map"))
    {
        // Load the map configuration from the file.

        const std::string program = n.get_s("program");

        pattern = ::conf->get_s("data_dir") + n.get_s("name");

        ext_W = n.get_f("W", -PI);
        ext_E = n.get_f("E",  PI);
        ext_S = n.get_f("S", -PI_2);
        ext_N = n.get_f("N",  PI_2);

        w     = n.get_i("w", 1024);
        h     = n.get_i("h", 512);
        s     = n.get_i("s", 510);
        c     = n.get_i("c", 3);
        b     = n.get_i("b", 1);

        lsb = (n.get_s("order") == "lsb");

        // Compute the extents of the mipmap pyramid.

        mip_w = 2 * next_power_of_2(std::max(w / s, 1));
        mip_h = 2 * next_power_of_2(std::max(h / s, 1));

        int t = s;

        while (t < w || t < h)
        {
            t *= 2;
            D += 1;
        }

        // Generate the mipmap pyramid catalog.

        root = new page(w, h, s, 0, 0, D, 0, ext_W, ext_E, ext_S, ext_N);

        // Load and initialize the shader.

        prog = ::glob->load_program(program);
    }

    cutoff = ::conf->get_f("texel_cutoff");

    S = s + 2;

    index = ::glob->new_image(mip_w, mip_h,
                              GL_TEXTURE_2D, GL_LUMINANCE16,
                              GL_LUMINANCE, GL_UNSIGNED_SHORT);

    buffer = new ogl::buffer(GL_PIXEL_UNPACK_BUFFER_ARB, mip_w * mip_h * 2);

    buffer->bind();
    {
        image = (GLushort *) buffer->amap();
    }
    buffer->free();
    
    if (image) memset(image, 0xFF, mip_w * mip_h * 2);
}

uni::geomap::~geomap()
{
    if (buffer) delete buffer;

    if (index) ::glob->free_image(index);

    if (prog) ::glob->free_program(prog);

    if (root) delete root;
}

//-----------------------------------------------------------------------------

GLushort& uni::geomap::index_x(int d, int i, int j)
{
    int dj = mip_w >> (d + 1);
    int di = mip_h >> (d + 1);

    int jj = j + mip_w - (dj << 1);
    int ii = i + mip_h - (di << 1);

    return image[(ii + di) * mip_w + (jj     )];
}

GLushort& uni::geomap::index_y(int d, int i, int j)
{
    int dj = mip_w >> (d + 1);
    int di = mip_h >> (d + 1);

    int jj = j + mip_w - (dj << 1);
    int ii = i + mip_h - (di << 1);

    return image[(ii     ) * mip_w + (jj     )];
}

GLushort& uni::geomap::index_l(int d, int i, int j)
{
    int dj = mip_w >> (d + 1);
    int di = mip_h >> (d + 1);

    int jj = j + mip_w - (dj << 1);
    int ii = i + mip_h - (di << 1);

    return image[(ii     ) * mip_w + (jj + dj)];
}

//-----------------------------------------------------------------------------

void uni::geomap::do_index(int d, int i, int j,
                           GLushort x, GLushort y, GLushort l)
{
    if (d >= 0)
    {
        int j2 = j << 1;
        int i2 = i << 1;

        if (index_l(d, i, j) > l)
        {
            index_l(d, i, j) = l;
            index_x(d, i, j) = x;
            index_y(d, i, j) = y;
        }

        do_index(d - 1, i2 + 0, j2 + 0, x, y, l);
        do_index(d - 1, i2 + 1, j2 + 0, x, y, l);
        do_index(d - 1, i2 + 0, j2 + 1, x, y, l);
        do_index(d - 1, i2 + 1, j2 + 1, x, y, l);
    }
}

void uni::geomap::do_eject(int d, int i, int j,
                           GLushort x0, GLushort y0, GLushort l0,
                           GLushort x1, GLushort y1, GLushort l1)
{
    if (d >= 0)
    {
        int j2 = j << 1;
        int i2 = i << 1;

        if (index_x(d, i, j) == x0 &&
            index_y(d, i, j) == y0 &&
            index_l(d, i, j) == l0)
        {
            index_x(d, i, j) = x1;
            index_y(d, i, j) = y1;
            index_l(d, i, j) = l1;
        }

        do_eject(d - 1, i2 + 0, j2 + 0, x0, y0, l0, x1, y1, l1);
        do_eject(d - 1, i2 + 1, j2 + 0, x0, y0, l0, x1, y1, l1);
        do_eject(d - 1, i2 + 0, j2 + 1, x0, y0, l0, x1, y1, l1);
        do_eject(d - 1, i2 + 1, j2 + 1, x0, y0, l0, x1, y1, l1);
    }
}

void uni::geomap::cache_page(const page *Q, int x, int y)
{
    int d = Q->get_d();
    int i = Q->get_i();
    int j = Q->get_j();

    int l = 1 << d;

    if (d < D)
    {
        if (image == 0)
        { 
            buffer->bind();
            image = (GLushort *) buffer->amap();
            buffer->free();
        }
        if (image)
        {
            do_index(d, i, j, GLushort(x * S + 1),
                              GLushort(y * S + 1),
                              GLushort(l));
            dirty = true;
        }
    }
}

void uni::geomap::eject_page(const page *Q, int x, int y)
{
    int d = Q->get_d();
    int i = Q->get_i();
    int j = Q->get_j();

    int l = 1 << d;

    if (d < D)
    {
        if (image == 0)
        {
            buffer->bind();
            image = (GLushort *) buffer->amap();
            buffer->free();
        }
        if (image)
        {
            GLushort x1 = (d < D-1) ? index_x(d + 1, i >> 1, j >> 1) : 0xFFFF;
            GLushort y1 = (d < D-1) ? index_y(d + 1, i >> 1, j >> 1) : 0xFFFF;
            GLushort l1 = (d < D-1) ? index_l(d + 1, i >> 1, j >> 1) : 0xFFFF;

            do_eject(d, i, j, GLushort(x * S + 1),
                              GLushort(y * S + 1),
                              GLushort(l), x1, y1, l1);
            dirty = true;
        }
    }
}

//-----------------------------------------------------------------------------

void uni::geomap::view_page(page *P,
                            int frusc, const app::frustum *const *frusv,
                            double r0, double r1, int N)
{
    page *C;

    // If this page is visible...

    if (P->needed(frusc, frusv, r0, r1, N, s, cutoff))
    {
        // Load it or ping it.

        switch(P->get_state())
        {
        case page_default:
            cache->add_needed(this, P);
            break;

        case page_cached:
            cache->use_needed(this, P);
            break;

        default:
            break;
        }

        // Whether cached, loading or missing, check the children.

        if ((C = P->child(0))) view_page(C, frusc, frusv, r0, r1, N);
        if ((C = P->child(1))) view_page(C, frusc, frusv, r0, r1, N);
        if ((C = P->child(2))) view_page(C, frusc, frusv, r0, r1, N);
        if ((C = P->child(3))) view_page(C, frusc, frusv, r0, r1, N);
    }
}

void uni::geomap::view(int frusc, const app::frustum *const *frusv,
                       double r0, double r1, int N)
{
    view_page(root, frusc, frusv, r0, r1, N);
}

//-----------------------------------------------------------------------------

void uni::geomap::proc()
{
    if (dirty)
    {
        buffer->bind();
        buffer->umap();
        {
            index->blit(0, 0, 0, mip_w, mip_h);
        }
        buffer->free();

        image = 0;
    }

    dirty = false;
}

void uni::geomap::draw(double sea, const double *ori) const
{
    prog->bind();
    index->bind(GL_TEXTURE1);
    cache->bind(GL_TEXTURE2);
    {
        // Initialize the uniforms.

        prog->uniform("index", 1);
        prog->uniform("cache", 2);

        prog->uniform("data_size", w, h);
        prog->uniform("tile_size", s, s);
        prog->uniform("page_size", S, S);
        prog->uniform("pool_size", cache->pool_w(), cache->pool_h());
        prog->uniform("over_page", 1.0 / S, 1.0 / S);
        prog->uniform("over_pool", 1.0 / cache->pool_w(),
                                   1.0 / cache->pool_h());

        prog->uniform("data_over_tile",      double(    w) / (        s),
                                             double(    h) / (        s));
        prog->uniform("data_over_tile_base", double(2 * w) / (mip_w * s),
                                             double(2 * h) / (mip_h * s));

        prog->uniform("cylk",    1.0 / (ext_E - ext_W),
                                 1.0 / (ext_N - ext_S));
        prog->uniform("cyld", -ext_W / (ext_E - ext_W),
                              -ext_S / (ext_N - ext_S));

        prog->uniform("minL",     0.0);
        prog->uniform("maxL", D - 1.0);

        // HACK!

        if (ori)
        {
            prog->uniform("sea", sea);
            prog->uniform("ori", ori[0], ori[1], ori[2]);
        }

        // Render a full-screen quad.

        glMatrixMode(GL_PROJECTION);
        {
            glPushMatrix();
            glLoadIdentity();
        }
        glMatrixMode(GL_MODELVIEW);
        {
            glPushMatrix();
            glLoadIdentity();
        }

        glRecti(-1, -1, +1, +1);

        glMatrixMode(GL_PROJECTION);
        {
            glPopMatrix();
        }
        glMatrixMode(GL_MODELVIEW);
        {
            glPopMatrix();
        }
    }
    cache->free(GL_TEXTURE2);
    index->free(GL_TEXTURE1);
    prog->free();
}

std::string uni::geomap::name(const page *P)
{
    char str[256];

    // Construct the file name.

    sprintf(str, pattern.c_str(), P->get_d(), P->get_i(), P->get_j());

    return std::string(str);
}

//-----------------------------------------------------------------------------