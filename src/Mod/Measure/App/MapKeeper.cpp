// SPDX-License-Identifier: LGPL-2.0-or-later

/***************************************************************************
 *   Copyright (c) 2024 wandererfan <wandererfan at gmail dot com>         *
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

// a place to keep the maps of geometry handlers for various measure types and
// modules.

 
#include "MapKeeper.h"

using namespace Measure;

//! create or replace the callback for a modName
void MapKeeper::addCallback(const std::string& MeasureType, const std::string& modName, GeometryHandlerCB callback)
{
    auto thisMap = Map(MeasureType);
    (*thisMap) [modName] = callback;

}
//! assign many modNames to the same callback
void MapKeeper::addCallbacks(const std::string& MeasureType, const std::vector<std::string>& modNames, GeometryHandlerCB callback)
{
    auto thisMap = Map(MeasureType);
    for (auto& name : modNames) {
        (*thisMap) [name] = callback;
    }
}

//! get the address of callback std::function
GeometryHandlerCB MapKeeper::getCallback(const std::string& MeasureType, const std::string& modName)
{
    auto thisMap = Map(MeasureType);
    return (*thisMap) [modName];

}

//! true if the MeasureType map contains an entry for modName
bool MapKeeper::hasCallback(const std::string& MeasureType, const std::string& modName)
{
    auto thisMap = Map(MeasureType);
    return ((*thisMap).count(modName) > 0);
}

//! return the callback map for MeasureType
HandlerMapPtr MapKeeper::Map(const std::string& MeasureType)
{
    if (MeasureType == "Length") {
        return &m_mapLength;
    }
    if (MeasureType == "Angle") {
        return &m_mapAngle;
    }    
    if (MeasureType == "Area") {
        return &m_mapArea;
    }
    if (MeasureType == "Distance") {
        return &m_mapDistance;
    }
    if (MeasureType == "Position") {
        return &m_mapPosition;
    }
    if (MeasureType == "Radius") {
        return &m_mapRadius;
    }
    
    return nullptr;
}


//! load the individual tables from info provided by module
void MapKeeper::loadMaps(CallbackTable table)
{
    for (auto& entry : table) {
        if (entry.measureType == "Length") {
            m_mapLength[entry.moduleName] = entry.callback;
            continue;
        }
        if (entry.measureType == "Angle") {
            m_mapAngle[entry.moduleName] = entry.callback;
            continue;
        }
        if (entry.measureType == "Area") {
            m_mapArea[entry.moduleName] = entry.callback;
            continue;
        }
        if (entry.measureType == "Distance") {
            m_mapDistance[entry.moduleName] = entry.callback;
            continue;
        }
        if (entry.measureType == "Position") {
            m_mapPosition[entry.moduleName] = entry.callback;
            continue;
        }
        if (entry.measureType == "Radius") {
            m_mapRadius[entry.moduleName] = entry.callback;
        }
    }
}

