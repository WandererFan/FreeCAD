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
"""Provides resolution conversion routines for TechDraw.Diagram"""


import FreeCAD as App

# TODO: look up rez factor in preferences
# double Rez::m_rezFactor = Rez::getParameter();
# double Rez::getParameter()
# {
#     Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
#        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Rez");
#     double rezFactor  = hGrp->GetFloat("Resolution", 10.0);
#     return rezFactor;
# }
def getParameter():
    params = App.ParamGet("User parameter:BaseApp/Preferences/Mod/TechDraw/Rez")
    return params.GetFloat("Resolution", 10.0)


def guiX(x):
    return x * getParameter()


def appX(x):
    return x / getParameter()
