//  Copyright (C) 2007-2011 Robert Kooima
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

#include <cstring>
#include <cstdlib>
#include <iostream>
#include <algorithm>

#include <mxml.h>

#include <etc-log.hpp>
#include <app-file.hpp>
#include <app-data.hpp>

//-----------------------------------------------------------------------------

// The full compliment of constructor and assignment operators is necessary
// to ensure consistent reference counting by mini-XML.

app::node::node(mxml_node_t *p) : ptr(p)
{
    if (ptr) mxmlRetain(ptr);
}

app::node::node(const std::string& tag) :
    ptr(mxmlNewElement(MXML_NO_PARENT, tag.c_str()))
{
}

app::node::node(const app::node& that) : ptr(0)
{
    if (that.ptr)
    {
        ptr = that.ptr;
        mxmlRetain(ptr);
    }
}

app::node::~node()
{
    if (ptr) mxmlRelease(ptr);
}

app::node& app::node::operator=(const app::node &that)
{
    if (this != &that)
    {
        ptr = that.ptr;
        if (ptr) mxmlRetain(ptr);
    }
    return *this;
}

//-----------------------------------------------------------------------------

void app::node::dirty()
{
    for (mxml_node_t *itr = ptr; itr; itr = itr->parent)
        itr->user_data = (void *) 1;
}

void app::node::clean()
{
    if (ptr)
        ptr->user_data = (void *) 0;
}

//-----------------------------------------------------------------------------

static const char *save_cb(mxml_node_t *, int);

void app::node::read(const std::string& name)
{
    // Release any previously loaded DOM.

    if (ptr) mxmlRelease(ptr);

    ptr = 0;

    if (!name.empty())
    {
        // Load the data stream and parse the XML.

        try
        {
            const char *buff = (const char *) ::data->load(name);

            ptr = mxmlLoadString(0, buff, MXML_TEXT_CALLBACK);
            clean();

            ::data->free(name);
        }
        catch (std::runtime_error& e)
        {
            etc::log(e.what());
        }
    }
}

void app::node::write(const std::string& name)
{
    // If dirty, write the XML to a stream and store the stream.

    if (ptr && ptr->user_data)
    {
        if (char *buff = mxmlSaveAllocString(ptr, save_cb))
        {
            try
            {
                ::data->save(name, buff);
            }
            catch (std::runtime_error& e)
            {
                etc::log(e.what());
            }
            free(buff);
        }
        clean();
    }
}

//-----------------------------------------------------------------------------

const char *app::node::get(const std::string& name) const
{
    // If this node has the named entity, return its string.

    if (ptr && !name.empty())
        return mxmlElementGetAttr(ptr, name.c_str());
    else
        return 0;
}

const char *app::node::get() const
{
    // If this node has a text child, return its string.

    if (ptr && ptr->child && ptr->child->type == MXML_TEXT)
        return ptr->child->value.text.string;
    else
        return 0;
}

//-----------------------------------------------------------------------------

void app::node::set_i(const std::string& name, int i)
{
    if (ptr && !name.empty())
    {
        // Store the integer attribute as text.

        mxmlElementSetAttrf(ptr, name.c_str(), "%d", i);
        dirty();
    }
}

void app::node::set_i(int i)
{
    if (ptr)
    {
        while (ptr->child) mxmlRelease(ptr->child);

        // Store the integer as a text element.

        mxmlNewTextf(ptr, 0, "%d", i);
        dirty();
    }
}

int app::node::get_i(const std::string& name, int i) const
{
    if (const char *c = get(name))
        return int(strtol(c, 0, 0));
    else
        return i;
}

int app::node::get_i(int i) const
{
    if (const char *c = get())
        return int(strtol(c, 0, 0));
    else
        return i;
}

//-----------------------------------------------------------------------------

void app::node::set_f(const std::string& name, double f)
{
    if (ptr && !name.empty())
    {
        // Store the double attribute as text.

        mxmlElementSetAttrf(ptr, name.c_str(), "%f", f);
        dirty();
    }
}

void app::node::set_f(double f)
{
    if (ptr)
    {
        while (ptr->child) mxmlRelease(ptr->child);

        // Store the double as a text element.

        mxmlNewTextf(ptr, 0, "%f", f);
        dirty();
    }
}

double app::node::get_f(const std::string& name, double f) const
{
    if (const char *c = get(name))
        return strtod(c, 0);
    else
        return f;
}

