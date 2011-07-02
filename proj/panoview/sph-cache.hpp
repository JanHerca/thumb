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

#ifndef SPH_CACHE_HPP
#define SPH_CACHE_HPP

#include <vector>
#include <string>
#include <list>
#include <set>

#include <GL/glew.h>
#include <tiffio.h>
#include <SDL.h>
#include <SDL_thread.h>

#include "queue.hpp"

//------------------------------------------------------------------------------

template <typename T> class fifo : public std::list <T>
{
public:

    void enq(T p) {
        this->push_front(p);
    }
    
    T deq() {
        T p = this->front();
        this->pop_front();
        return p;
    }
};

//------------------------------------------------------------------------------

struct sph_item
{
    sph_item()             : f(-1), i(-1) { }
    sph_item(int f, int i) : f( f), i( i) { }

    int f;
    int i;
    
    bool operator<(const sph_item& that) const {
        if (i == that.i)
            return f < that.f;
        else
            return i < that.i;
    }
};

//------------------------------------------------------------------------------

struct sph_page : public sph_item
{
    sph_page()             : sph_item(    ), o(0), new_t(0), use_t(0) { }
    sph_page(int f, int i) : sph_item(f, i), o(0), new_t(0), use_t(0) { }
    sph_page(int f, int i, GLuint o, int n, int u);

    GLuint o;
    int new_t;
    int use_t;
};

//------------------------------------------------------------------------------

struct sph_task : public sph_item
{
    sph_task()             : sph_item(    ), u(0), p(0) { }
    sph_task(int f, int i) : sph_item(i, f), u(0), p(0) { }
    sph_task(int f, int i, GLuint u, GLsizei s);
    
    GLuint u;
    void  *p;
    
    void make_texture(GLuint, uint32, uint32, uint16, uint16);
    void load_texture(TIFF *, uint32, uint32, uint16, uint16);
    void dump_texture();
};

//------------------------------------------------------------------------------

struct sph_file
{
    sph_file(const std::string& name);
    
    std::string name;
    uint32 w, h;
    uint16 c, b;
};

//------------------------------------------------------------------------------

class sph_set
{
public:

    sph_set(int size) : size(size) { }

    bool full()  const { return (s.size() >= size); }
    bool empty() const { return (s.empty()); }

    void   insert(sph_page);
    void   remove(sph_page);
    GLuint search(sph_page, int&, int);
    GLuint eject();
    void   draw();
    
private:

    std::set<sph_page> s;

    int size;
};

//------------------------------------------------------------------------------

class sph_cache
{
public:

    sph_cache(int);
   ~sph_cache();

    int    add_file(const std::string&);
    GLuint get_page(int, int, int&, int);
    GLuint get_fill() { return filler; }
    
    void update(int);

    void draw() { pages.draw(); }

private:

    std::vector<sph_file> files;

    sph_set pages;
    sph_set waits;
    
    queue<sph_task> needs;
    queue<sph_task> loads;

    fifo<GLuint> pbos;
    fifo<GLuint> texs;
        
    GLsizei pagelen(int);
    GLuint  filler;
    
    SDL_Thread *thread[4];
    
    friend int loader(void *);
};

//------------------------------------------------------------------------------

#endif
