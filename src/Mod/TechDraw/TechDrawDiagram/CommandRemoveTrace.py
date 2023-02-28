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
"""Provides the TechDraw RemoveTrace GuiCommand."""

__title__ = "TechDrawDiagram.CommandRemoveTrace"
__author__ = "WandererFan"
__url__ = "https://www.freecadweb.org"
__version__ = "00.01"
__date__ = "2022/12/31"

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

class CommandRemoveTrace:
    """Remove a Trace from the current Diagram."""

    def __init__(self):
        """Initialize variables for the command that must exist at all times."""
        pass

    def GetResources(self):
        """Return a dictionary with data that will be used by the button or menu item."""
        return {'Pixmap': 'actions/TechDraw_DiagramTraceRemove.svg',
                'Accel': "",
                'MenuText': QT_TRANSLATE_NOOP("TechDraw_RemoveTrace", "Remove Trace"),
                'ToolTip': QT_TRANSLATE_NOOP("TechDraw_RemoveTrace", "Remove a Trace from the current Diagram")}


    def Activated(self):
        """Run the following code when the command is activated (button press)."""
        # print("TechDraw CommandRemoveTrace Activated()")
        diagram = TDDiagramUtil.findDiagram()
        if not diagram:
            return

        # find selected pathpainters in scene and delete them or display a selection task
        tracePainterType = TDDiagramTypeManager.getTypeByName("PathPainter")
        sceneItems = TDG.getSceneForPage(diagram).selectedItems()
        # there's a pythonic way to do this :(
        pathsInScene = list()
        for item in sceneItems:
            if item.type() == tracePainterType:
                pathsInScene.append(item)

        if not pathsInScene:
            self.ui = TechDrawDiagram.TaskRemoveTrace(diagram)
            Gui.Control.showDialog(self.ui)
            return

        for item in pathsInScene:
            TDDiagramWorkers.removeTrace(diagram, item.Id, item)


    def IsActive(self):
        """Return True when the command should be active or False when it should be disabled (greyed)."""
        if App.ActiveDocument:
            return TechDrawDiagram.TDDiagramUtil.haveDiagram()
        else:
            return False


#
# The command must be "registered" with a unique name by calling its class.
Gui.addCommand('TechDraw_RemoveTrace', CommandRemoveTrace())

