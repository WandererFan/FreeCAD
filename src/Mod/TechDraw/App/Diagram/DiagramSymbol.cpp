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
#include <limits>
#endif

#include <Base/Console.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include "Diagram.h"
#include "DiagramSymbol.h"
#include "DiagramSymbolPy.h"
#include "SymbolPort.h"

using namespace TechDraw;

TYPESYSTEM_SOURCE(TechDraw::DiagramSymbol, Base::Persistence)

DiagramSymbol::DiagramSymbol()  :
    m_parentDiagram(nullptr),
    m_width(256),
    m_height(256)
{
    m_symbolId = 0;
    m_name = "";
    m_representation = new SymbolRepresentation();
    m_location = Base::Vector3d(0.0, 0.0, 0.0);
    m_flags = 0;
    makeDefaultPort();
}

DiagramSymbol::DiagramSymbol(std::string name,
                  SymbolRepresentation* representation,
                  Base::Vector3d location,
                  int flags)  :
    m_parentDiagram(nullptr),
    m_width(256),
    m_height(256)
{
    m_symbolId = 0;
    m_name = name;
    m_representation = representation;
    m_location = location;
    m_flags = flags;
    makeDefaultPort();
}

DiagramSymbol::DiagramSymbol(DiagramSymbol& other) :
    m_parentDiagram(nullptr),
    m_width(256),
    m_height(256)
{
    m_symbolId = 0;
    m_name = other.getName();
    m_representation = other.getRepresentation();
    m_location = other.getLocation();
    m_flags = getFlags();
    makeDefaultPort();
}

//TODO: add a dtor that will release the default port (and the rest of the ports?) or
//      make the default port a smart pointer with ref counting
DiagramSymbol::~DiagramSymbol()
{

}

//this is a temporary kludge. as it stands, it will add an additional default port
//when the symbol is restored.
void DiagramSymbol::makeDefaultPort()
{
    m_defaultPort = new SymbolPort();
    m_defaultPort->setName(std::string("default"));
    m_defaultPort->setAnchor(Base::Vector3d(0.0, 0.0, 0.0));
    m_defaultPort->setMaxConnect(std::numeric_limits<int>::max());
    m_defaultPort->setFlags(0);
}

SymbolPort* DiagramSymbol::getPort(std::string portName) const
{
    if (portName == "default") {
        return m_defaultPort;
    }

    std::vector<SymbolPort*> portsAll = getPorts();
    for (auto& port : portsAll) {
        if (port->getName() == portName) {
            return port;
        }
    }
    return nullptr;
}

void DiagramSymbol::setLocation(Base::Vector3d location)
{
//    Base::Console().Message("DS::setLocation()- : parent? %s\n", getParentDiagram() ? "yes" : "no");
    m_location = location;
    //if we belong to a diagram, we need to advise any connected traces that we have moved
    //this could be a QObject signal eventually
    if (getParentDiagram()) {
        getParentDiagram()->symbolMoved(this);
    }
}

//default implementation
Base::Vector3d DiagramSymbol::getSize() const
{
    return Base::Vector3d(m_width, m_height, 0.0);
}

void DiagramSymbol::Save(Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<Id value=\"" <<  m_symbolId << "\"/>" << std::endl;
    writer.Stream() << writer.ind() << "<Name value=\"" <<  m_name << "\"/>" << std::endl;
    writer.Stream()
         << writer.ind()
             << "<Ports "
             << "PortCount=\""
             << m_ports.size()
             << "\">" << std::endl;

    writer.incInd();
    for (auto& port: m_ports) {
        port->Save(writer);
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</Ports>" << std::endl ;
    m_representation->Save(writer);
    writer.Stream() << writer.ind() << "<Location "
                << "X=\"" <<  m_location.x <<
                "\" Y=\"" <<  m_location.y <<
                "\" Z=\"" <<  m_location.z <<
                 "\"/>" << std::endl;
    writer.Stream() << writer.ind() << "<Flags value=\"" <<  m_flags << "\"/>" << std::endl;
}

void DiagramSymbol::Restore(Base::XMLReader &reader)
{
    reader.readElement("Id");
    m_symbolId = reader.getAttributeAsInteger("value");
    reader.readElement("Name");
    m_name = reader.getAttribute("value");
    reader.readElement("Ports");
    int count = reader.getAttributeAsInteger("PortCount");
    int i = 0;
    for ( ; i < count; i++) {
        SymbolPort* newPort = new SymbolPort;
        newPort->Restore(reader);
        m_ports.push_back(newPort);
    }
    reader.readEndElement("Ports");
    SymbolRepresentation* newRepresentation = new SymbolRepresentation;
    newRepresentation->Restore(reader);
    m_representation = newRepresentation;
    reader.readElement("Location");
    m_location.x = reader.getAttributeAsFloat("X");
    m_location.y = reader.getAttributeAsFloat("Y");
    m_location.z = reader.getAttributeAsFloat("Z");
    reader.readElement("Flags");
    m_flags = reader.getAttributeAsInteger("value");
}

void DiagramSymbol::dump(const char* title)
{
    Base::Console().Message("DS::dump - %s \n", title);
    Base::Console().Message("DS::dump - %s \n", toString().c_str());
}

std::string DiagramSymbol::toString() const
{
    std::stringstream ss;
    ss << m_name << ", " <<
          "(" << m_location.x << ", " <<
          m_location.y << ", " <<
          m_location.z << "), " <<
          m_representation->toString() << ", " <<
          m_flags;
    return ss.str();
}

PyObject *DiagramSymbol::getPyObject(void)
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new DiagramSymbolPy(this), true);
    }

    return Py::new_reference_to(PythonObject);
}

