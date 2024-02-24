// SPDX-License-Identifier: LGPL-2.0-or-later

/***************************************************************************
 *   Copyright (c) 2024 wandererfan <wandererfan at gmail dot com>         *
 *                                                                         *
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


#ifndef MEASURE_MAPKEEPER_H
#define MEASURE_MAPKEEPER_H

#include <Mod/Measure/MeasureGlobal.h>

#include <functional>
#include <map>

#include "MeasureInfo.h"
#include "HandlerTable.h"

namespace Measure
{

using GeometryHandlerCB = std::function<MeasureInfo* (std::string*, std::string*)>;
using HandlerMap = std::map<std::string, GeometryHandlerCB>;
using HandlerMapPtr = HandlerMap*;


class MeasureExport MapKeeper
{
public:
    //! create or replace the callback for a module
    static void addCallback(const std::string& MeasureType, const std::string& module, GeometryHandlerCB callback);

    //! assign many modules to the same callback
    static void addCallbacks(const std::string& MeasureType, const std::vector<std::string>& modules, GeometryHandlerCB callback);

    //! get the address of callback std::function
    static GeometryHandlerCB getCallback(const std::string& MeasureType, const std::string& module);

    //! true if the MeasureType map contains an entry for module
    static bool hasCallback(const std::string& MeasureType, const std::string& module);

    //! load the individual tables from info provided by module
    void loadMaps(CallbackTable table);

private:
    //! return the callback map for MeasureType
    static HandlerMapPtr Map(const std::string& MeasureType);

    inline static HandlerMap m_mapLength = HandlerMap();
    inline static HandlerMap m_mapAngle= HandlerMap();
    inline static HandlerMap m_mapArea = HandlerMap();
    inline static HandlerMap m_mapDistance = HandlerMap();
    inline static HandlerMap m_mapPosition = HandlerMap();
    inline static HandlerMap m_mapRadius = HandlerMap();
    
};


} //namespace Measure


#endif // MEASURE_MAPKEEPER_H

