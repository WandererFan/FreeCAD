/***************************************************************************
 *   Copyright (c) 2022 WandererFan <wandererfan@gmail.com>                *
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

#include "PreCompiled.h"

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/GeometryPyCXX.h>
#include <Base/PyObjectBase.h>
#include <Base/Vector3D.h>
#include <Base/VectorPy.h>

#include <Mod/TechDraw/App/DrawBrokenViewPy.h>
#include <Mod/TechDraw/App/DrawBrokenViewPy.cpp>

#include "DrawUtil.h"
#include "DrawBrokenView.h"

using namespace TechDraw;

//TODO: errors to PyErrors

// returns a string which represents the object e.g. when printed in python
std::string DrawBrokenViewPy::representation(void) const
{
    return std::string("<DrawBrokenView object>");
}


//==============================================================================
PyObject *DrawBrokenViewPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int DrawBrokenViewPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

