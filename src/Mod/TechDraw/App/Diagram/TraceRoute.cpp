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

#include "TraceRoute.h"
#include "TraceRoutePy.h"

using namespace TechDraw;

TYPESYSTEM_SOURCE(TechDraw::TraceRoute, Base::Persistence)

TraceRoute::TraceRoute(TraceWaypointList waypoints, int flags)
{
    m_waypoints = waypoints;
    m_flags = flags;
}

void TraceRoute::Save(Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<TraceRoute>" << std::endl;
    writer.incInd();
    writer.Stream()
         << writer.ind()
             << "<Waypoints "
             << "PointCount=\""
             << m_waypoints.size()
             << "\">" << std::endl;

    writer.incInd();
    for (auto& point: m_waypoints) {
        point->Save(writer);
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</Waypoints>" << std::endl ;

    writer.Stream() << writer.ind() << "<Flags value=\"" <<  m_flags << "\"/>" << std::endl;
    writer.decInd();
    //TODO: why do we need to add the closing entry only for some objects?
    writer.Stream() << writer.ind() << "</TraceRoute>" << std::endl;
}

void TraceRoute::Restore(Base::XMLReader &reader)
{
    reader.readElement("TraceRoute");
    reader.readElement("Waypoints");
    int count = reader.getAttributeAsInteger("PointCount");
    int i = 0;
    for ( ; i < count; i++) {
        TraceWaypoint* newPoint = new TraceWaypoint;
        newPoint->Restore(reader);
        m_waypoints.push_back(newPoint);
    }
    reader.readEndElement("Waypoints");

    reader.readElement("Flags");
    m_flags = reader.getAttributeAsInteger("value");
    reader.readEndElement("TraceRoute");
}

void TraceRoute::dump(const char* title)
{
    Base::Console().Message("TR:dump - %s \n", title);
    Base::Console().Message("TR::dump - %s \n", toString().c_str());
}

std::string TraceRoute::toString() const
{
    std::stringstream ss;
    for (auto& point : m_waypoints) {
        ss << point->toString() << ", ";
    }
    ss << m_flags;
    return ss.str();
}

PyObject *TraceRoute::getPyObject(void)
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new TraceRoutePy(this), true);
    }

    return Py::new_reference_to(PythonObject);
}

