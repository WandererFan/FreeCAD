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
"""Provides the TechDraw AddSymbol Task Dialog."""

__title__ = "TechDrawDiagram.TaskAddSymbol"
__author__ = "WandererFan"
__url__ = "https://www.freecadweb.org"
__version__ = "00.01"
__date__ = "2022/01/11"

import os

from PySide.QtCore import QT_TRANSLATE_NOOP
from PySide import QtCore
import PySide.QtGui as QtGui

import FreeCAD as App
import FreeCADGui as Gui

import TechDraw as TD
import TechDrawGui as TDG

from TechDrawDiagram import TDDiagramWorkers
from TechDrawDiagram import TDDiagramUtil

class TaskAddSymbol:
    def __init__(self, diagram):
        self.diagram = diagram
        self._uiPath = App.getHomePath()
        self._uiPath = os.path.join(self._uiPath, "Mod/TechDraw/TechDrawDiagram/Gui/DiagramAddSymbol.ui")
        self.form = Gui.PySideUic.loadUi(self._uiPath)
        self.form.setWindowTitle(QT_TRANSLATE_NOOP("TechDraw_AddSymbol", "Add Symbol to Diagram"))

        # shouldn't need these lists, but QListWidgetItem().data(x) doesn't seem to work correctly
        self.idList = list()
        self.nameList = list()
        self.fillSymbolList()

        self.form.fcFile.fileNameSelected.connect(self.pickFromFile)
        self.form.pbPlace.clicked.connect(self.placeSymbol)

        self.dialogOpen = False;

    def accept(self):
        print ("TaskAddSymbol.Accept()")
        return True

    def reject(self):
        print ("TaskAddSymbol.Reject()")
        return True

    def placeSymbol(self):
        #print("TaskAddSymbol.placeSymbol()")
        if not self.form.lwSymbols.selectedItems():
            return

        selItem = self.form.lwSymbols.currentItem()
        selectedIndex = self.form.lwSymbols.currentRow()

        symbolId = self.idList[selectedIndex]
        symbolName = self.nameList[selectedIndex]
        TDDiagramWorkers.symbolRubberStamp(self.diagram, symbolId)

    def pickFromFile(self, fileName):
        #print("TaskAddSymbol.pickFromFile()")
        TDDiagramWorkers.addSymbol(self.diagram, fileName)
        self.fillSymbolList()

    def fillSymbolList(self):
        #print("TaskAddSymbol.fillSymbolList()")
        self.form.lwSymbols.clear()
        self.idList.clear()
        self.nameList.clear()
        for symbol in self.diagram.Symbols:
            item = QtGui.QListWidgetItem()
            self.form.lwSymbols.addItem(item)
            item.setText(symbol.Name)
            self.idList.append(symbol.SymbolId)
            self.nameList.append(symbol.Name)

