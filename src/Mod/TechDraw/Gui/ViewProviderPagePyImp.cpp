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

#ifndef _PreComp_
# include <sstream>
#endif

#include <Mod/TechDraw/App/DrawPage.h>

// inclusion of the generated files (generated out of ViewProviderPagePy.xml)
#include <Mod/TechDraw/Gui/ViewProviderPagePy.h>
#include <Mod/TechDraw/Gui/ViewProviderPagePy.cpp>

using namespace TechDrawGui;
using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string ViewProviderPagePy::representation() const
{
    std::stringstream str;
    str << "<View provider object at " << getViewProviderPagePtr() << ">";

    return str.str();
}

PyObject* ViewProviderPagePy::getDrawPage(PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    DrawPage* dp = getViewProviderPagePtr()->getDrawPage();
    return dp->getPyObject();
}

Py::Object ViewProviderPagePy::getPageWidth() const
{
    DrawPage* dp = getViewProviderPagePtr()->getDrawPage();
    double width = dp->getPageWidth();
    return Py::asObject(PyFloat_FromDouble(width));
}

Py::Object ViewProviderPagePy::getPageHeight() const
{
    DrawPage* dp = getViewProviderPagePtr()->getDrawPage();
    double height = dp->getPageHeight();
    return Py::asObject(PyFloat_FromDouble(height));
}

PyObject *ViewProviderPagePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ViewProviderPagePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
