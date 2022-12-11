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
"""Provides a general purpose Svg painter for TechDraw.Diagram"""

# painter item data(i) fields:
#   0 painter class name
#   1 painter data object id
#   2 name
#   n custom

import os

from PySide import QtGui, QtCore
from PySide import QtSvg, QtWidgets

import FreeCAD as App
import FreeCADGui as Gui
import TechDraw as TD
import TechDrawGui as TDG

from TechDrawDiagram import TDDiagramUtil
from TechDrawDiagram import TDDiagramTypeManager
from TechDrawDiagram import Rez


class SvgPainter(QtSvg.QGraphicsSvgItem):
    Type = TDDiagramTypeManager.getTypeByName("SvgPainter")
    
    def __init__(self, parent = None):
        super().__init__(parent)
        self.setData(0, "SvgPainter")
        
        self.Id = 0
        self.Name = "Name"
        self.Symbol = None
        self.setZValue(500)

        self.renderer = QtSvg.QSvgRenderer()        #this could be passed in order to share across items.
        self.setSharedRenderer(self.renderer)

        self.setFlag(QtGui.QGraphicsItem.ItemIsFocusable, True)
        self.setFlag(QtGui.QGraphicsItem.ItemIsSelectable, True)
        self.setFlag(QtGui.QGraphicsItem.ItemIsMovable, True)
        self.setFlag(QtGui.QGraphicsItem.ItemSendsGeometryChanges, True)

        self.oldPosition = QtCore.QPoint(0.0, 0.0)
        self.mode = "idle"

        self.dragStart = QtCore.QPoint(0, 0)
        self.dragEnd = QtCore.QPoint(0, 0)

        pxMm = 3.78;                 #96px/25.4mm ( CSS/SVG defined value of 96 pixels per inch)
        #pxMm = 3.54;                 //90px/25.4mm ( inkscape value version <= 0.91)
        svgScaleFactor = Rez.guiX(1.0) / pxMm
        self.setScale(svgScaleFactor)

        self.setElementId("")       #this is a kludge to get QGraphicsSvgItem to recalculate boundingRect


    def type(self):
        return SvgPainter.Type

    def itemChange(self, change, value):
        if change == QtGui.QGraphicsItem.ItemPositionChange:
            # value is the new position.
            if self.Symbol :
                # might need a distance threshold here to limit number of updates
                self.Symbol.Location = TDDiagramUtil.toVector3d(value)
            return value

        if change == QtGui.QGraphicsItem.ItemSelectedHasChanged :
            #value is selection state
            
            return value

        return super().itemChange(change, value)

    #TODO
    #def load(self, QXmlStreamReader)
    #def load(self, QByteArray)
    def load(self, fileName):
        if fileName:
            self.svgFileName = fileName
            self.renderer.load(fileName)
            self.setElementId("")   #this is a kludge to get QGraphicsSvgItem to recalculate boundingRect

    def setId(self, symbolId):
        self.Id = symbolId
        self.setData(1, symbolId)

    def setName(self, symbolName):
        self.Name = symbolName
        self.setData(2, symbolName)

    def setSymbol(self, symbol):
        self.Symbol = symbol

