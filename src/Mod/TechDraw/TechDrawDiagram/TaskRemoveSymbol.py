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
"""Provides the TechDraw RemoveSymbol Task Dialog."""

__title__ = "TechDrawDiagram.TaskRemoveSymbol"
__author__ = "WandererFan"
__url__ = "https://www.freecad.org"
__version__ = "00.01"
__date__ = "2023/02/25"

from PySide.QtCore import QT_TRANSLATE_NOOP
from PySide import QtCore
import PySide.QtGui as QtGui

import FreeCAD as App
import FreeCADGui as Gui

import TechDraw as TD
import TechDrawGui as TDG

from TechDrawDiagram import TDDiagramWorkers
from TechDrawDiagram import TDDiagramUtil

import os

class TaskRemoveSymbol:
    def __init__(self, diagram):
        self.diagram = diagram

        self._uiPath = App.getHomePath()
        self._uiPath = os.path.join(self._uiPath, "Mod/TechDraw/TechDrawDiagram/Gui/DiagramRemoveSymbol.ui")
        self.form = Gui.PySideUic.loadUi(self._uiPath)
        self.form.setWindowTitle(QT_TRANSLATE_NOOP("TechDraw_RemoveSymbol", "Remove Symbol From Diagram"))

        # shouldn't need these lists, but i can not make item.data(x) work :(
        self.idList = list()
        self.nameList = list()
        self.fillSymbolList()

        self.form.pbRemove.clicked.connect(self.slotRemoveSymbol)

        self.dialogOpen = False


    def accept(self):
        print ("Accept")
        return True


    def reject(self):
        print ("Reject")
        return True


    def slotRemoveSymbol(self):
        # print("TaskRemoveSymbol.slotRemoveSymbol(")
        if not self.form.lwSymbols.selectedItems():
            return

        selItem = self.form.lwSymbols.currentItem()
        selectedIndex = self.form.lwSymbols.currentRow()

        symbolId = self.idList[selectedIndex]
        symbolName = self.nameList[selectedIndex]
        TDDiagramWorkers.removeSymbol(self.diagram, symbolId)


    def fillSymbolList(self):
        # print("TaskRemoveSymbol.fillSymbolList()")
        self.form.lwSymbols.clear()
        self.idList.clear()
        self.nameList.clear()
        for symbol in self.diagram.Symbols:
            item = QtGui.QListWidgetItem()
            self.form.lwSymbols.addItem(item)
            item.setText("{0} - {1}".format(symbol.SymbolId, symbol.Name))
            self.idList.append(symbol.SymbolId)
            self.nameList.append(symbol.Name)

