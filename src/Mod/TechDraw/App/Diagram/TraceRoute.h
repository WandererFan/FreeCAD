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
#ifndef TECHDRAW_TRACEROUTE_H
#define TECHDRAW_TRACEROUTE_H

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <App/FeaturePython.h>
#include <Base/Persistence.h>
#include <Base/Vector3D.h>

#include "TraceWaypoint.h"

namespace TechDraw {

//description of a path taken by a parent Trace
class TechDrawExport TraceRoute : Base::Persistence
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    TraceRoute() {}
    TraceRoute(TraceWaypointList waypoints, int flags = 0);
    ~TraceRoute() override = default;

    // setters & getters
    TraceWaypointList  getWaypoints() const { return m_waypoints; }
    void setWaypoints(TraceWaypointList  waypoints) { m_waypoints = waypoints; }
    int getFlags() const { return m_flags; }
    void setFlags(int flags) { m_flags = flags; }

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override  { return 1; }
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;

    PyObject *getPyObject() override;

    void dump(const char* title);
    std::string toString() const;

private:
    TraceWaypointList  m_waypoints;
    int m_flags;

    Py::Object PythonObject;
};

using TraceRoutePython = App::FeaturePythonT<TraceRoute>;

}

#endif //end TECHDRAW_TRACEROUTE_H
