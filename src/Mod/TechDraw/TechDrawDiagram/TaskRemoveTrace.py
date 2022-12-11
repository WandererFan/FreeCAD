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
"""Provides the TechDraw RemoveTrace Task Dialog."""

__title__ = "TechDrawDiagram.TaskRemoveTrace"
__author__ = "WandererFan"
__url__ = "https://www.freecadweb.org"
__version__ = "00.01"
__date__ = "2022/01/11"

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

class TaskRemoveTrace:
    def __init__(self):
        import os
        self._uiPath = App.getHomePath()
        self._uiPath = os.path.join(self._uiPath, "Mod/TechDraw/TechDrawDiagram/Gui/TaskRemoveTrace.ui")
        self.form = Gui.PySideUic.loadUi(self._uiPath)

        self.form.setWindowTitle(QT_TRANSLATE_NOOP("TechDraw_RemoveTrace", "Remove Trace to Diagram"))

#        self.form.pbView.clicked.connect(self.pickView)
#        self.viewName = ""

        self.dialogOpen = False;


    def accept(self):
#        print ("Accept")
#        fromPage = App.ActiveDocument.getObject(self.fromPageName)
        return True


    def reject(self):
#        print ("Reject")
        return True


    def pickView(self):
#        print("pickView")
#        if (self.dialogOpen) :
#            return

        self.dialogOpen = False


    def setValues(self, viewName, fromPageName, toPageName):
        self.form.leView.setText(viewName)
        self.form.leFromPage.setText(fromPageName)

