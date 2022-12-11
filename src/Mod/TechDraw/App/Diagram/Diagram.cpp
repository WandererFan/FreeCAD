/***************************************************************************
 *   Copyright (c) 2022 WandererFan  <wandererfan@gmail.com>              *
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
# include <iostream>
# include <sstream>
# include <Precision.hxx>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include "Diagram.h"
#include "DiagramPy.h"  // generated from DiagramPy.xml


using namespace TechDraw;

//===========================================================================
// Diagram
//===========================================================================

PROPERTY_SOURCE(TechDraw::Diagram, TechDraw::DrawPage)


Diagram::Diagram()
{
    static const char *group = "Diagram";

    ADD_PROPERTY_TYPE(Symbols, (nullptr), group, (App::PropertyType::Prop_Output), "The symbols in this diagram");
    ADD_PROPERTY_TYPE(Traces, (nullptr), group, (App::PropertyType::Prop_Output), "The connections between symbols");
    ADD_PROPERTY_TYPE(Metadata, (), group, (App::PropertyType::Prop_Output), "Information about the diagram");
    ADD_PROPERTY_TYPE(NextSymbolId, (0), group, (App::PropertyType::Prop_Output), "The next symbol id to be assigned");
    ADD_PROPERTY_TYPE(NextTraceId, (0), group, (App::PropertyType::Prop_Output), "The next trace id to be assigned");
}

//Diagram is just a container. It doesn't "do" anything.
App::DocumentObjectExecReturn* Diagram::execute()
{
    return App::DocumentObject::execute();
}

void Diagram::onDocumentRestored()
{
    for (auto& symbol : Symbols.getValues()) {
        symbol->setParentDiagram(this);
    }
    for (auto& trace : Traces.getValues())  {
        trace->setParentDiagram(this);
    }
    App::DocumentObject::onDocumentRestored();
}

int Diagram::addSymbol(TechDraw::DiagramSymbol* symbol)
{
    SymbolId id = NextSymbolId.getValue();
    NextSymbolId.setValue(id + 1);
    symbol->setSymbolId(id);
    auto symbols = Symbols.getValues();
    symbols.push_back(symbol);
    Symbols.setValues(symbols);
    symbol->setParentDiagram(this);
    return id;
}

SymbolId Diagram::copySymbol(SymbolId id)
{
    DiagramSymbol* pattern = getSymbol(id);
    DiagramSymbol* copy = new DiagramSymbol(*pattern);
    SymbolId copyId = addSymbol(copy);
    return copyId;
}

int Diagram::removeSymbol(TechDraw::DiagramSymbol* symbol)
{
    return 0;
}

DiagramSymbol* Diagram::getSymbol(SymbolId id)
{
    for (auto& symbol : Symbols.getValues()) {
        if (symbol->getSymbolId() == id) {
            return symbol;
        }
    }
    return nullptr;
}

int Diagram::addTrace(TechDraw::DiagramTrace* trace)
{
    TraceId id = NextTraceId.getValue();
    NextTraceId.setValue(id + 1);
    trace->setTraceId(id);
    auto traces = Traces.getValues();
    traces.push_back(trace);
    Traces.setValues(traces);
    trace->setParentDiagram(this);
    return id;
}

int Diagram::removeTrace(TechDraw::DiagramTrace* trace)
{
    return 0;
}

DiagramTrace* Diagram::getTrace(TraceId id)
{
    for (auto& trace : Traces.getValues()) {
        if (trace->getTraceId() == id) {
            return trace;
        }
    }
    return nullptr;
}

int Diagram::addMetaEntry(std::string key, std::string data)
{
    return 0;
}

int Diagram::removeMetaEntry(std::string key)
{
    return 0;
}

void Diagram::symbolMoved(DiagramSymbol* symbol)
{
//    Base::Console().Message("D::symbolMoved() - %d  %s\n", symbol->getSymbolId(), symbol->getName().c_str());
    auto tracesAll = Traces.getValues();
    for (auto& trace : tracesAll) {
        if (trace->isInterested(symbol)) {
            trace->changedEndpoint(symbol);
        }
    }
}

PyObject *Diagram::getPyObject(void)
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new DiagramPy(this), true);
    }

    return Py::new_reference_to(PythonObject);
}


// Python Diagram feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DiagramPython, TechDraw::Diagram)
template<> const char* TechDraw::DiagramPython::getViewProviderName(void) const {
    return "TechDrawGui::ViewProviderPage";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::Diagram>;
}
