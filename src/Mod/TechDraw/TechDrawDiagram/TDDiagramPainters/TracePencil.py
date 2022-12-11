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

'''provides an interactive painter to lay out a DiagramTrace'''

# painter item data(i) fields:
#   0 painter class name
#   1 painter data object id
#   2 name
#   n custom

from PySide import QtGui, QtCore
from PySide import QtSvg, QtWidgets
from PySide.QtCore import QObject, Signal, Slot  

import FreeCAD as App
import FreeCADGui as Gui
import TechDraw as TD
import TechDrawGui as TDG

from TechDrawDiagram import TDDiagramTypeManager

# supplies a cursor like item
class CursorItem(QtGui.QGraphicsItem):
    Type = TDDiagramTypeManager.getTypeByName("CursorItem")

    def __init__(self, parent = None):
        super().__init__(parent)
        self.setData(0, "CursorItem")
        self.setFlag(QtGui.QGraphicsItem.ItemSendsGeometryChanges, True)
        self.cursorSize = 50
        self.centerGap = self.cursorSize * 0.2

    def type(self):
        return CursorItem.Type

    def boundingRect(self):
        penWidth = 4
        return QtCore.QRectF(-self.cursorSize / 2 - penWidth / 2, -self.cursorSize / 2 - penWidth / 2,  
                      self.cursorSize + penWidth, self.cursorSize + penWidth)

    def setCursorSize(size):
        self.cursorSize = size

    def getCursorSize():
        return self.cursorSize

    #TODO: there should be a way to reuse existing cursor shapes here
    def paint(self, painter, option, widget):
        painter.drawRoundedRect(-self.cursorSize / 2, -self.cursorSize / 2, self.cursorSize, self.cursorSize, 50, 50)
        painter.drawLine(-self.cursorSize / 2, 0, -self.centerGap, 0)
        painter.drawLine(self.cursorSize / 2, 0, self.centerGap, 0)
        painter.drawLine(0, -self.cursorSize / 2, 0, -self.centerGap)
        painter.drawLine(0, self.cursorSize / 2, 0, self.centerGap)

# Builds a path by clicking and dragging.  Esc ends the drawing.
class TracePencil(QtGui.QGraphicsObject):
    signalFinished = QtCore.Signal()
    Type = TDDiagramTypeManager.getTypeByName("TracePencil")

    def __init__(self, parent = None):
        print("TDPainters.TracePencil()")
        super().__init__(parent)
        #self.Type = QtGui.QGraphicsItem.UserType + TD.UserTypePy + TracePencilType;
        self.setData(0, "TracePencil")

        self.setZValue(500)

        self.points = list()
        self.caller = None

        self.pathItem = QtGui.QGraphicsPathItem(self)
        self.pathItem.setPos(0,0)
        self.reticuleItem = CursorItem(self)
        self.reticuleItem.setPos(0,0)
        self.reticuleItem.setFlag(QtGui.QGraphicsItem.ItemSendsGeometryChanges, True)

        self.setFlag(QtGui.QGraphicsItem.ItemIsFocusable, True)
        self.setFlag(QtGui.QGraphicsItem.ItemIsSelectable, True)
        self.setFlag(QtGui.QGraphicsItem.ItemIsMovable, True)
        self.setFlag(QtGui.QGraphicsItem.ItemSendsGeometryChanges, True)
        self.setFlag(QtGui.QGraphicsItem.ItemIsFocusable, True)

        self.oldPosition = QtCore.QPoint(0.0, 0.0)
        self.mode = "idle"

        self.dragStart = self.pos()
        self.dragEnd = self.pos()
        path = QtGui.QPainterPath()
        self.pathItem.setPath(path)

    def updatePath(self):
        path = QtGui.QPainterPath()
        path.moveTo(self.mapFromScene(self.points[0]))
        for point in self.points[1:]:
            path.lineTo(self.mapFromScene(point))
        path.lineTo(self.mapFromScene(self.dragEnd))
        self.pathItem.setPath(path)

    def type(self):
        return TracePencil.Type

    def boundingRect(self):
        # childrenBoundingRect()?
        rect = QtCore.QRectF()
        for child in self.childItems():
            rect = rect.united(child.boundingRect())
        return rect

    def paint(self, painter, option, widget):
        option.state = option.state & ~QtGui.QStyle.State_Selected
        #self.pathItem.update()  #?
        for child in self.childItems():
            child.paint(painter, option, widget)

    def itemChange(self, change, value):
        if change == QtGui.QGraphicsItem.ItemPositionChange:
            # value is the new position.
            if (value - self.oldPosition).manhattanLength() > 100 :
                #print("I've moved to {0}".format(value)) 
                self.oldPosition = value

            rect = self.scene().sceneRect()
            if not rect.contains(value):
                # Keep the item inside the scene rect.
                #print("boundary control active")
                value.setX(qMin(rect.right(), qMax(value.x(), rect.left())))
                value.setY(qMin(rect.bottom(), qMax(value.y(), rect.top())))
            return value

        if change == QtGui.QGraphicsItem.ItemSelectedHasChanged :
            if not value:
                print("TP.itemChange - not selected - end drawing?")
            return value

        return super().itemChange(change, value)

    def mousePressEvent(self, event):
        print("TracePencil.mousePressEvent - button: {0}".format(event.button()))
        if event.button() == QtCore.Qt.RightButton:
            self.onDrawingFinished()
            self.mode = "idle"
            return

        self.dragStart = event.scenePos()
        # self.reticuleItem.setPos(self.mapFromScene(self.dragStart))
        if self.mode == "idle":
            self.mode = "drawing"
            self.setFocus()
            self.points.append(self.dragStart)
        super().mousePressEvent(event)

    def mouseMoveEvent(self, event):
        if self.mode == "drawing":
            self.dragEnd = event.scenePos()
            # self.reticuleItem.setPos(self.mapFromScene(self.dragEnd))
            self.updatePath()
        super().mouseMoveEvent(event)

    def mouseReleaseEvent(self, event):
        print("TracePencil.mouseReleaseEvent - button: {0}".format(event.button()))
        if event.button() == QtCore.Qt.RightButton:     #blocked by context menu? sometimes?
            self.onDrawingFinished()
            self.mode = "idle"
    
        if self.mode == "drawing":
            self.dragEnd = event.scenePos()
            # self.reticuleItem.setPos(self.mapFromScene(self.dragEnd))
            self.points.append(self.dragEnd)
            self.updatePath()
        super().mouseReleaseEvent(event)

    def keyReleaseEvent(self, event):
        print("TracePencil.keyReleaseEvent - key: {0}".format(hex(event.key())))
        if event.key() == QtCore.Qt.Key_Escape:
            self.onDrawingFinished()
            self.mode = "finished"
        super().keyReleaseEvent(event)

    def onDrawingFinished(self):
        print("TracePencil.onDrawingFinished()")
        self.reticuleItem.hide()
        self.signalFinished.emit()

    def setCaller(self, caller):
        self.caller = caller

    def getPoints(self):
        return self.points

