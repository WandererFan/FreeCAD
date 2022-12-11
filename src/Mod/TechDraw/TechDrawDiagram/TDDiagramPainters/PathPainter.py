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
"""Provides a general purpose QPathItem from points painter for TechDraw.Diagram"""

# painter item data(i) fields:
#   0 painter class name
#   1 painter data object id
#   2 name
#   n custom

import os

from PySide import QtGui, QtCore
from PySide import QtWidgets

import FreeCAD as App
import FreeCADGui as Gui
import TechDraw as TD
import TechDrawGui as TDG

from TechDrawDiagram import TDDiagramTypeManager

class PathPainter(QtGui.QGraphicsPathItem):
    Type = TDDiagramTypeManager.getTypeByName("PathPainter")

    def __init__(self, parent = None):
        super().__init__(parent)
        self.setData(0, "PathPainter")

        self.Id = 0
        self.Owner = None
        self.setZValue(500)
        self.points = list()
        color = QtGui.QColor()
        color.setNamedColor("red")
        self.Pen = QtGui.QPen(color)
        self.Pen.setWidth(4.0)
        self.setPen(self.Pen)

        self.setFlag(QtGui.QGraphicsItem.ItemIsFocusable, True)
        self.setFlag(QtGui.QGraphicsItem.ItemIsSelectable, True)
        self.setFlag(QtGui.QGraphicsItem.ItemIsMovable, False)
        self.setFlag(QtGui.QGraphicsItem.ItemSendsGeometryChanges, True)

    def type(self):
        return PathPainter.Type


    def itemChange(self, change, value):
        if change == QtGui.QGraphicsItem.ItemPositionChange:
            # value is the new position.
            # emit a signal to redraw attached items?
            return value

        if change == QtGui.QGraphicsItem.ItemSelectedHasChanged :
            #value is selection state
            
            return value

        return super().itemChange(change, value)


    def load(self, points):
        # print("Painters.PathPainter.load()")
        path = QtGui.QPainterPath()
        path.moveTo(points[0])
        for point in points[1:]:
            path.lineTo(point)
        self.setPath(path)


    def setId(self, pathId):
        self.Id = pathId
        self.setData(1, pathId)

    def setOwner(self, owner):
        self.Owner = owner
        
    def setWidth(self, width):
        self.Pen.setWidth(width)
        self.setPen(self.Pen)

    def setColor(self, color):
        self.Pen.setColor(color)
        self.setPen(self.Pen)
        


