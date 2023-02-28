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
"""Provides the TechDraw RemoveSymbol GuiCommand."""

__title__ = "TechDrawDiagram.CommandRemoveSymbol"
__author__ = "WandererFan"
__url__ = "https://www.freecad.org"
__version__ = "00.01"
__date__ = "2023/02/25"

from PySide.QtCore import QT_TRANSLATE_NOOP
from PySide import QtCore
import PySide.QtGui as QtGui
from PySide.QtCore import QObject, Signal, Slot

import FreeCAD as App
import FreeCADGui as Gui

import TechDraw as TD
import TechDrawGui as TDG

import TechDrawDiagram
from TechDrawDiagram import TDDiagramWorkers
from TechDrawDiagram import TDDiagramUtil
from TechDrawDiagram import TDDiagramTypeManager

class CommandRemoveSymbol:
    """Remove a Symbol from the current Diagram."""

    def __init__(self):
        """Initialize variables for the command that must exist at all times."""
        pass

    def GetResources(self):
        """Return a dictionary with data that will be used by the button or menu item."""
        return {'Pixmap': 'actions/TechDraw_DiagramSymbolRemove.svg',
                'Accel': "",
                'MenuText': QT_TRANSLATE_NOOP("TechDraw_RemoveSymbol", "Remove Symbol"),
                'ToolTip': QT_TRANSLATE_NOOP("TechDraw_RemoveSymbol", "Remove a Symbol from the current Diagram")}


    def Activated(self):
        """Run the following code when the command is activated (button press)."""
        # print("TechDraw CommandRemoveSymbol Activated()")
        diagram = TDDiagramUtil.findDiagram()
        if not diagram:
            return

        # find selected symbol painters in scene and delete them or display a selection task
        symbolPainterType = TDDiagramTypeManager.getTypeByName("SvgPainter")
        sceneItems = TDG.getSceneForPage(diagram).selectedItems()
        # there's a pythonic way to do this :(
        symbolsInScene = list()
        for item in sceneItems:
            if item.type() == symbolPainterType:
                symbolsInScene.append(item)

        if not symbolsInScene:
            self.ui = TechDrawDiagram.TaskRemoveSymbol(diagram)
            Gui.Control.showDialog(self.ui)
            return

        for item in symbolsInScene:
            TDDiagramWorkers.removeSymbol(diagram, item.Id, item)


    def IsActive(self):
        """Return True when the command should be active or False when it should be disabled (greyed)."""
        if App.ActiveDocument:
            return TechDrawDiagram.TDDiagramUtil.haveDiagram()
        else:
            return False


#
# The command must be "registered" with a unique name by calling its class.
Gui.addCommand('TechDraw_RemoveSymbol', CommandRemoveSymbol())

