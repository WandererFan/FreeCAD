# ***************************************************************************
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
"""Provides view moving functions for TD Tools."""

import FreeCAD as App
import FreeCADGui as Gui
import TechDraw
import os

#move a simple view and its dependents (dimensions, balloons, etc) from p1 to p2
def simpleViewMove(v, p1, p2, copy):
    deps = [x for x in v.InList if x.isDerivedFrom("TechDraw::DrawView")]
    if not copy:
        for d in deps:
            rc = p1.removeView(d)
        rc = p1.removeView(v)
        #redraw p1 without v
        p1.requestPaint()

    rc = p2.addView(v)
    for d in deps:
        rc = p2.addView(d)

    v.recompute()
    App.ActiveDocument.recompute()
    return

#move a section view, its Base View and all its dependents (items, dimensions, balloons, etc) of both from p1 to p2
def sectionViewMove(v, p1, p2, copy):
    #move the base
    base = v.BaseView
    simpleViewMove(base, p1, p2, copy)

#move a projection group and all its dependents (items, dimensions, balloons, etc) from p1 to p2
def projGroupMove(v, p1, p2, copy):
    items = [x for x in v.Views if x.isDerivedFrom("TechDraw::DrawProjGroupItem")]
    if not copy:
        for i in items:
            id = [x for x in i.InList if x.isDerivedFrom("TechDraw::DrawView")]
            for dep in id:
                rc = p1.removeView(dep)
        rc = p1.removeView(v)

    rc = p2.addView(v)
    for i in items:
        id = [x for x in i.InList if x.isDerivedFrom("TechDraw::DrawView")]
        for dep in id:
            if not dep.isDerivedFrom("TechDraw::DrawProjGroup"):
                rc = p2.addView(dep)

def moveView(v, p1, p2, copy=False):
#    print("moveView {0} {1} {2}".format(v, p1, p2))
    if v.isDerivedFrom("TechDraw::DrawProjGroup"):
        projGroupMove(v, p1, p2, copy)
    elif v.isDerivedFrom("TechDraw::DrawViewSection") or v.isDerivedFrom("TechDraw::DrawViewDetail"):
        sectionViewMove(v, p1, p2, copy)
    elif v.isDerivedFrom("TechDraw::DrawView"):
        simpleViewMove(v, p1, p2, copy)

