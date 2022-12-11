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
#ifndef _PreComp_
#endif

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include <Mod/TechDraw/App/DrawUtil.h>
#include "DiagramPy.h"
#include "DiagramTrace.h"
#include "DiagramTracePy.h"

using namespace TechDraw;
using DU = DrawUtil;

TYPESYSTEM_SOURCE(TechDraw::DiagramTrace, Base::Persistence)

DiagramTrace::DiagramTrace() :
    m_parentDiagram(nullptr),
    m_traceId(0),
    m_fromSymbol(0),
    m_fromPort(std::string()),
    m_toSymbol(0),
    m_toPort(std::string()),
    m_route(nullptr),
    m_flags(0)
{
}

DiagramTrace::DiagramTrace(SymbolId fromSymbol,
                           std::string fromPort,
                SymbolId toSymbol,
                std::string toPort,
                TraceRoute *route,
                int flags) :
    m_parentDiagram(nullptr)
{
    m_traceId = 0;
    m_fromSymbol = fromSymbol;
    m_fromPort = fromPort;
    m_toSymbol = toSymbol;
    m_toPort = toPort;
    m_route = route;
    m_flags = flags;
}

DiagramTrace::DiagramTrace(DiagramTrace& other) :
    m_parentDiagram(nullptr)
{
    m_fromSymbol = other.getFromSymbol();
    m_fromPort = other.getFromPort();
    m_toSymbol = other.getToSymbol();
    m_toPort = other.getToPort();
    m_route = other.getRoute();
    m_flags = other.getFlags();
}

//if changes in this symbol affect us, we are interested
bool DiagramTrace::isInterested(DiagramSymbol* symbol) const
{
//    Base::Console().Message("DT::isInterested() - symbolId: %d fromSymbol: %d toSymbol: %d\n", symbol->getSymbolId(), getFromSymbol(), getToSymbol());
    if (symbol->getSymbolId() == getFromSymbol()  ||
        symbol->getSymbolId() == getToSymbol()) {
        return true;
    }
    return false;
}

//adjust the end of our route to match the location of the new endpoint
void DiagramTrace::changedEndpoint(DiagramSymbol* endpoint)
{
    Base::Vector3d newLocation = endpoint->getLocation();
    auto pointsAll = getRoute()->getWaypoints();
    std::string diagramName = getParentDiagram()->getNameInDocument();

    if (endpoint->getSymbolId() == getFromSymbol()) {
        Base::Vector3d anchorAdjust = endpoint->getPort(getFromPort())->getAnchor();
        pointsAll.front()->setLocation(newLocation + anchorAdjust);
        getRoute()->setWaypoints(pointsAll);

        //redrawOurselves()
        Base::Interpreter().runString("from TechDrawDiagram import TDDiagramWorkers");
        Base::Interpreter().runStringArg("TDDiagramWorkers.redrawTrace(App.activeDocument().%s, %d)",
                                         diagramName.c_str(), getTraceId() );

    } else if (endpoint->getSymbolId() == getToSymbol()) {
        Base::Vector3d anchorAdjust = endpoint->getPort(getToPort())->getAnchor();
        pointsAll.back()->setLocation(newLocation + anchorAdjust);
        getRoute()->setWaypoints(pointsAll);

        //redrawOurselves()
        Base::Interpreter().runString("from TechDrawDiagram import TDDiagramWorkers");
        Base::Interpreter().runStringArg("TDDiagramWorkers.redrawTrace(App.activeDocument().%s, %d)",
                                         diagramName.c_str(), getTraceId() );
    }
}

void DiagramTrace::Save(Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<Id value=\"" <<  m_traceId << "\"/>" << std::endl;
    writer.Stream() << writer.ind() << "<FromSymbol value=\"" <<  m_fromSymbol << "\"/>" << std::endl;
    writer.Stream() << writer.ind() << "<FromPort value=\"" <<  m_fromPort << "\"/>" << std::endl;
    writer.Stream() << writer.ind() << "<ToSymbol value=\"" <<  m_toSymbol << "\"/>" << std::endl;
    writer.Stream() << writer.ind() << "<ToPort value=\"" <<  m_toPort << "\"/>" << std::endl;
    m_route->Save(writer);
    writer.Stream() << writer.ind() << "<Flags value=\"" <<  m_flags << "\"/>" << std::endl;
}

void DiagramTrace::Restore(Base::XMLReader &reader)
{
    reader.readElement("Id");
    m_traceId = reader.getAttributeAsInteger("value");
    reader.readElement("FromSymbol");
    m_fromSymbol = reader.getAttributeAsInteger("value");
    reader.readElement("FromPort");
    m_fromPort = reader.getAttribute("value");
    reader.readElement("ToSymbol");
    m_toSymbol = reader.getAttributeAsInteger("value");
    reader.readElement("ToPort");
    m_toPort = reader.getAttribute("value");
    TraceRoute* newRoute = new TraceRoute;
    newRoute->Restore(reader);
    m_route = newRoute;
    reader.readElement("Flags");
    m_flags = reader.getAttributeAsInteger("value");
}

void DiagramTrace::dump(const char* title)
{
    Base::Console().Message("DT::dump - %s \n", title);
    Base::Console().Message("DT::dump - %s \n", toString().c_str());
}

std::string DiagramTrace::toString() const
{
    std::stringstream ss;
    ss << m_traceId << ", " <<
          m_fromSymbol << ", " <<
          m_fromPort << ", " <<
          m_toSymbol << ", " <<
          m_toPort << ", " <<
          m_route->toString() << ", " <<
          m_flags;
    return ss.str();
}

PyObject *DiagramTrace::getPyObject(void)
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new DiagramTracePy(this), true);
    }

    return Py::new_reference_to(PythonObject);
}
