// SPDX-License-Identifier: LGPL-2.0-or-later

/***************************************************************************
 *   Copyright (c) 2024 wandererfan <wandererfan at gmail dot com>         *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

//! a class to deal with geometry handlers.  Performs gets, puts, deletes, etc 
//! for clients.
//! based on ConsoleSingleton in Base/Console.cpp

#include <cassert>

#include "HandlerManager.h"

using namespace Measure;

//**************************************************************************
// Construction destruction

HandlerManagerSingleton::HandlerManagerSingleton() = default;

HandlerManagerSingleton::~HandlerManagerSingleton()
{
    HandlerManagerSingleton::Destruct();
}

//**************************************************************************
// Singleton stuff

// TODO: this has global scope as written? do we need static here??  is
// static & private on the member enough? IDU 
HandlerManagerSingleton* HandlerManagerSingleton::_pcSingleton = nullptr;

HandlerManagerSingleton& HandlerManagerSingleton::Instance()
{
    // not initialized?
    if (!_pcSingleton) {
        _pcSingleton = new HandlerManagerSingleton();
    }
    return *_pcSingleton;
}

void HandlerManagerSingleton::Destruct()
{
    // not initialized or double destructed!
    // TODO: isn't assert only for testing? should throw an error instead?
    assert(_pcSingleton);
    delete _pcSingleton;
    _pcSingleton = nullptr;
}
