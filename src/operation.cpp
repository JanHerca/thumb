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

#include <iostream>

#include "operation.hpp"
#include "matrix.hpp"
#include "world.hpp"

//-----------------------------------------------------------------------------

// This atom set remains empty at all times. It is used as a return value
// when an undo or redo should cause the selection to be cleared.

namespace ops
{
    static wrl::atom_set deselection;
}

//-----------------------------------------------------------------------------

ops::create_op::create_op(wrl::atom_set& S) : operation(S)
{
}

ops::create_op::~create_op()
{
    // An undone create operation contains the only reference to the created
    // atoms. If this operation is deleted before being redone then these
    // atoms become abandoned and should be deleted.

    wrl::atom_set::iterator i;

    if (done == false)
        for (i = selection.begin(); i != selection.end(); ++i)
            delete (*i);
}

wrl::atom_set& ops::create_op::undo(wrl::world *w)
{
    w->delete_set(selection);
    done = false;
    return deselection;
}

wrl::atom_set& ops::create_op::redo(wrl::world *w)
{
    w->create_set(selection);
    done = true;
    return selection;
}

//-----------------------------------------------------------------------------

ops::delete_op::delete_op(wrl::atom_set& S) : operation(S)
{
}

ops::delete_op::~delete_op()
{
    // A non-undone delete operation contains the only reference to the deleted
    // atom. If this operation is deleted before being undone then these atoms
    // become abandoned and should be deleted.

    wrl::atom_set::iterator i;

    if (done == true)
        for (i = selection.begin(); i != selection.end(); ++i)
            delete (*i);
}

wrl::atom_set& ops::delete_op::undo(wrl::world *w)
{
    w->create_set(selection);
    done = false;
    return selection;
}

wrl::atom_set& ops::delete_op::redo(wrl::world *w)
{
    w->delete_set(selection);
    done = true;
    return deselection;
}

//-----------------------------------------------------------------------------

ops::modify_op::modify_op(wrl::atom_set& S, const float M[16]) : operation(S)
{
    load_mat(T, M);
    load_inv(I, M);
}

wrl::atom_set& ops::modify_op::undo(wrl::world *w)
{
    w->modify_set(selection, I, T);
    done = false;
    return selection;
}

wrl::atom_set& ops::modify_op::redo(wrl::world *w)
{
    w->modify_set(selection, T, I);
    done = true;
    return selection;
}

//-----------------------------------------------------------------------------

ops::embody_op::embody_op(wrl::atom_set& S, int id) : operation(S), new_id(id)
{
    wrl::atom_set::iterator i;

    // Store the previous body IDs of all selected atoms.

    for (i = selection.begin(); i != selection.end(); ++i)
        old_id[*i] = (*i)->body();
}

wrl::atom_set& ops::embody_op::undo(wrl::world *)
{
    wrl::atom_set::iterator i;

    // Reassign the previous body ID to each selected atom.

    for (i = selection.begin(); i != selection.end(); ++i)
        (*i)->body(old_id[*i]);

    done = false;
    return selection;
}

wrl::atom_set& ops::embody_op::redo(wrl::world *)
{
    wrl::atom_set::iterator i;

    // Assign the new body ID to each selected atom.

    for (i = selection.begin(); i != selection.end(); ++i)
        (*i)->body(new_id);

    done = true;
    return selection;
}

//-----------------------------------------------------------------------------

ops::enjoin_op::enjoin_op(wrl::atom_set& S) : operation(S), one_id(0),two_id(0)
{
    wrl::atom_set::iterator i;

    // Store the previous join IDs of all selected atoms.

    for (i = selection.begin(); i != selection.end(); ++i)
        old_id[*i] = (*i)->join();

    // Scan the selection for potential joint targets.

    std::set<int> ids;

    for (i = selection.begin(); i != selection.end(); ++i)
        ids.insert((*i)->body());

    // Select an arbitrary two, non-zero if possible.

    std::set<int>::reverse_iterator j = ids.rbegin();

    if (j != ids.rend()) one_id = *(j++);
    if (j != ids.rend()) two_id = *(j++);
}

wrl::atom_set& ops::enjoin_op::undo(wrl::world *)
{
    wrl::atom_set::iterator i;

    // Reassign the previous join ID to each selected atom.

    for (i = selection.begin(); i != selection.end(); ++i)
        (*i)->join(old_id[*i]);

    done = false;
    return selection;
}

wrl::atom_set& ops::enjoin_op::redo(wrl::world *)
{
    wrl::atom_set::iterator i;

    // Assign the new join ID to each selected atom.

    for (i = selection.begin(); i != selection.end(); ++i)
    {
        if ((*i)->body() == one_id) (*i)->join(two_id);
        if ((*i)->body() == two_id) (*i)->join(one_id);
    }

    done = true;
    return selection;
}

//-----------------------------------------------------------------------------
