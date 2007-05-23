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

#include "mesh.hpp"
#include "glob.hpp"
#include "matrix.hpp"
#include "opengl.hpp"
#include "batcher.hpp"

//=============================================================================
// Batchable element

ogl::element::element(bool& dirty, std::string name) : dirty(dirty)
{
    // Build the vert_array map using the named surface...

    dirty = true;
}

ogl::element::~element()
{
    dirty = true;
}

void ogl::element::move(const GLfloat *T)
{
    // Cache the current transform.

    load_mat(M, T);

    // If the vertex array can be updated in place, do so.

    if (dirty == false)
    {
        vert_m::iterator i;

        for (i = vert_array.begin(); i != vert_erray.end(); ++i)
            (void) i->first->vert_cache(i->second, M);
    }
}

//-----------------------------------------------------------------------------

GLsizei ogl::element::vcount() const
{
    // Sum all vertex counts.

    GLsizei vc = 0;

    for (vert_m::iterator i = vert_array.begin(); i != vert_erray.end(); ++i)
        vc += i->first->vert_count();

    return vc;
}

GLsizei ogl::element::ecount() const
{
    // Sum all element counts.

    GLsizei ec = 0;

    for (vert_m::iterator i = vert_array.begin(); i != vert_erray.end(); ++i)
        ec += i->first->fce_count();

    return ec;
}

//-----------------------------------------------------------------------------

void ogl::element::enlist(element_s& opaque,
                          element_s& transp)
{
    // Insert each mesh into the proper opacity set.

    for (vert_m::iterator i = vert_array.begin(); i != vert_erray.end(); ++i)
        if (i->first->state()->opaque())
            opaque.insert(i->first, this);
        else
            transp.insert(i->first, this);
}

vert *ogl::element::vert_cache(const mesh *m, vert *v)
{
    // Update the vertex array pointer for the given mesh.

    vert_array[m] = v;

    // Pretransform the mesh to the given vertex array.

    return m->vert_cache(v, M);
}

//=============================================================================
// Batch segment

ogl::segment::segment(bool& dirty) : dirty(dirty)
{
    dirty = true;
}

ogl::segment::~segment()
{
    dirty = true;

    for (element_set::iterator i = elements.begin(); i != elements.end(); ++i)
        delete (*i);
}

void ogl::segment::move(const GLfloat *T)
{
    load_mat(M, T);
}

//-----------------------------------------------------------------------------

ogl::element *ogl::segment::add(std::string name)
{
    element *elt = new element(dirty, name);
    elements.insert(elt);
    return elt;
}

void ogl::segment::del(ogl::element *elt)
{
    elements.erase(elt);
    delete elt;
}

//-----------------------------------------------------------------------------

GLsizei ogl::segment::vcount() const
{
    // Sum all vertex counts.

    GLsizei vc = 0;

    for (element_set::iterator i = elements.begin(); i != elements.end(); ++i)
        vc += (*i)->vcount();

    return vc;
}

GLsizei ogl::segment::ecount() const
{
    // Sum all element counts.

    GLsizei ec = 0;

    for (element_set::iterator i = elements.begin(); i != elements.end(); ++i)
        ec += (*i)->ecount();

    return ec;
}

//-----------------------------------------------------------------------------

void ogl::segment::reduce(vert_p v0, vert_p& v,
                          face_p f0, face_p& f,
                          element_m& meshes, batch_v& batches)
{
    element_m::iterator i;

    batch_v tmp;

    // Set up the vertex and element arrays and batches for this segment.

    for (i = meshes.begin(); i != meshes.end(); ++i)
    {
        // Create a new batch for this mesh.

        tmp.push_back(batch(i->first->state(), f - f0,
                            i->first->face_count()));

        // Copy the face elements and vertices.

        GLuint d = GLuint(v - v0);

        v = i->second->vert_cache(i->first, v);
        f = i->first ->face_cache(f, d);
    }

    // Merge adjacent batches that share the same state binding.

    if (!tmp.empty())
    {
        batches.push_back(tmp.front());

        for (i = tmp.begin() + 1; i != tmp.end(); ++i)
            if (batches.back().bnd == i->bnd)
                batches.back().num += i->num;
            else
                batches.push_back(*i);
    }

    // Scan for the minimum and maximum elements of each batch.

    for (i = batches.begin(); i != batches.end(); ++i)
    {
        i->min = std::numeric_limits<GLuint>::max();
        i->max = std::numeric_limits<GLuint>::min();

        for (GLsizei j = 0; j < i->num; ++j)
        {
            i->min = std::min(i->min, i->ptr[j]);
            i->max = std::max(i->max, i->ptr[j]);
        }
    }
}

