/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2022 WandererFan   <wandererfan@gmail.com>              *
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

#ifndef DRAWINGGUI_VIEWPROVIDERDIAGRAM_H
#define DRAWINGGUI_VIEWPROVIDERDIAGRAM_H

#include <Mod/TechDraw/TechDrawGlobal.h>
#include <Mod/TechDraw/App/Diagram/Diagram.h>
#include "ViewProviderPage.h"

namespace TechDraw{
    class DrawPage;
    class Diagram;
}

namespace TechDrawGui {

class TechDrawGuiExport ViewProviderDiagram : public ViewProviderPage
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDrawGui::ViewProviderDiagram);

public:
    /// constructor
    ViewProviderDiagram();
    /// destructor
    ~ViewProviderDiagram() override = default;

    void attach(App::DocumentObject *) override;
    bool showMDIViewPage() override;
    TechDraw::Diagram* getDiagram() const;

    const char* whoAmI() const override;

private:
    std::string m_diagramName;
};

} // namespace TechDrawGui


#endif // DRAWINGGUI_VIEWPROVIDERDIAGRAM_H
