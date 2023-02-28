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
"""Provides the TechDraw AddDiagram GuiCommand."""

__title__ = "TechDrawDiagram.CommandAddDiagram"
__author__ = "WandererFan"
__url__ = "https://www.freecadweb.org"
__version__ = "00.01"
__date__ = "2022/12/31"

from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui

import TechDrawDiagram

class CommandAddDiagram:
    """Adds a Diagram to the current Diagram."""

    def __init__(self):
        """Initialize variables for the command that must exist at all times."""
        pass

    def GetResources(self):
        """Return a dictionary with data that will be used by the button or menu item."""
        return {'Pixmap': 'actions/TechDraw_DiagramNew.svg',
                'Accel': "",
                'MenuText': QT_TRANSLATE_NOOP("TechDraw_AddDiagram", "Add Diagram"),
                'ToolTip': QT_TRANSLATE_NOOP("TechDraw_AddDiagram", "Add a Diagram to the current Document")}

    def Activated(self):
        """Run the following code when the command is activated (button press)."""
        #print("TechDraw CommandAddDiagram Activated()")
        TechDrawDiagram.TDDiagramWorkers.diagramNew()

    def IsActive(self):
        """Return True when the command should be active or False when it should be disabled (greyed)."""
        if App.ActiveDocument:
            return True
        else:
            return False


#
# The command must be "registered" with a unique name by calling its class.
Gui.addCommand('TechDraw_AddDiagram', CommandAddDiagram())