void ogl::segment::enlist(vert_p v0, vert_p& v,
                          face_p f0, face_p& f)
{
    std::map<const mesh_p, element *> opaque;
    std::map<const mesh_p, element *> transp;

    // Sort all meshes.

    for (element_set::iterator i = elements.begin(); i != elements.end(); ++i)
        enlist(opaque, transp);

    // Batch all meshes.

    reduce(opaque, opaque_batch, v0, v, f0, f);
    reduce(transp, transp_batch, v0, v, f0, f);
}

void ogl::segment::draw_opaque(bool lit) const
{
    // Test the bounding box.

    // Draw all opaque meshes.
}

void ogl::segment::draw_transp(bool lit) const
{
    // Test the bounding box.

    // Draw all transp meshes.
}

//=============================================================================
// Batch manager

ogl::batcher::batcher() : vbo(0), ebo(0), dirty(true)
{
    glGenBuffersARB(1, &vbo);
    glGenBuffersARB(1, &ebo);
}

ogl::batcher::~batcher()
{
    for (segment_set::iterator i = segments.begin(); i != segments.end(); ++i)
        delete (*i);
    
    if (ebo) glDeleteBuffersARB(1, &ebo);
    if (vbo) glDeleteBuffersARB(1, &vbo);
}

//-----------------------------------------------------------------------------

ogl::segment *ogl::batcher::add()
{
    segment *seg = new segment(dirty);
    segments.insert(seg);
    return seg;
}

void ogl::batcher::del(ogl::segment *seg)
{
    segments.erase(seg);
    delete seg;
}

//-----------------------------------------------------------------------------

void ogl::batcher::clean()
{
    // Sum all vertex array and element array sizes.

    GLsizei vsz = 0;
    GLsizei esz = 0;

    for (segment_set::iterator i = segments.begin(); i != segments.end(); ++i)
    {
        vsz += (*i)->vcount() * sizeof (vert);
        esz += (*i)->fcount() * sizeof (face);
    }

    // Initialize vertex and element buffers of that size.

    glBindBufferARB(GL_ARRAY_BUFFER_ARB,         vbo);
    glBufferDataARB(GL_ARRAY_BUFFER_ARB,         vsz, 0, GL_STATIC_DRAW_ARB);

    glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, ebo);
    glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, esz, 0, GL_STATIC_DRAW_ARB);

    // Let all segments copy to the buffers.

    face *f0, *f;
    vert *v0, *v;

    f0 = f = face_p(glMapBufferARB(GL_ARRAY_BUFFER_ARB,        GL_WRITE_ONLY));
    v0 = v = vert_p(glMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,GL_WRITE_ONLY));

    for (segment_set::iterator i = segments.begin(); i != segments.end(); ++i)
        (*i)->enlist(v0, v, f0, f);

    glUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB);
    glBindBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, 0);

    glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
    glBindBufferARB (GL_ARRAY_BUFFER_ARB, 0);

    // Mark the buffers as clean.

    dirty = false;
}

//-----------------------------------------------------------------------------

void ogl::batcher::draw_init()
{
    // Update the VBO and EBO as necessary.

    if (dirty) clean();

    // Bind the VBO and EBO.

    GLsizei s = sizeof (vert);

    glEnableVertexAttribArrayARB(6);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo);

    glTexCoordPointer       (   2, GL_FLOAT,    s, (GLubyte *) 36);
    glVertexAttribPointerARB(6, 3, GL_FLOAT, 0, s, (GLubyte *) 24);
    glNormalPointer         (      GL_FLOAT,    s, (GLubyte *) 12);
    glVertexPointer         (   3, GL_FLOAT,    s, (GLubyte *)  0);

    glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, ebo);
}

void ogl::batcher::draw_fini()
{
    // Unbind the VBO and EBO.

    glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB,         0);
}

void ogl::batcher::draw_opaque(bool lit) const
{
    // Opaque draw all segments.

    for (segment_set::iterator i = segments.begin(); i != segments.end(); ++i)
        (*i)->draw_opaque(lit);
}

void ogl::batcher::draw_transp(bool lit) const
{
    // Transp draw all segments.

    for (segment_set::iterator i = segments.begin(); i != segments.end(); ++i)
        (*i)->draw_transp(lit);
}

//=============================================================================
