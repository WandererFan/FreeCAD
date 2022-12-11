# ***************************************************************************
# *   Copyright (c) 2022 Wanderer Fan <wandererfan@gmail.com>               *
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
"""Provides utility functions for TD Diagram."""

from PySide import QtCore

import FreeCAD as App

def haveDiagram():
    for object in App.ActiveDocument.Objects:
        if object.isDerivedFrom("TechDraw::Diagram"):
            return True
    return False

#from DrawGuiUtil::findPage
#finds the first diagram in the active document.  Should look through all the open
#documents looking for a diagram, and if more than 1 is found, ask for help
def findDiagram():
    for object in App.ActiveDocument.Objects:
        if object.isDerivedFrom("TechDraw::Diagram"):
            return object
    return None

#returns App.Vector given a QPointF
def toVector3d(pointf):
    return App.Vector(pointf.x(), pointf.y(), 0.0)
