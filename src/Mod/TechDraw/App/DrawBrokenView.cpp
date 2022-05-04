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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <sstream>

#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakeHalfSpace.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>

#endif

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/Vector3D.h>

#include <Mod/TechDraw/App/DrawBrokenViewPy.h>  // generated from DrawBrokenViewPy.xml

#include "Preferences.h"
#include "DrawBrokenView.h"

using namespace TechDraw;
using namespace std;


//===========================================================================
// DrawBrokenView
//===========================================================================


PROPERTY_SOURCE(TechDraw::DrawBrokenView, TechDraw::DrawViewPart)

DrawBrokenView::DrawBrokenView(void)
{
    static const char *group = "Broken View";

    ADD_PROPERTY_TYPE(Separation ,(Preferences::brokenSeparation()),group,App::Prop_None,"Distance between broken parts");
    ADD_PROPERTY_TYPE(CutPoints ,(Base::Vector3d()),group,App::Prop_None,"Location of cutting planes");

}

DrawBrokenView::~DrawBrokenView()
{
}

App::DocumentObjectExecReturn *DrawBrokenView::execute(void)
{
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }
    std::vector<Base::Vector3d> cutPoints = CutPoints.getValues();
    if (cutPoints.size() < 2) {
        return App::DocumentObject::StdReturn;
    } else if (cutPoints[0].IsEqual(cutPoints[1], Precision::Confusion())) {
        return App::DocumentObject::StdReturn;
    }
    return DrawViewPart::execute();
}

TopoDS_Shape DrawBrokenView::getSourceShape() const
{
    TopoDS_Shape inputShape = DrawViewPart::getSourceShape();
    std::vector<Base::Vector3d> cutPoints = CutPoints.getValues();
//    if (cutPoints.size() < 2) {
//        return TopoDS_Shape();
//    } else if (cutPoints[0].IsEqual(cutPoints[1], Precision::Confusion())) {
//        return TopoDS_Shape();
//    }
    double desiredSeparation = Separation.getValue();
    return breakShape(inputShape, cutPoints, desiredSeparation);
}

TopoDS_Shape DrawBrokenView::breakShape(TopoDS_Shape inputShape,
                                 std::vector<Base::Vector3d> cutPoints,
                                 double desiredSeparation) const
{
    BRepTools::Write(inputShape, "DBVShape.brep");            //debug

    double distanceRemoved = (cutPoints[0] - cutPoints[1]).Length();

    gp_Pnt gCutPoint0(cutPoints[0].x, cutPoints[0].y, cutPoints[0].z);
    gp_Pnt gCutPoint1(cutPoints[1].x, cutPoints[1].y, cutPoints[1].z);

    Base::Vector3d vCutDir0 = (cutPoints[1] - cutPoints[0]).Normalize();
    Base::Vector3d vCutDir1 = (cutPoints[0] - cutPoints[1]).Normalize();
    gp_Dir gCutDir0(vCutDir0.x, vCutDir0.y, vCutDir0.z);
    gp_Dir gCutDir1(vCutDir1.x, vCutDir1.y, vCutDir1.z);

    gp_Pln cutPlane0(gCutPoint0, gCutDir0);
    gp_Pln cutPlane1(gCutPoint1, gCutDir1);

    BRepBuilderAPI_MakeFace mkFace0(cutPlane0);
    TopoDS_Face face0 = mkFace0.Face();
    BRepBuilderAPI_MakeFace mkFace1(cutPlane1);
    TopoDS_Face face1 = mkFace1.Face();
    BRepTools::Write(face0, "DBVFace0.brep");            //debug
    BRepTools::Write(face1, "DBVFace1.brep");            //debug


    BRepPrimAPI_MakeHalfSpace mkSolid0(face0, gCutPoint1);
    TopoDS_Solid solid0 = mkSolid0.Solid();
    BRepPrimAPI_MakeHalfSpace mkSolid1(face1, gCutPoint0);
    TopoDS_Solid solid1 = mkSolid1.Solid();

    BRepAlgoAPI_Cut mkCut0(inputShape, solid0);
    BRepAlgoAPI_Cut mkCut1(inputShape, solid1);

    TopoDS_Shape cut0 = mkCut0.Shape();
    TopoDS_Shape cut1 = mkCut1.Shape();
    BRepTools::Write(cut0, "DBVcut0.brep");            //debug
    BRepTools::Write(cut1, "DBVcut1.brep");            //debug

    gp_Vec translateVector0(gCutDir0);
    translateVector0 = translateVector0 * (distanceRemoved - desiredSeparation) / 2.0;
    gp_Vec translateVector1(gCutDir1);
    translateVector1 = translateVector1 * (distanceRemoved - desiredSeparation) / 2.0;

    gp_Trsf xform0;
    gp_Trsf xform1;
    xform0.SetTranslation(translateVector0);
    xform1.SetTranslation(translateVector1);

    BRepBuilderAPI_Transform mkTrf0(cut0, xform0);
    TopoDS_Shape result0 = mkTrf0.Shape();
    BRepBuilderAPI_Transform mkTrf1(cut1, xform1);
    TopoDS_Shape result1 = mkTrf1.Shape();
    BRepTools::Write(result0, "DBVresult0.brep");            //debug
    BRepTools::Write(result1, "DBVresult1.brep");            //debug

    BRep_Builder builder;
    TopoDS_Compound result;
    builder.MakeCompound(result);
    builder.Add(result, result0);
    builder.Add(result, result1);

    return result;
}
short DrawBrokenView::mustExecute() const
{
    short result = 0;
    if (!isRestoring()) {
        result  =  (Separation.isTouched()  ||
                    CutPoints.isTouched());
    }

    if (result) {
        return result;
    }
    return TechDraw::DrawViewPart::mustExecute();
}

void DrawBrokenView::onChanged(const App::Property* prop)
{

    DrawViewPart::onChanged(prop);
}


PyObject *DrawBrokenView::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawBrokenViewPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}


// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawBrokenViewPython, TechDraw::DrawBrokenView)
template<> const char* TechDraw::DrawBrokenViewPython::getViewProviderName(void) const {
    return "TechDrawGui::ViewProviderViewPart";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawBrokenView>;
}
