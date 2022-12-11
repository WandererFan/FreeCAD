/***************************************************************************
 *   Copyright (c) 2022  WandererFan  <wandererfan@gmail.com>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_

#endif

#include <Base/Console.h>
#include <Gui/Command.h>

#include <Mod/TechDraw/App/Diagram/Diagram.h>
#include <Mod/TechDraw/App/DiagramPy.h>
#include <Mod/TechDraw/App/DrawPage.h>

#include "QGSPage.h"
#include "ViewProviderDiagram.h"

using namespace TechDrawGui;
using namespace TechDraw;


PROPERTY_SOURCE(TechDrawGui::ViewProviderDiagram, TechDrawGui::ViewProviderPage)


//**************************************************************************
// Construction/Destruction

ViewProviderDiagram::ViewProviderDiagram()
  : m_diagramName("")
{
    sPixmap = "actions/TechDraw_Diagram";
}

void ViewProviderDiagram::attach(App::DocumentObject *pcFeat)
{
    ViewProviderPage::attach(pcFeat);

    TechDraw::Diagram* feature = dynamic_cast<TechDraw::Diagram*>(pcFeat);
    if (feature) {
        m_diagramName = feature->getNameInDocument();
        getQGSPage()->setObjectName(QString::fromLocal8Bit(m_diagramName.c_str()));
    }
}

bool ViewProviderDiagram::showMDIViewPage()
{
//    Base::Console().Message("VPD::showMDIViewPage()\n");
    ViewProviderPage::showMDIViewPage();
    Diagram* diagram = getDiagram();
    std::string diagramName = diagram->getNameInDocument();
    Gui::Command::doCommand(Gui::Command::Doc, "from TechDrawDiagram import TDDiagramWorkers");
    Gui::Command::doCommand(Gui::Command::Doc, "TDDiagramWorkers.repaintDiagram(App.activeDocument().%s)", diagramName.c_str());

    return true;
}

TechDraw::Diagram* ViewProviderDiagram::getDiagram() const
{
    if (!pcObject) {
        Base::Console().Log("VPD::getDiagram - no Diagram Object!\n");
        return nullptr;
    }
    return dynamic_cast<TechDraw::Diagram*>(pcObject);
}
const char*  ViewProviderDiagram::whoAmI() const
{
    return m_diagramName.c_str();
}

