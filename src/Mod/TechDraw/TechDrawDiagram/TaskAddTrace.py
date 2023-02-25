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
"""Provides the TechDraw AddTrace Task Dialog."""

__title__ = "TechDrawDiagram.TaskAddTrace"
__author__ = "WandererFan"
__url__ = "https://www.freecadweb.org"
__version__ = "00.01"
__date__ = "2022/01/11"

import os

from PySide.QtCore import QT_TRANSLATE_NOOP
from PySide import QtCore
import PySide.QtGui as QtGui
from PySide.QtCore import QObject, Signal, Slot


import FreeCAD as App
import FreeCADGui as Gui

import TechDraw as TD
import TechDrawGui as TDG

from TechDrawDiagram import TDDiagramWorkers
from TechDrawDiagram import TDDiagramUtil

class TaskAddTrace:
    def __init__(self, diagram):
        self.diagram = diagram
        self.controller = TDDiagramWorkers.TracePencilController(diagram)
        self._uiPath = App.getHomePath()
        self._uiPath = os.path.join(self._uiPath, "Mod/TechDraw/TechDrawDiagram/Gui/DiagramAddTrace.ui")
        self.form = Gui.PySideUic.loadUi(self._uiPath)
        self.form.setWindowTitle(QT_TRANSLATE_NOOP("TechDraw_AddTrace", "Add Trace to Diagram"))

        # shouldn't need these lists, but item.data(x) doesn't seem to work correctly
        self.idList = list()
        self.nameList = list()
        self.fillSymbolList()

        self.form.pbFromSymbol.clicked.connect(self.fromSymbolPicked)
        self.form.pbToSymbol.clicked.connect(self.toSymbolPicked)
        self.form.lwPorts.itemDoubleClicked.connect(self.portPicked)
        self.connectToSignal()
        
        self.stage = None
        self.fromSymbol = None
        self.fromPort = None
        self.toSymbol = None
        self.toPort = None
        self.points = list()

        self.dialogOpen = False;

    def accept(self):
        print ("TaskAddTrace.Accept()")
        return True

    def reject(self):
        print ("TaskAddTrace.Reject()")
        return True

    def connectToSignal(self):
        print("TPC.connectToSignal()")
        self.controller.signalFinished.connect(self.slotFinishedSignalFromController)

    def fromSymbolPicked(self):
        print("TaskAddTrace.fromSymbolPicked()")
        if not self.form.lwSymbols.selectedItems():
            return
        self.stage = "from"
        selItem = self.form.lwSymbols.currentItem()
        selectedIndex = self.form.lwSymbols.currentRow()

        symbolId = self.idList[selectedIndex]
        symbolName = self.nameList[selectedIndex]
        self.form.leFromSymbol.setText(symbolName)
        self.fromSymbol = self.diagram.getSymbol(symbolId)
        self.fillPortList(self.fromSymbol)
        print("TaskAddTrace.fromSymbolPicked - fromSymbol.Location: {0}".format(self.fromSymbol.Location))

    def toSymbolPicked(self):
        print("TaskAddTrace.toSymbolPicked()")
        if not self.form.lwSymbols.selectedItems():
            return
        self.stage = "to"
        selItem = self.form.lwSymbols.currentItem()
        selectedIndex = self.form.lwSymbols.currentRow()

        symbolId = self.idList[selectedIndex]
        symbolName = self.nameList[selectedIndex]
        self.form.leToSymbol.setText(symbolName)
        self.toSymbol = self.diagram.getSymbol(symbolId)
        self.fillPortList(self.toSymbol)

    def fillSymbolList(self):
        print("TaskAddTrace.fillSymbolList()")
        self.form.lwSymbols.clear()
        self.idList.clear()
        self.nameList.clear()
        for symbol in self.diagram.Symbols:
            item = QtGui.QListWidgetItem()
            self.form.lwSymbols.addItem(item)
            item.setText(symbol.Name)
            self.idList.append(symbol.SymbolId)
            self.nameList.append(symbol.Name)

    def fillPortList(self, symbol):
        print("TaskAddTrace.fillPortList()")
        self.form.lwPorts.clear()
        if not symbol.Ports:
            print("TaskAddTrace.fillPortList - adding default port to list")
            item = QtGui.QListWidgetItem()
            self.form.lwPorts.addItem(item)
            item.setText("default")
            return
            
        for port in symbol.Ports:
            print("TaskAddTrace.fillPortList - adding port to list from symbol")
            item = QtGui.QListWidgetItem()
            self.form.lwPorts.addItem(item)
            item.setText(port.Name)

    def portPicked(self, item):
        if self.stage == "from":
            self.form.leFromPort.setText(item.text())
            self.fromPort = self.fromSymbol.getPort(item.text())
        if self.stage == "to":
            self.form.leToPort.setText(item.text())
            self.toPort = self.toSymbol.getPort(item.text())
        if self.form.leFromPort.text() and self.form.leToPort.text():
            if self.fromPort == "default":
                self.controller.setFrom(self.fromSymbol)
            else:
                self.controller.setFrom(self.fromSymbol, self.fromPort)
            self.controller.begin()

    def drawingFinished(self):
        print("TaskAddTrace.drawingFinished()")
        route = TDDiagramWorkers.makeTraceRoute(self.diagram, self.controller.getPoints())
        traceId = TDDiagramWorkers.addTrace(self.diagram, route)
        trace = self.diagram.getTrace(traceId)
        trace.FromSymbol = self.fromSymbol.SymbolId
        trace.FromPort = self.fromPort.Name
        trace.ToSymbol = self.toSymbol.SymbolId
        trace.ToPort = self.toPort.Name
        #how to shut down controller?
        self.controller.goAway()
        TDDiagramWorkers.placeTrace(self.diagram, traceId)

    @Slot()
    def slotFinishedSignalFromController(self):
        print("TPC.slotFinishedSignalFromContoller()")
        self.drawingFinished()
