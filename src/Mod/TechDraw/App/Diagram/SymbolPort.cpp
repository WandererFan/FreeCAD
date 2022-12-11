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
#include <Base/Reader.h>
#include <Base/Writer.h>

#include "SymbolPort.h"
#include "SymbolPortPy.h"

using namespace TechDraw;

TYPESYSTEM_SOURCE(TechDraw::SymbolPort, Base::Persistence)

SymbolPort::SymbolPort(std::string name,
               Base::Vector3d anchor,
               int maxConnect,
               int flags)
{
    m_name = name;
    m_anchor = anchor;
    m_maxConnect = maxConnect;
    m_flags = flags;
}

void SymbolPort::Save(Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<SymbolPort>" << std::endl;
    writer.incInd();
    writer.Stream() << writer.ind() << "<Name value=\"" <<  m_name << "\"/>" << std::endl;
    writer.Stream() << writer.ind() << "<Anchor "
                << "X=\"" <<  m_anchor.x <<
                "\" Y=\"" <<  m_anchor.y <<
                "\" Z=\"" <<  m_anchor.z <<
                 "\"/>" << std::endl;
    writer.Stream() << writer.ind() << "<MaxConnect value=\"" <<  m_maxConnect << "\"/>" << std::endl;
    writer.Stream() << writer.ind() << "<Flags value=\"" <<  m_flags << "\"/>" << std::endl;
    writer.decInd();
    writer.Stream() << writer.ind() << "</SymbolPort>" << std::endl;
}

void SymbolPort::Restore(Base::XMLReader &reader)
{
    reader.readElement("SymbolPort");
    reader.readElement("Name");
    m_name = reader.getAttribute("value");
    reader.readElement("Anchor");
    m_anchor.x = reader.getAttributeAsFloat("X");
    m_anchor.y = reader.getAttributeAsFloat("Y");
    m_anchor.z = reader.getAttributeAsFloat("Z");
    reader.readElement("MaxConnect");
    m_maxConnect = reader.getAttributeAsInteger("value");
    reader.readElement("Flags");
    m_flags = reader.getAttributeAsInteger("value");
    reader.readEndElement("SymbolPort");
}

void SymbolPort::dump(const char* title)
{
    Base::Console().Message("SP::dump - %s \n", title);
    Base::Console().Message("SP::dump - %s \n", toString().c_str());
}

std::string SymbolPort::toString() const
{
    std::stringstream ss;
    ss << m_name << ", " <<
          "(" << m_anchor.x << ", " <<
          m_anchor.x << ", " <<
          m_anchor.y << ", " <<
          m_anchor.z << "), " <<
          m_maxConnect << ", " <<
          m_flags;
    return ss.str();
}

PyObject *SymbolPort::getPyObject(void)
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new SymbolPortPy(this), true);
    }

    return Py::new_reference_to(PythonObject);
}
