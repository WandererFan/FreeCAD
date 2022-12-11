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
#include <Base/Vector3D.h>

#include "Diagram/SymbolPort.h"

// inclusion of the generated files
#include <Mod/TechDraw/App/SymbolPortPy.h>
#include <Mod/TechDraw/App/SymbolPortPy.cpp>

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string SymbolPortPy::representation() const
{
    return std::string("<SymbolPort object>");
}

PyObject* SymbolPortPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of SymbolPortPy
    return new SymbolPortPy(new SymbolPort());
}

// takes 0 arguments or  Name, Anchor, MaxConnect, Flags
int SymbolPortPy::PyInit(PyObject* args, PyObject*)
{
    if (PyArg_ParseTuple(args, "")) {
        return 0;
    }

    PyErr_Clear();
    char* inName(nullptr);
    PyObject* inAnchor;
    int inMax(0);
    int inFlags(0);

    if (!PyArg_ParseTuple(args, "sO!i|i", inName, &(Base::VectorPy::Type), &inAnchor, inMax, inFlags)) {
        return -1;
    }

    if (inName) {
        getSymbolPortPtr()->setName(inName);
        Base::Vector3d anchor = static_cast<Base::VectorPy*>(inAnchor)->value();
        getSymbolPortPtr()->setAnchor(anchor);
        getSymbolPortPtr()->setMaxConnect(inMax);
        getSymbolPortPtr()->setFlags(inFlags);
    }

    return 0;
}

Py::String SymbolPortPy::getName() const
{
    return Py::String(getSymbolPortPtr()->getName());
}

void SymbolPortPy::setName(Py::String arg)
{
    std::string repr = static_cast<std::string>(arg);
    getSymbolPortPtr()->setName(std::string(repr));
}

Py::Object SymbolPortPy::getAnchor() const
{
    Base::Vector3d anchor = getSymbolPortPtr()->getAnchor();
    return Py::asObject(new Base::VectorPy(anchor));
}

void SymbolPortPy::setAnchor(Py::Object arg)
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

    getSymbolPortPtr()->setAnchor(anchor);
}

Py::Object SymbolPortPy::getMaxConnect() const
{
    int flags = getSymbolPortPtr()->getMaxConnect();
    return Py::asObject(PyLong_FromLong((long) flags));
}

void SymbolPortPy::setMaxConnect(Py::Object arg)
{
    int flags(0);
    PyObject* p = arg.ptr();
    if (PyLong_Check(p)) {
        flags = (int) PyLong_AsLong(p);
    } else {
        throw Py::TypeError("expected (int)");
    }
    getSymbolPortPtr()->setMaxConnect(flags);
}

Py::Object SymbolPortPy::getFlags() const
{
    int flags = getSymbolPortPtr()->getFlags();
    return Py::asObject(PyLong_FromLong((long) flags));
}

void SymbolPortPy::setFlags(Py::Object arg)
{
    int flags(0);
    PyObject* p = arg.ptr();
    if (PyLong_Check(p)) {
        flags = (int) PyLong_AsLong(p);
    } else {
        throw Py::TypeError("expected (int)");
    }
    getSymbolPortPtr()->setFlags(flags);
}

PyObject *SymbolPortPy::getCustomAttributes(const char* ) const
{
    return nullptr;
}

int SymbolPortPy::setCustomAttributes(const char* , PyObject *)
{
    return 0;
}
