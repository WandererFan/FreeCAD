# ***************************************************************************
# *   Copyright (c) 2023 Wanderer Fan <wandererfan@gmail.com>               *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Custom QGraphicsItem type management for TD Diagram."""

from PySide import QtCore, QtGui

import FreeCAD as App

TDDUserTypePy = QtGui.QGraphicsItem.UserType + 10001;  #the start of our range for QGI types
TDDBadUserType = -1

#the original set of types
DiagramQGITypes = {
    "SvgPainter": 10,
    "TracePencil": 20,
    "CursorItem": 21,
    "PathPainter": 30
}

# get the type number for a type name
def getTypeByName(name):
    typenum = DiagramQGITypes.get(name)
    if not typenum:
        for key in DiagramQGITypes.keys():
            print(key)
        return TDDBadUserType
    return TDDUserTypePy + typenum

# get a type name from a type number
def getNameForType(type):
    if type > TDDUserTypePy:
        type = type - TDDUserTypePy
    for key, value in DiagramQGITypes.items():
        if value > type:
            return key
    return None

# add a new type
def addType(name, type):
    if getTypeByName(name) != TDDBadUserType:
        # can not add this as we already have this name
        # should this be 
        raise Exception("Type name already exists") 
        #return TDDBadUserType # ???
    if getNameForType(type) :
        # can not add this as we already have this type
        raise Exception("Type number already exists") 
        #return TDDBadUserType # ???
    DiagramQGITypes[name] = type
    return type
    
def getNextAvailableType():
    if not DiagramQGITypes:
        return 10
    # we have already assigned some types, so find the largest type number
    availType = 0
    for key, value in DiagramQGITypes.items():
        if value > availType:
            availType = value
    return availType + 10


