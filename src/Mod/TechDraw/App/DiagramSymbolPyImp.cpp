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

#include "DiagramPy.h"
#include "SymbolRepresentationPy.h"
#include "SymbolPortPy.h"

// inclusion of the generated files
#include <Mod/TechDraw/App/DiagramSymbolPy.h>
#include <Mod/TechDraw/App/DiagramSymbolPy.cpp>

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DiagramSymbolPy::representation() const
{
    return std::string("<DiagramSymbol object>");
}

PyObject* DiagramSymbolPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    Base::Console().Message("DSP::PyMake()\n");
    // create a new instance of DiagramSymbolPy
    return new DiagramSymbolPy(new DiagramSymbol());
}

//DiagramSymbol takes 0 arguments or  Name, SymbolRepresentation, Location, Flags
int DiagramSymbolPy::PyInit(PyObject* args, PyObject*)
{
//    Base::Console().Message("DSP::PyInit()\n");
    if (PyArg_ParseTuple(args, "")) {
        return 0;
    }

    PyErr_Clear();
    char* inName(nullptr);
    PyObject* inRepresentation(nullptr);
    PyObject* inLocation(nullptr);
    int inFlags(0);

    if (!PyArg_ParseTuple(args, "sO!O!|i", &inName, &(TechDraw::SymbolRepresentationPy::Type), &inRepresentation,
                                                  &(Base::VectorPy::Type), &inLocation, &inFlags)) {
        return -1;
    }

    if (inName) {
        getDiagramSymbolPtr()->setName(inName);

        SymbolRepresentation* rep = static_cast<SymbolRepresentationPy*>(inRepresentation)->getSymbolRepresentationPtr();
        getDiagramSymbolPtr()->setRepresentation(rep);

        Base::Vector3d locn = static_cast<Base::VectorPy*>(inLocation)->value();
        getDiagramSymbolPtr()->setLocation(locn);

        getDiagramSymbolPtr()->setFlags(inFlags);
    }

    return 0;
}

Py::Object DiagramSymbolPy::getParentDiagram() const
{
    Diagram* diagram = getDiagramSymbolPtr()->getParentDiagram();
    return Py::asObject(diagram->getPyObject());
}

void DiagramSymbolPy::setParentDiagram(Py::Object arg)
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

    getDiagramSymbolPtr()->setParentDiagram(diagram);
}

Py::Object DiagramSymbolPy::getSymbolId() const
{
    int id = getDiagramSymbolPtr()->getSymbolId();
    return Py::asObject(PyLong_FromLong((long) id));
}

Py::String DiagramSymbolPy::getName() const
{
    return Py::String(getDiagramSymbolPtr()->getName());
}

void DiagramSymbolPy::setName(Py::String arg)
{
    std::string repr = static_cast<std::string>(arg);
    getDiagramSymbolPtr()->setName(std::string(repr));
}

PyObject* DiagramSymbolPy::getPort(PyObject* args)
{
    char* inName(nullptr);
    if (!PyArg_ParseTuple(args, "s", &inName)) {
        return nullptr;
    }

    SymbolPort* port = getDiagramSymbolPtr()->getPort(std::string(inName));
    if (!port) {
        Base::Console().Message("DSP::getPort(%s) - port not found\n", inName);
        return nullptr;
    }
    return new SymbolPortPy(port);
}

Py::List DiagramSymbolPy::getPorts() const
{
    std::vector<SymbolPort*> ports = getDiagramSymbolPtr()->getPorts();
    Py::List list;
    for (auto it : ports) {
        list.append(Py::asObject(new SymbolPortPy(it)));
    }
    return list;
}

void DiagramSymbolPy::setPorts(Py::List arg)
{
    std::vector<SymbolPort*> ports;
    PyObject* p = arg.ptr();

    if (PyTuple_Check(p) || PyList_Check(p)) {
        Py::Sequence list(p);
        Py::Sequence::size_type size = list.size();
        ports.resize(size);

        for (Py::Sequence::size_type i = 0; i < size; i++) {
            Py::Object item = list[i];
            if (!PyObject_TypeCheck(*item, &(TechDraw::SymbolPortPy::Type))) {
                std::string error = std::string("type in list must be 'SymbolPort', not ");
                error += (*item)->ob_type->tp_name;
                throw Base::TypeError(error);
            }
            ports[i] = static_cast<TechDraw::SymbolPortPy*>(*item)->getSymbolPortPtr();
        }
        getDiagramSymbolPtr()->setPorts(ports);
    }
}
Py::Object DiagramSymbolPy::getLocation() const
{
    Base::Vector3d anchor = getDiagramSymbolPtr()->getLocation();
    return Py::asObject(new Base::VectorPy(anchor));
}

void DiagramSymbolPy::setLocation(Py::Object arg)
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

    getDiagramSymbolPtr()->setLocation(anchor);
}

Py::Object DiagramSymbolPy::getRepresentation() const
{
    SymbolRepresentation* repr = getDiagramSymbolPtr()->getRepresentation();
    return Py::asObject(new SymbolRepresentationPy(repr));
}

void DiagramSymbolPy::setRepresentation(Py::Object arg)
{
    SymbolRepresentation* repr;
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(TechDraw::SymbolRepresentationPy::Type)))  {
        repr = static_cast<TechDraw::SymbolRepresentationPy*>(p)->getSymbolRepresentationPtr();
    } else {
        std::string error = std::string("type must be 'SymbolRepresentation', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }

    getDiagramSymbolPtr()->setRepresentation(repr);
}

Py::Object DiagramSymbolPy::getFlags() const
{
    int flags = getDiagramSymbolPtr()->getFlags();
    return Py::asObject(PyLong_FromLong((long) flags));
}

void DiagramSymbolPy::setFlags(Py::Object arg)
{
    int flags(0);
    PyObject* p = arg.ptr();
    if (PyLong_Check(p)) {
        flags = (int) PyLong_AsLong(p);
    } else {
        throw Py::TypeError("expected (int)");
    }
    getDiagramSymbolPtr()->setFlags(flags);
}


PyObject *DiagramSymbolPy::getCustomAttributes(const char* ) const
{
    return nullptr;
}

int DiagramSymbolPy::setCustomAttributes(const char* , PyObject *)
{
    return 0;
}
