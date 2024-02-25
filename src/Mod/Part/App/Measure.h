/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
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

#ifndef PART_MEASURE_H
#define PART_MEASURE_H

#include <Mod/Part/PartGlobal.h>
#include <functional>
#include <string>

#include <Mod/Measure/App/MeasureAngle.h>
#include <Mod/Measure/App/MeasureDistance.h>
#include <Mod/Measure/App/MeasurePosition.h>
#include <Mod/Measure/App/MeasureLength.h>
#include <Mod/Measure/App/MeasureArea.h>
#include <Mod/Measure/App/MeasureRadius.h>
#include <Mod/Measure/App/MeasureInfo.h>

namespace Part
{

using GeometryHandler = std::function<Measure::MeasureInfo* (std::string*, std::string*)>;
using CallbackEntry = std::pair<std::string, GeometryHandler>;
using CallbackTable = std::vector<CallbackEntry>;

class PartExport Measure
{
public:

    static void initialize();
    static Part::CallbackTable  reportLengthCallbacks();
    static Part::CallbackTable  reportAngleCallbacks();
    static Part::CallbackTable  reportAreaCallbacks();
    static Part::CallbackTable  reportDistanceCallbacks();
    static Part::CallbackTable  reportPositionCallbacks();
    static Part::CallbackTable  reportRadiusCallbacks();
};


} //namespace Part

#endif
