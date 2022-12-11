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
#ifndef TECHDRAW_DIAGRAMTRACE_H
#define TECHDRAW_DIAGRAMTRACE_H

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <App/FeaturePython.h>
#include <Base/Persistence.h>
#include <Base/Vector3D.h>

#include "TraceRoute.h"
#include "DiagramSymbol.h"
#include "SymbolPort.h"

using TraceId = int;

namespace TechDraw {

//description of a connection between (SymbolA, PortN) and (SymbolB, portM)
class TechDrawExport DiagramTrace : Base::Persistence
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    DiagramTrace();
    DiagramTrace(SymbolId fromSymbol,
                std::string fromPort,
                SymbolId toSymbol,
                std::string toPort,
                TraceRoute* route,
                int flags);
    DiagramTrace(DiagramTrace& other);
    ~DiagramTrace() override = default;

    // setters & getters
    Diagram* getParentDiagram() const { return m_parentDiagram; }
    void setParentDiagram(Diagram* parent) { m_parentDiagram = parent; }

    TraceId getTraceId() const { return m_traceId; }
    void setTraceId(TraceId id) { m_traceId = id; }

    SymbolId getFromSymbol() const { return m_fromSymbol; }
    void setFromSymbol(SymbolId fromSymbol) { m_fromSymbol = fromSymbol; }

    SymbolId getToSymbol() const { return m_toSymbol; }
    void setToSymbol(SymbolId toSymbol) { m_toSymbol = toSymbol; }

    std::string getFromPort() const { return m_fromPort; }
    void setFromPort(std::string fromPort) { m_fromPort = fromPort; }

    std::string getToPort() const { return m_toPort; }
    void setToPort(std::string toPort) { m_toPort = toPort; }

    TraceRoute* getRoute() const { return m_route; }
    void setRoute(TraceRoute* route) { m_route = route; }

    int getFlags() const { return m_flags; }
    void setFlags(int flags) { m_flags = flags; }

    // personality
    virtual bool isInterested(DiagramSymbol* symbol) const;
    virtual void changedEndpoint(DiagramSymbol* endpoint);

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override { return 1; }
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;

    PyObject *getPyObject() override;

    void dump(const char* title);
    std::string toString() const;

private:
    TechDraw::Diagram* m_parentDiagram;

    TraceId m_traceId;
    SymbolId m_fromSymbol;
    std::string m_fromPort;
    SymbolId m_toSymbol;
    std::string m_toPort;
    TraceRoute* m_route;
    int m_flags;

    Py::Object PythonObject;
};

using DiagramTracePython = App::FeaturePythonT<DiagramTrace>;

} //end namespace TechDraw

#endif //TECHDRAW_DIAGRAMTRACE_H

