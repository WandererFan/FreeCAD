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

#include "TraceWaypointPy.h"

// inclusion of the generated files
#include <Mod/TechDraw/App/TraceRoutePy.h>
#include <Mod/TechDraw/App/TraceRoutePy.cpp>

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string TraceRoutePy::representation() const
{
    return std::string("<TraceRoute object>");
}

PyObject* TraceRoutePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of TraceRoutePy
    return new TraceRoutePy(new TraceRoute());
}

// TraceRoute takes 0 arguments or  [WayPoints], Flags
int TraceRoutePy::PyInit(PyObject* args, PyObject*)
{
    if (PyArg_ParseTuple(args, "")) {
        return 0;
    }

    PyErr_Clear();
    PyObject* inPoints(nullptr);
    int inFlags(0);

    if (!PyArg_ParseTuple(args, "O!|i", inPoints, inFlags)) {
        return -1;
    }

    if (inPoints) {
        Py::Sequence list(inPoints);
        TraceWaypointList pointsAll;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            if (PyObject_TypeCheck((*it).ptr(), &(TraceWaypointPy::Type))) {
                TraceWaypoint* point = static_cast<TraceWaypointPy*>((*it).ptr())->getTraceWaypointPtr();
                pointsAll.push_back(point);
            }
        }
        getTraceRoutePtr()->setWaypoints(pointsAll);
        getTraceRoutePtr()->setFlags(inFlags);
    }

    return 0;
}

Py::List TraceRoutePy::getWaypoints() const
{
    TraceWaypointList waypoints = getTraceRoutePtr()->getWaypoints();
    Py::List list;
    for (auto it : waypoints) {
        list.append(Py::asObject(new TraceWaypointPy(it)));
    }
    return list;
}

void TraceRoutePy::setWaypoints(Py::List arg)
{
    TraceWaypointList points;
    PyObject* p = arg.ptr();

    if (PyTuple_Check(p) || PyList_Check(p)) {
        Py::Sequence list(p);
        Py::Sequence::size_type size = list.size();
        points.resize(size);

        for (Py::Sequence::size_type i = 0; i < size; i++) {
            Py::Object item = list[i];
            if (!PyObject_TypeCheck(*item, &(TechDraw::TraceWaypointPy::Type))) {
                std::string error = std::string("type in list must be 'TraceWaypoint', not ");
                error += (*item)->ob_type->tp_name;
                throw Base::TypeError(error);
            }
            points[i] = static_cast<TechDraw::TraceWaypointPy*>(*item)->getTraceWaypointPtr();
        }
        getTraceRoutePtr()->setWaypoints(points);
    }
}

Py::Object TraceRoutePy::getFlags() const
{
    int flags = getTraceRoutePtr()->getFlags();
    return Py::asObject(PyLong_FromLong((long) flags));
}

void TraceRoutePy::setFlags(Py::Object arg)
{
    int flags(0);
    PyObject* p = arg.ptr();
    if (PyLong_Check(p)) {
        flags = (int) PyLong_AsLong(p);
    } else {
        throw Py::TypeError("expected (int)");
    }
    getTraceRoutePtr()->setFlags(flags);
}

PyObject *TraceRoutePy::getCustomAttributes(const char* ) const
{
    return nullptr;
}

int TraceRoutePy::setCustomAttributes(const char* , PyObject *)
{
    return 0;
}
