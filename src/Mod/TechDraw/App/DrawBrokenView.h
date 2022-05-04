/***************************************************************************
 *   Copyright (c) 2022 WandererFan <wandererfan@gmail.com>                *
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

#ifndef _DRAWBROKENVIEW_H_
#define _DRAWBROKENVIEW_H_

#include <TopoDS_Shape.hxx>

#include "DrawViewPart.h"


namespace TechDraw
{

class TechDrawExport DrawBrokenView : public DrawViewPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::DrawBrokenView);

public:
    DrawBrokenView(void);
    virtual ~DrawBrokenView();

    App::PropertyDistance     Separation;
    App::PropertyVectorList   CutPoints;

    virtual short mustExecute() const override;
    virtual App::DocumentObjectExecReturn *execute(void) override;
    virtual PyObject *getPyObject(void) override;

    virtual TopoDS_Shape getSourceShape(void) const override; 
    TopoDS_Shape breakShape(TopoDS_Shape inputShape,
                            std::vector<Base::Vector3d> cutPoints,
                            double desiredSeparation) const;

protected:
    virtual void onChanged(const App::Property* prop) override;


private:

};

typedef App::FeaturePythonT<DrawBrokenView> DrawBrokenViewPython;

} //namespace TechDraw

#endif  // #ifndef _DRAWBROKENVIEW_H_
