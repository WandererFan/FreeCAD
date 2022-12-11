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
"""Provides worker routines for TechDraw.Diagram"""

from pathlib import Path

import FreeCAD as App
import TechDraw as TD
import TechDrawGui as TDG
from PySide import QtCore
from PySide.QtCore import Slot
from TechDrawDiagram import TDDiagramTypeManager
from TechDrawDiagram import TDDiagramUtil
from TechDrawDiagram import Rez
from TechDrawDiagram.TDDiagramPainters import PathPainter
from TechDrawDiagram.TDDiagramPainters import SvgPainter
from TechDrawDiagram.TDDiagramPainters import TracePencil


# create a new diagram in the current document
def diagramNew():
    diagram = App.activeDocument().addObject('TechDraw::Diagram', "Diagram")
    template = App.activeDocument().addObject('TechDraw::DrawSVGTemplate', "Template")

    params = App.ParamGet("User parameter:BaseApp/Preferences/Mod/TechDraw/Files")
    default = App.getResourceDir() + "Mod/TechDraw/Templates/A4_LandscapeTD.svg"
    templateFile = params.GetString("TemplateFile", default)
    template.Template = templateFile

    diagram.Template = template
    diagram.ViewObject.show()

    return


# place a copy of a symbol (already in diagram) on the diagram
def symbolRubberStamp(diagram, symbolId):
#    print("TDDiagramWorkers.symbolRubberStamp()")
    # get a new symbol from diagram
    copySymbolId = diagram.copySymbol(symbolId)
    symbol = diagram.getSymbol(copySymbolId)

    # make a painter for the symbol
    painter = SvgPainter.SvgPainter()
    painter.setSymbol(symbol)
    painter.setId(symbol.SymbolId)
    painter.setName(symbol.Name)
    painter.load(symbol.Representation.FileReference)

    # add symbol to scene
    scene = TDG.getSceneForPage(diagram)
    scene.addItem(painter)

    painter.setPos(Rez.guiX(diagram.PageWidth) / 2, -Rez.guiX(diagram.PageHeight) / 2)


# add a symbol to the diagram
def symbolAdd(diagram, symbolFile):
    # print("TDDiagramWorkers.symbolAdd()")
    location = App.Vector(Rez.guiX(diagram.PageWidth) / 2, -Rez.guiX(diagram.PageHeight) / 2, 0.0)

    # add symbol to diagram
    symbolName = Path(symbolFile).stem

    representation = TD.SymbolRepresentation()
    representation.FileReference = symbolFile
    newSymbol = TD.DiagramSymbol(symbolName, representation, location, 0)

    symbolId = diagram.addSymbol(newSymbol)
    return symbolId


# repaint the entire diagram
def repaintDiagram(diagram):
    # print("workers.repaintDiagram()")
    scene = TDG.getSceneForPage(diagram)
    if not scene:
        print("Error: no scene for diagram")
        return
    for symbol in diagram.Symbols:
        # make a painter for the symbol
        painter = SvgPainter.SvgPainter()
        painter.setId(symbol.SymbolId)
        painter.setName(symbol.Name)
        painter.setSymbol(symbol)
        painter.load(symbol.Representation.FileReference)
        # add symbol to scene
        scene.addItem(painter)
        location = symbol.Location
        painter.setPos(location.x, location.y)

    for trace in diagram.Traces:
        # make a painter for the trace
        # print(dir(PathPainter))
        painter = PathPainter.PathPainter()
        painter.setId(trace.TraceId)
        # painter.setName(trace.Name)
        painter.setOwner(trace)
        points = list()
        for waypoint in trace.Route.Waypoints:
            newPoint = QtCore.QPointF(waypoint.Location.x, waypoint.Location.y)
            points.append(newPoint)
        painter.load(points)
        # add trace to scene
        scene.addItem(painter)
        # trace.pos() is (0, 0)