double app::node::get_f(double f) const
{
    if (const char *c = get())
        return strtod(c, 0);
    else
        return f;
}

//-----------------------------------------------------------------------------

void app::node::set_s(const std::string& name,
                      const std::string& s)
{
    if (ptr && !name.empty())
    {
        // Store the text attribute.

        mxmlElementSetAttr(ptr, name.c_str(), s.empty() ? 0 : s.c_str());
        dirty();
    }
}

void app::node::set_s(const std::string& s)
{
    if (ptr)
    {
        while (ptr->child) mxmlRelease(ptr->child);

        // Store the integer as a text element.

        mxmlNewText(ptr, 0, s.c_str());
        dirty();
    }
}

std::string app::node::get_s(const std::string& name) const
{
    if (const char *c = get(name))
        return std::string(c);
    else
        return "";
}

std::string app::node::get_s() const
{
    if (const char *c = get())
        return std::string(c);
    else
        return "";
}

//-----------------------------------------------------------------------------

app::node app::node::find(const std::string& name,
                          const std::string& attr,
                          const std::string& data) const
{
    if (ptr)
        return app::node(mxmlFindElement(ptr, ptr,
                                         name.empty() ? 0 : name.c_str(),
                                         attr.empty() ? 0 : attr.c_str(),
                                         data.empty() ? 0 : data.c_str(),
                                         MXML_DESCEND));
    else
        return 0;
}

app::node app::node::next(app::node& itr,
                          const std::string& name,
                          const std::string& attr,
                          const std::string& data) const
{
    if (ptr && itr.ptr)
        return app::node(mxmlFindElement(itr.ptr, ptr,
                                         name.empty() ? 0 : name.c_str(),
                                         attr.empty() ? 0 : attr.c_str(),
                                         data.empty() ? 0 : data.c_str(),
                                         MXML_NO_DESCEND));
    else
        return 0;
}

//-----------------------------------------------------------------------------

void app::node::insert(app::node& parent)
{
    if (parent.ptr && ptr)
    {
        mxmlRetain(ptr);
        mxmlAdd(parent.ptr, MXML_ADD_AFTER, 0, ptr);
        dirty();
    }
}

void app::node::remove()
{
    if (ptr)
    {
        dirty();
        mxmlRemove(ptr);
        mxmlRelease(ptr);
    }
}

//-----------------------------------------------------------------------------

app::file::file(const std::string& name) :
    name(name),
    root(0)
{
    root.read(name);
}

app::file::~file()
{
    root.write(name);
}

//-----------------------------------------------------------------------------

double scale_to_meters(const std::string& unit)
{
    // Return a scaling factor to convert the named unit to meters.

    if (unit == "mi") return 5280.0 * 12.0 * 2.54 / 100.0;
    if (unit == "yd") return    3.0 * 12.0 * 2.54 / 100.0;
    if (unit == "ft") return          12.0 * 2.54 / 100.0;
    if (unit == "in") return                 2.54 / 100.0;

    if (unit == "km") return 1000.000;
    if (unit == "cm") return    0.010;
    if (unit == "mm") return    0.001;

    return 1.0;
}

//-----------------------------------------------------------------------------

static bool onechild(mxml_node_t *node)
{
    // Return true if a node is not the root and has EXACTLY one child.

    return (node->type   == MXML_ELEMENT &&
            node->parent &&
            node->child  &&
            node->child  == node->last_child);
}

static const char *save_cb(mxml_node_t *node, int where)
{
    const char *space  = "                                                   "
                         "                                                   "
                         "                                                   ";

    if (node->type == MXML_ELEMENT)
    {
        // Find a string giving proper indentation for this node.

        const char *indent = space + strlen(space);

        for (mxml_node_t *n = node->parent; n; n = n->parent)
            indent = std::max(space, indent - 2);

        // Return the proper whitespace for location of this node.

        switch (where)
        {
        case MXML_WS_BEFORE_OPEN:  return                      indent;
        case MXML_WS_AFTER_OPEN:   return onechild(node) ? 0 : "\n";
        case MXML_WS_BEFORE_CLOSE: return onechild(node) ? 0 : indent;
        case MXML_WS_AFTER_CLOSE:  return                      "\n";
        }
    }
    return 0;
}

//-----------------------------------------------------------------------------
