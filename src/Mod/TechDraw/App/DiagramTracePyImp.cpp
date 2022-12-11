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

#include "DiagramPy.h"
#include "TraceRoutePy.h"

// inclusion of the generated files
#include <Mod/TechDraw/App/DiagramTracePy.h>
#include <Mod/TechDraw/App/DiagramTracePy.cpp>

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DiagramTracePy::representation() const
{
    return std::string("<DiagramTrace object>");
}

PyObject* DiagramTracePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of DiagramSymbolPy
    return new DiagramTracePy(new DiagramTrace());
}

// takes 0 arguments or  FromSymbol, ToSymbol, Route, Flags
int DiagramTracePy::PyInit(PyObject* args, PyObject*)
{
    if (PyArg_ParseTuple(args, "")) {
        return 0;
    }

    PyErr_Clear();
    SymbolId inFrom(-1);
    char* inFromPort(nullptr);
    SymbolId inTo(-1);
    char* inToPort(nullptr);
    PyObject* inRoute(nullptr);
    int inFlags(0);

    if (!PyArg_ParseTuple(args, "isisO!|i", inFrom, inFromPort, inTo, inToPort, &(TechDraw::TraceRoutePy::Type), &inRoute, inFlags)) {
        return -1;
    }

    if (inFrom > -1) {
        getDiagramTracePtr()->setFromSymbol(inFrom);
        getDiagramTracePtr()->setFromPort(inFromPort);
        getDiagramTracePtr()->setToSymbol(inTo);
        getDiagramTracePtr()->setToPort(inToPort);
        TraceRoute* route = static_cast<TraceRoutePy*>(inRoute)->getTraceRoutePtr();
        getDiagramTracePtr()->setRoute(route);
        getDiagramTracePtr()->setFlags(inFlags);
    }

    return 0;
}

Py::Object DiagramTracePy::getParentDiagram() const
{
    Diagram* diagram = getDiagramTracePtr()->getParentDiagram();
    return Py::asObject(diagram->getPyObject());
}

void DiagramTracePy::setParentDiagram(Py::Object arg)
{
    Diagram* diagram;
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(TechDraw::DiagramPy::Type)))  {
        diagram = static_cast<TechDraw::DiagramPy*>(p)->getDiagramPtr();
    } else {
        std::string error = std::string("type must be 'Diagram', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }

    getDiagramTracePtr()->setParentDiagram(diagram);
}

Py::Object DiagramTracePy::getTraceId() const
{
    int id = getDiagramTracePtr()->getTraceId();
    return Py::asObject(PyLong_FromLong((long) id));
}

Py::Object DiagramTracePy::getFromSymbol() const
{
    int id = getDiagramTracePtr()->getFromSymbol();
    return Py::asObject(PyLong_FromLong((long) id));
}

void DiagramTracePy::setFromSymbol(Py::Object arg)
{
    int id(0);
    PyObject* p = arg.ptr();
    if (PyLong_Check(p)) {
        id = (int) PyLong_AsLong(p);
    } else {
        throw Py::TypeError("expected (int)");
    }
    getDiagramTracePtr()->setFromSymbol(id);
}

Py::Object DiagramTracePy::getToSymbol() const
{
    int id = getDiagramTracePtr()->getToSymbol();
    return Py::asObject(PyLong_FromLong((long) id));
}

void DiagramTracePy::setToSymbol(Py::Object arg)
{
    int id(0);
    PyObject* p = arg.ptr();
    if (PyLong_Check(p)) {
        id = (int) PyLong_AsLong(p);
    } else {
        throw Py::TypeError("expected (int)");
    }
    getDiagramTracePtr()->setToSymbol(id);
}

Py::String DiagramTracePy::getFromPort() const
{
    return Py::String(getDiagramTracePtr()->getFromPort());
}

void DiagramTracePy::setFromPort(Py::String arg)
{
    std::string port = static_cast<std::string>(arg);
    getDiagramTracePtr()->setFromPort(std::string(port));
}

Py::String DiagramTracePy::getToPort() const
{
    return Py::String(getDiagramTracePtr()->getToPort());
}

void DiagramTracePy::setToPort(Py::String arg)
{
    std::string port = static_cast<std::string>(arg);
    getDiagramTracePtr()->setToPort(std::string(port));
}

Py::Object DiagramTracePy::getRoute() const
{
    TraceRoute* repr = getDiagramTracePtr()->getRoute();
    return Py::asObject(new TraceRoutePy(repr));
}

void DiagramTracePy::setRoute(Py::Object arg)
{
    TraceRoute* repr;
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(TechDraw::TraceRoutePy::Type)))  {
        repr = static_cast<TechDraw::TraceRoutePy*>(p)->getTraceRoutePtr();
    } else {
        std::string error = std::string("type must be 'TraceRoute', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }

    getDiagramTracePtr()->setRoute(repr);
}

Py::Object DiagramTracePy::getFlags() const
{
    int flags = getDiagramTracePtr()->getFlags();
    return Py::asObject(PyLong_FromLong((long) flags));
}

void DiagramTracePy::setFlags(Py::Object arg)
{
    int flags(0);
    PyObject* p = arg.ptr();
    if (PyLong_Check(p)) {
        flags = (int) PyLong_AsLong(p);
    } else {
        throw Py::TypeError("expected (int)");
    }
    getDiagramTracePtr()->setFlags(flags);
}
PyObject *DiagramTracePy::getCustomAttributes(const char* ) const
{
    return nullptr;
}

int DiagramTracePy::setCustomAttributes(const char* , PyObject *)
{
    return 0;
}
