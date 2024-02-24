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

//! HandlerTable.h
//! a convenience for passing callbacks around

#ifndef MEASURE_HANDLERTABLE_H
#define MEASURE_HANDLERTABLE_H

#include <Mod/Measure/MeasureGlobal.h>

#include <string>
#include <functional>
#include "MeasureInfo.h"

namespace Measure {

using CallbackItem = std::function<typename Measure::MeasureInfo* (std::string*, std::string*)>;

class MeasureExport HandlerEntry {
public:
    HandlerEntry() = default;
    HandlerEntry(std::string type, std::string modName, CallbackItem cb) { measureType = type; moduleName = modName; callback = cb;}
    ~HandlerEntry() = default;
    
    std::string measureType;
    std::string moduleName;
    CallbackItem callback;
    
};

using CallbackTable = std::vector<HandlerEntry>;

}  //end namespace Measure

#endif

