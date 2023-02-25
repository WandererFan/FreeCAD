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

//a feature for a simple diagram on top of a TechDrawPage, rather like a transparent overlay.

#ifndef TECHDRAW_DIAGRAM_H
#define TECHDRAW_DIAGRAM_H

#include <Mod/TechDraw/TechDrawGlobal.h>
#include <Mod/TechDraw/App/DrawPage.h>

#include "PropertyDiagramSymbolList.h"
#include "PropertyDiagramTraceList.h"
#include "DiagramSymbol.h"
#include "DiagramTrace.h"

namespace TechDraw
{

class TechDrawExport Diagram: public TechDraw::DrawPage
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::Diagram);

public:
    Diagram();
    ~Diagram() override = default;

    TechDraw::PropertyDiagramSymbolList Symbols;
    TechDraw::PropertyDiagramTraceList Traces;
    App::PropertyMap Metadata;
    App::PropertyInteger NextSymbolId;
    App::PropertyInteger NextTraceId;

    App::DocumentObjectExecReturn *execute() override;
    void onDocumentRestored() override;

    // child object ACID
    virtual SymbolId addSymbol(TechDraw::DiagramSymbol* symbol);
    virtual SymbolId copySymbol(SymbolId id);
    virtual int removeSymbol(TechDraw::DiagramSymbol* toDelete);
    virtual DiagramSymbol* getSymbol(SymbolId id);
    virtual TraceId addTrace(TechDraw::DiagramTrace* trace);
    virtual int removeTrace(TechDraw::DiagramTrace* toDelete);
    virtual DiagramTrace* getTrace(TraceId id);

    virtual int addMetaEntry(std::string key, std::string data);
    virtual int removeMetaEntry(std::string key);

    virtual void symbolMoved(DiagramSymbol* symbol);

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override {
        return "TechDrawGui::ViewProviderDiagram";
    }

    PyObject *getPyObject() override;

private:

};

using DiagramPython = App::FeaturePythonT<Diagram>;

} //namespace TechDraw


#endif


