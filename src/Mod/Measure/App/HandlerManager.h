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
//! based on ConsoleSingleton in Base/Console.h

#ifndef MEASURE_HANDLERMANAGER_H
#define MEASURE_HANDLERMANAGER_H

#include <Mod/Measure/MeasureGlobal.h>

namespace Measure
{

class MeasureExport HandlerManagerSingleton
{
public:
    HandlerManagerSingleton();
    ~HandlerManagerSingleton();

    // get the HandlerManager (1 and only)
    static HandlerManagerSingleton& Instance();
  
private:
    void Destruct();

    // there is only ever 1 _pcSingleton.  Any HandlerManagerSingleton (not
    // allowed!) would use the same _pcSingleton.
    static HandlerManagerSingleton* _pcSingleton;  // NOLINT
};


// This method is used to gain access to the one and only instance of
// the HandlerManagerSingleton class.
inline HandlerManagerSingleton& HandlerManager()
{
    return HandlerManagerSingleton::Instance();
}

} // namspace Measure

#endif