class TracePencilController(QtCore.QObject):
    signalFinished = QtCore.Signal()

    def __init__(self, diagram, fromSymbol=None, fromPort=None, parent=None):
        super().__init__(parent)
        self.finished = False
        self.points = list()
        self.diagram = diagram
        self.scene = TDG.getSceneForPage(diagram)
        self.fromSymbol = fromSymbol
        self.fromPort = fromPort
        self.startPos = App.Vector(0, 0, 0)
        if fromSymbol:
            self.startPos = fromSymbol.Location
        if fromPort:
            self.startPos += fromPort.Anchor
        self.tracePencil = TracePencil.TracePencil()
        # self.tracePencil.hide()
        self.connectToSignal()

    @Slot()
    def slotFinishedSignalFromPencil(self):
        # print("TPC.slotFinishedSignalFromPencil()")
        self.drawingFinished()

    def connectToSignal(self):
        # print("TPC.connectToSignal()")
        self.tracePencil.signalFinished.connect(self.slotFinishedSignalFromPencil)

    def drawingFinished(self):
        # print("TPC.drawingFinished()")
        waypoints = list()
        self.points = self.tracePencil.getPoints()
        self.finished = True
        self.signalFinished.emit()

    def isFinished(self):
        return self.finished

    def getPoints(self):
        return self.points

    def setFrom(self, symbol, port):
        self.fromSymbol = symbol
        self.fromPort = port
        self.startPos = self.fromSymbol.Location
        if port:
            self.startPos += self.fromPort.Anchor

    def begin(self):
        # print("TPC.begin()")
        self.tracePencil.setCaller(self)
        self.scene.addItem(self.tracePencil)
        self.tracePencil.setPos(self.startPos.x, self.startPos.y)
        self.tracePencil.show()

    def goAway(self):
        # print("TPC.goAway()")
        self.tracePencil.hide()
        self.scene.removeItem(self.tracePencil)


# make a TraceRoute from parameters
def makeTraceRoute(diagram, points):
    # print("workers.makeTraceRoute()")
    waypoints = list()
    for p in points:
        print("point: {0}".format(p))
        newPoint = TD.TraceWaypoint()
        newPoint.Location = TDDiagramUtil.toVector3d(p)
        waypoints.append(newPoint)
    newRoute = TD.TraceRoute()
    newRoute.Waypoints = waypoints
    return newRoute


# place a copy of a trace (already in diagram) on the diagram
def tracePlace(diagram, traceId):
    # print("TDDiagramWorkers.tracePlace()")
    # get trace from diagram
    trace = diagram.getTrace(traceId)

    # make a painter for the trace
    painter = PathPainter.PathPainter()
    painter.setId(trace.TraceId)
    points = list()
    for waypoint in trace.Route.Waypoints:
        newPoint = QtCore.QPointF(waypoint.Location.x, waypoint.Location.y)
        points.append(newPoint)
    painter.load(points)

    # add trace to scene
    scene = TDG.getSceneForPage(diagram)
    scene.addItem(painter)


# add a trace to the diagram
def traceAdd(diagram, route, name=None):
    # print("TDDiagramWorkers.traceAdd()")
    newTrace = TD.DiagramTrace()
    # newTrace.FromSymbol =
    # newTrace.FromPort =
    # newTrace.ToSymbol =
    # newTrace.ToPort =
    newTrace.Route = route

    traceId = diagram.addTrace(newTrace)
    return traceId


# redraw a trace after points have been changed
def redrawTrace(diagram, traceId):
    # print("TDDiagramWorkers.redrawTrace({0})".format(traceId))
    # get the new points
    trace = diagram.getTrace(traceId)
    points = list()
    for waypoint in trace.Route.Waypoints:
        newPoint = QtCore.QPointF(waypoint.Location.x, waypoint.Location.y)
        points.append(newPoint)
    # find painter for trace
    tracePainterType = TDDiagramTypeManager.getTypeByName("PathPainter")
    sceneItems = TDG.getSceneForPage(diagram).items()
    for item in sceneItems:
        itemTraceId = item.data(1)
        if item.type() == tracePainterType and item.data(1) == traceId:
            item.load(points)
            return

    return None
