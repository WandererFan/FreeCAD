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

#include "DrawPage.h"
#include "Diagram/Diagram.h"

// inclusion of the generated files
#include <Mod/TechDraw/App/DiagramPy.h>
#include <Mod/TechDraw/App/DiagramPy.cpp>
#include <Mod/TechDraw/App/DiagramSymbolPy.h>
#include <Mod/TechDraw/App/DiagramTracePy.h>

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DiagramPy::representation() const
{
    return std::string("<Diagram object>");
}

PyObject* DiagramPy::addSymbol(PyObject* args)
{
    PyObject *pyDocObj;
    if (!PyArg_ParseTuple(args, "O!", &(TechDraw::DiagramSymbolPy::Type), &pyDocObj)) {
        return nullptr;
    }

    Diagram* diagram = getDiagramPtr();
    DiagramSymbolPy* pySymbol = static_cast<TechDraw::DiagramSymbolPy*>(pyDocObj);
    DiagramSymbol* symbol = pySymbol->getDiagramSymbolPtr();
    int addedSymbolId = diagram->addSymbol(symbol);

    return PyLong_FromLong(addedSymbolId);
}

PyObject* DiagramPy::copySymbol(PyObject* args)
{
    int id;
    if (!PyArg_ParseTuple(args, "i", &id)) {
        return nullptr;
    }

    Diagram* diagram = getDiagramPtr();
    int copyId = diagram->copySymbol(id);
    return PyLong_FromLong(copyId);
}

PyObject* DiagramPy::removeSymbol(PyObject* args)
{
    PyObject *pyDocObj;
    if (!PyArg_ParseTuple(args, "O!", &(TechDraw::DiagramSymbolPy::Type), &pyDocObj)) {
        return nullptr;
    }

    Diagram* diagram = getDiagramPtr();
    DiagramSymbolPy* pySymbol = static_cast<TechDraw::DiagramSymbolPy*>(pyDocObj);
    DiagramSymbol* symbol = pySymbol->getDiagramSymbolPtr();
    int rc = diagram->removeSymbol(symbol);

    return PyLong_FromLong(rc);
}

PyObject* DiagramPy::getSymbol(PyObject* args)
{
    int id;
    if (!PyArg_ParseTuple(args, "i", &id)) {
        return nullptr;
    }

    Diagram* diagram = getDiagramPtr();
    DiagramSymbol* symbol = diagram->getSymbol(id);
    if (symbol) {
        return new DiagramSymbolPy(symbol);
    }

    return Py_None;
}

PyObject* DiagramPy::addTrace(PyObject* args)
{
    PyObject *pyDocObj;
    if (!PyArg_ParseTuple(args, "O!", &(TechDraw::DiagramTracePy::Type), &pyDocObj)) {
        return nullptr;
    }

    Diagram* diagram = getDiagramPtr();
    DiagramTracePy* pyTrace = static_cast<TechDraw::DiagramTracePy*>(pyDocObj);
    DiagramTrace* trace = pyTrace->getDiagramTracePtr();
    int rc = diagram->addTrace(trace);

    return PyLong_FromLong(rc);
}

PyObject* DiagramPy::removeTrace(PyObject* args)
{
    PyObject *pyDocObj;
    if (!PyArg_ParseTuple(args, "O!", &(TechDraw::DiagramTracePy::Type), &pyDocObj)) {
        return nullptr;
    }

    Diagram* diagram = getDiagramPtr();
    DiagramTracePy* pyTrace = static_cast<TechDraw::DiagramTracePy*>(pyDocObj);
    DiagramTrace* trace = pyTrace->getDiagramTracePtr();
    int rc = diagram->removeTrace(trace);

    return PyLong_FromLong(rc);
}

PyObject* DiagramPy::getTrace(PyObject* args)
{
    int id;
    if (!PyArg_ParseTuple(args, "i", &id)) {
        return nullptr;
    }

    Diagram* diagram = getDiagramPtr();
    DiagramTrace* trace = diagram->getTrace(id);
    if (trace) {
        return new DiagramTracePy(trace);
    }

    return Py_None;
}

PyObject *DiagramPy::getCustomAttributes(const char* ) const
{
    return nullptr;
}

int DiagramPy::setCustomAttributes(const char* , PyObject *)
{
    return 0;
}
