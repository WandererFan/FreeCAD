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
"""Provides the TechDraw CopyView GuiCommand."""

__title__ = "TechDrawTools.CommandCopyView"
__author__ = "WandererFan"
__url__ = "http://www.freecadweb.org/index.html"
__version__ = "00.01"
__date__ = "2022/01/11"

from PySide.QtCore import QT_TRANSLATE_NOOP
from PySide import QtCore
import PySide.QtGui as QtGui

import FreeCAD as App
import FreeCADGui as Gui
import TechDraw
from TechDrawTools import TDToolsUtil
from TechDrawTools import TDToolsMovers
from TechDrawTools import TaskCopyView
import os

class CommandCopyView:
    """Copies a View from current Page to a different Page."""

    def __init__(self):
        """Initialize variables for the command that must exist at all times."""
        pass

    def GetResources(self):
        """Return a dictionary with data that will be used by the button or menu item."""
        return {'Pixmap': 'actions/TechDraw_CopyView.svg',
                'Accel': "",
                'MenuText': QT_TRANSLATE_NOOP("CopyView", "Copy View"),
                'ToolTip': QT_TRANSLATE_NOOP("CopyView", "Copy a View to a second Page")}

    def Activated(self):
        """Run the following code when the command is activated (button press)."""
#        print("Activated()")
        sel = Gui.Selection.getSelection()

        vName = ""
        views = list()
        for o in sel:
            if o.isDerivedFrom("TechDraw::DrawView"):
                views.append(o)
        if views:
            v = views[0]
            vName = v.Name

        tpName = ""
        fpName = ""
        pages = list()
        for o in sel:
            if o.isDerivedFrom("TechDraw::DrawPage"):
                pages.append(o)
        if pages:
            fromPage = pages[0]
            fpName = fromPage.Name
        if len(pages) > 1:
            toPage = pages[1]
            tpName = toPage.Name

        self.ui  = TaskCopyView.TaskCopyView()

        self.ui.setValues(vName, fpName, tpName)
        Gui.Control.showDialog(self.ui)

    def IsActive(self):
        """Return True when the command should be active or False when it should be disabled (greyed)."""
        if App.ActiveDocument:
            return TDToolsUtil.havePage() and TDToolsUtil.haveView()
        else:
            return False


#
# The command must be "registered" with a unique name by calling its class.
Gui.addCommand('TechDraw_CopyView', CommandCopyView())

