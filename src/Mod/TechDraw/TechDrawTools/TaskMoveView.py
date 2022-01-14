# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022 - Wanderer Fan <wandererfan@gmail.com>             *
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
"""Provides the TechDraw MoveView Task Dialog."""

__title__ = "TechDrawTools.TaskMoveView"
__author__ = "WandeererFan"
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
import os

class TaskMoveView:
    def __init__(self):
        import os
        self._uiPath = App.getHomePath()
        self._uiPath = os.path.join(self._uiPath, "Mod/TechDraw/Gui/TaskMoveView.ui")
        self.form = Gui.PySideUic.loadUi(self._uiPath)

        self.form.pbView.clicked.connect(self.pickView)
        self.form.pbFromPage.clicked.connect(self.pickFromPage)
        self.form.pbToPage.clicked.connect(self.pickToPage)

        self.viewName = ""
        self.fromPageName = ""
        self.toPageName   = ""

    def accept(self):
#        print ("Accept")
        v = App.ActiveDocument.getObject(self.viewName)
        f = App.ActiveDocument.getObject(self.fromPageName)
        t = App.ActiveDocument.getObject(self.toPageName)
        TDToolsMovers.moveView(v, f, t)
        return True

    def reject(self):
#        print ("Reject")
        return True

    def pickView(self):
#        print("pickView")
        _dlgPath = App.getHomePath()
        _dlgPath = os.path.join(_dlgPath, "Mod/TechDraw/Gui/DlgPageChooser.ui")
        dlg = Gui.PySideUic.loadUi(_dlgPath)
        dlg.lPrompt.setText(QT_TRANSLATE_NOOP("MoveView", "Select View to move from list."))
        dlg.setWindowTitle(QT_TRANSLATE_NOOP("MoveView", "Select View"))

        views = [x for x in App.ActiveDocument.Objects if x.isDerivedFrom("TechDraw::DrawView")]
        for v in views:
            n = v.Name
            l = v.Label
            s = l + " / " + n
            item = QtGui.QListWidgetItem(s, dlg.lwPages)
            item.setData(QtCore.Qt.UserRole, n)
        if (dlg.exec() == QtGui.QDialog.Accepted) :
            if dlg.lwPages.selectedItems():
                selItem = dlg.lwPages.selectedItems()[0]
                self.viewName = selItem.data(QtCore.Qt.UserRole)
                self.form.leView.setText(self.viewName)

    def pickFromPage(self):
#        print("pickFromPage")
        _dlgPath = App.getHomePath()
        _dlgPath = os.path.join(_dlgPath, "Mod/TechDraw/Gui/DlgPageChooser.ui")
        dlg = Gui.PySideUic.loadUi(_dlgPath)
        dlg.lPrompt.setText(QT_TRANSLATE_NOOP("MoveView", "Select From Page."))
        dlg.setWindowTitle(QT_TRANSLATE_NOOP("MoveView", "Select Page"))

        pages = [x for x in App.ActiveDocument.Objects if x.isDerivedFrom("TechDraw::DrawPage")]
        for p in pages:
            n = p.Name
            l = p.Label
            s = l + " / " + n
            item = QtGui.QListWidgetItem(s, dlg.lwPages)
            item.setData(QtCore.Qt.UserRole, n)
        if (dlg.exec() == QtGui.QDialog.Accepted) :
            if dlg.lwPages.selectedItems():
                selItem = dlg.lwPages.selectedItems()[0]
                self.fromPageName = selItem.data(QtCore.Qt.UserRole)
                self.form.leFromPage.setText(self.fromPageName)

    def pickToPage(self):
#        print("pickToPage")
        _dlgPath = App.getHomePath()
        _dlgPath = os.path.join(_dlgPath, "Mod/TechDraw/Gui/DlgPageChooser.ui")
        dlg = Gui.PySideUic.loadUi(_dlgPath)
        dlg.lPrompt.setText(QT_TRANSLATE_NOOP("MoveView", "Select To Page."))
        dlg.setWindowTitle(QT_TRANSLATE_NOOP("MoveView", "Select Page"))

        pages = [x for x in App.ActiveDocument.Objects if x.isDerivedFrom("TechDraw::DrawPage")]
        for p in pages:
            n = p.Name
            l = p.Label
            s = l + " / " + n
            item = QtGui.QListWidgetItem(s, dlg.lwPages)
            item.setData(QtCore.Qt.UserRole, n)
        if (dlg.exec() == QtGui.QDialog.Accepted) :
            if dlg.lwPages.selectedItems():
                selItem = dlg.lwPages.selectedItems()[0]
                self.toPageName = selItem.data(QtCore.Qt.UserRole)
                self.form.leToPage.setText(self.toPageName)

    def setValues(self, v, f, t):
#        print("setValues {0} {1} {2}".format(v, f, t))
        self.viewName = v
        self.form.leView.setText(v)
        self.fromPageName = f
        self.form.leFromPage.setText(f)
        self.toPageName = t
        self.form.leToPage.setText(t)

