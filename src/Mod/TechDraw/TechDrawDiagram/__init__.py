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

__title__ = "TechDrawDiagram package"
__author__  = "WandererFan"
__url__     = "https://www.freecadweb.org"
__version__ = "00.01"
__date__    = "2022-12-31"

## @package TechDrawDiagram
#  \ingroup TechDraw
#  \brief TechDrawDiagram Package for TechDraw workbench

# Command => Icons & Menus
# Task => Dialogs
# Workers => application code
# Util => this and that
# Painters => draw on the screen

from .Rez import *
from .TDDiagramWorkers import *
from .TDDiagramUtil import *
from .TDDiagramTypeManager import *


from TechDrawDiagram.TDDiagramPainters import *

from .CommandAddDiagram import CommandAddDiagram

from .CommandAddSymbol import CommandAddSymbol
from .CommandRemoveSymbol import CommandRemoveSymbol
from .TaskAddSymbol import TaskAddSymbol
from .TaskRemoveSymbol import TaskRemoveSymbol

from .CommandAddTrace import CommandAddTrace
from .CommandRemoveTrace import CommandRemoveTrace
from .CommandEditTrace import CommandEditTrace
from .TaskAddTrace import TaskAddTrace
from .TaskRemoveTrace import TaskRemoveTrace

