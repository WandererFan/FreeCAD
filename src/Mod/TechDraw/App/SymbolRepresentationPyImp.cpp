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

#include "Diagram/SymbolRepresentation.h"

// inclusion of the generated files
#include <Mod/TechDraw/App/SymbolRepresentationPy.h>
#include <Mod/TechDraw/App/SymbolRepresentationPy.cpp>

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string SymbolRepresentationPy::representation() const
{
    return std::string("<SymbolRepresentation object>");
}

PyObject* SymbolRepresentationPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of SymbolRepresentationPy
    return new SymbolRepresentationPy(new SymbolRepresentation());
}

// takes 0 arguments or  Representation, Flags
int SymbolRepresentationPy::PyInit(PyObject* args, PyObject*)
{
    if (PyArg_ParseTuple(args, "")) {
        return 0;
    }

    PyErr_Clear();
    char* inFile(nullptr);
    int inFlags(0);

    if (!PyArg_ParseTuple(args, "s|i", inFile, inFlags)) {
        return -1;
    }

    if (inFile) {
        getSymbolRepresentationPtr()->setFileReference(inFile);
        getSymbolRepresentationPtr()->setFlags(inFlags);
    }

    return 0;
}
Py::String SymbolRepresentationPy::getFileReference() const
{
    return Py::String(getSymbolRepresentationPtr()->getFileReference());
}

void SymbolRepresentationPy::setFileReference(Py::String arg)
{
    std::string repr = static_cast<std::string>(arg);
    getSymbolRepresentationPtr()->setFileReference(repr);
}

Py::Object SymbolRepresentationPy::getFlags() const
{
    int flags = getSymbolRepresentationPtr()->getFlags();
    return Py::asObject(PyLong_FromLong((long) flags));
}

void SymbolRepresentationPy::setFlags(Py::Object arg)
{
    int flags(0);
    PyObject* p = arg.ptr();
    if (PyLong_Check(p)) {
        flags = (int) PyLong_AsLong(p);
    } else {
        throw Py::TypeError("expected (int)");
    }
    getSymbolRepresentationPtr()->setFlags(flags);
}

PyObject *SymbolRepresentationPy::getCustomAttributes(const char* ) const
{
    return nullptr;
}

int SymbolRepresentationPy::setCustomAttributes(const char* , PyObject *)
{
    return 0;
}
