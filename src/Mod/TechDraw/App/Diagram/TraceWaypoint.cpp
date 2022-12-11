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

#include "TraceWaypoint.h"
#include "TraceWaypointPy.h"

using namespace TechDraw;

TYPESYSTEM_SOURCE(TechDraw::TraceWaypoint, Base::Persistence)

TraceWaypoint::TraceWaypoint(Base::Vector3d location, int flags)
{
    m_location = location;
    m_flags = flags;
}

void TraceWaypoint::Save(Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<TraceWaypoint>" << std::endl;
    writer.incInd();
    writer.Stream() << writer.ind() << "<Location "
                << "X=\"" <<  m_location.x <<
                "\" Y=\"" <<  m_location.y <<
                "\" Z=\"" <<  m_location.z <<
                 "\"/>" << std::endl;
    writer.Stream() << writer.ind() << "<Flags value=\"" <<  m_flags << "\"/>" << std::endl;
    writer.decInd();
    writer.Stream() << writer.ind() << "</TraceWaypoint>" << std::endl;
}

void TraceWaypoint::Restore(Base::XMLReader &reader)
{
    reader.readElement("TraceWaypoint");
    reader.readElement("Location");
    m_location.x = reader.getAttributeAsFloat("X");
    m_location.y = reader.getAttributeAsFloat("Y");
    m_location.z = reader.getAttributeAsFloat("Z");
    reader.readElement("Flags");
    m_flags = reader.getAttributeAsInteger("value");
    reader.readEndElement("TraceWaypoint");
}

void TraceWaypoint::dump(const char* title)
{
    Base::Console().Message("TW::dump - %s \n", title);
    Base::Console().Message("TW::dump - %s \n", toString().c_str());
}

std::string TraceWaypoint::toString() const
{
    std::stringstream ss;
    ss << "(" << m_location.x << ", " <<
          m_location.y << ", " <<
          m_location.z << "), " <<
          m_flags;
    return ss.str();
}

PyObject *TraceWaypoint::getPyObject(void)
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new TraceWaypointPy(this), true);
    }

    return Py::new_reference_to(PythonObject);
}

