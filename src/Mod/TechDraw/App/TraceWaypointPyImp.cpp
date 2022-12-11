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

#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>

//#include "Diagram/TraceWaypoint.h"

// inclusion of the generated files
#include <Mod/TechDraw/App/TraceWaypointPy.h>
#include <Mod/TechDraw/App/TraceWaypointPy.cpp>

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string TraceWaypointPy::representation() const
{
    return std::string("<TraceWaypoint object>");
}

PyObject* TraceWaypointPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of TraceWaypointPy
    return new TraceWaypointPy(new TraceWaypoint());
}

// TraceWaypoint takes 0 arguments or  Location, Flags
int TraceWaypointPy::PyInit(PyObject* args, PyObject*)
{
    if (PyArg_ParseTuple(args, "")) {
        return 0;
    }

    PyErr_Clear();
    PyObject* inLocation(nullptr);
    int inFlags(0);

    if (!PyArg_ParseTuple(args, "O!|i", inLocation, &(Base::VectorPy::Type), inFlags)) {
        return -1;
    }

    if (inLocation) {
        Base::Vector3d locn = static_cast<Base::VectorPy*>(inLocation)->value();
        getTraceWaypointPtr()->setLocation(locn);
        getTraceWaypointPtr()->setFlags(inFlags);
    }

    return 0;
}

Py::Object TraceWaypointPy::getLocation() const
{
    Base::Vector3d anchor = getTraceWaypointPtr()->getLocation();
    return Py::asObject(new Base::VectorPy(anchor));
}

void TraceWaypointPy::setLocation(Py::Object arg)
{
    Base::Vector3d anchor;
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        anchor = static_cast<Base::VectorPy*>(p)->value();
    } else if (PyObject_TypeCheck(p, &PyTuple_Type)) {
        anchor = Base::getVectorFromTuple<double>(p);
    }
    else {
        std::string error = std::string("type must be 'Vector', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }

    getTraceWaypointPtr()->setLocation(anchor);
}

Py::Object TraceWaypointPy::getFlags() const
{
    int flags = getTraceWaypointPtr()->getFlags();
    return Py::asObject(PyLong_FromLong((long) flags));
}

void TraceWaypointPy::setFlags(Py::Object arg)
{
    int flags(0);
    PyObject* p = arg.ptr();
    if (PyLong_Check(p)) {
        flags = (int) PyLong_AsLong(p);
    } else {
        throw Py::TypeError("expected (int)");
    }
    getTraceWaypointPtr()->setFlags(flags);
}

PyObject *TraceWaypointPy::getCustomAttributes(const char* ) const
{
    return nullptr;
}

int TraceWaypointPy::setCustomAttributes(const char* , PyObject *)
{
    return 0;
}
