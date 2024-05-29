/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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
# include <BRep_Builder.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
# include <BRepTools.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Iterator.hxx>
# include <TopoDS_Vertex.hxx>
# include <BRepBuilderAPI_Copy.hxx>
#endif

#include <App/Document.h>
#include <App/GroupExtension.h>
#include <App/FeaturePythonPyImp.h>
#include <App/PropertyPythonObject.h>
#include <App/Link.h>
#include <App/Part.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Placement.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/PrimitiveFeature.h>
#include <Mod/Part/App/FeaturePartCircle.h>
#include <Mod/Part/App/TopoShapePy.h>
#include <Mod/Measure/App/ShapeFinder.h>
//#include <Mod/Sketcher/App/SketchObject.h>


#include "ShapeExtractor.h"
#include "DrawUtil.h"
#include "ShapeUtils.h"


using namespace TechDraw;
using namespace Measure;
using DU = DrawUtil;
using SU = ShapeUtils;


//! pick out the 2d document objects in  inDocumentObjects and return a vector of their shapes
//! Note that point objects will not make it through the hlr/projection process.
std::vector<TopoDS_Shape> ShapeExtractor::getShapes2d(const std::vector<App::DocumentObject*> inDocumentObjects)
{
//    Base::Console().Message("SE::getShapes2d() - inDocumentObjects: %d\n", inDocumentObjects.size());

    std::vector<TopoDS_Shape> shapes2d;

    for (auto& obj : inDocumentObjects) {
        if (is2dObject(obj)) {
            if (obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
                TopoDS_Shape temp = getLocatedShape(obj);
                if (!temp.IsNull()) {
                    shapes2d.push_back(temp);
                }
            }  // other 2d objects would go here - Draft objects? Arch Axis?
        }
    }
    return shapes2d;
}

//! get the located and oriented shapes corresponding to each object in inDocumentObjects as a compound
//! shape.
//! If the shapes are to be fused, include2d should be false as 2d & 3d shapes may not fuse.
TopoDS_Shape ShapeExtractor::getShapes(const std::vector<App::DocumentObject*> inDocumentObjects, bool include2d)
{
    Base::Console().Message("SE::getShapes() - inDocumentObjects in: %d\n", inDocumentObjects.size());
    std::vector<TopoDS_Shape> sourceShapes;

    for (auto& origObj : inDocumentObjects) {
        Base::Console().Message("SE::getShapes - a DocObj: %s/%s\n", origObj->getNameInDocument(), origObj->Label.getValue());

        if (is2dObject(origObj) && !include2d) {
            // Base::Console().Message("SE::getShapes - skipping 2d link: %s\n", origObj->Label.getValue());
            continue;
        }
        auto firstParent = origObj->getFirstParent();
        Base::Console().Message("SE::getShapes - first parent: %s\n", firstParent ? firstParent->Label.getValue() :  "None");
        // afaict getParents returns the ancestry of the subject
        auto manyParents = origObj->getParents();
        for (auto& parent : manyParents) {
            Base::Console().Message("SE::getShapes - a parent: %s / %s\n", parent.first->Label.getValue(), parent.second.c_str());
        }


        // Copy the pointer as not const so it can be changed if needed.
        App::DocumentObject* obj = origObj;
        if (isExplodedAssembly(obj)) {
            obj = getExplodedAssembly(sourceShapes, origObj);
        }

        if (isLinkLike(obj)) {
            // note that some links act as their linked object and type tests based on ancestry will not
            // detect these links?
            Base::Console().Message("SE::getShapes - an App::Link\n");
            std::vector<TopoDS_Shape> xShapes = getShapesFromXRoot(obj);
            Base::Console().Message("SE::getShapes - xShapes: %d\n", xShapes.size());

            if (!xShapes.empty()) {
                sourceShapes.insert(sourceShapes.end(), xShapes.begin(), xShapes.end());
                continue;
            }
        }
        else {
            auto shape = Part::Feature::getShape(obj);
            // if link obj has a shape, we use that shape.
            if(!SU::isShapeReallyNull(shape)) {
                Base::Console().Message("SE::getShapes - a shape\n");
                sourceShapes.push_back(getLocatedShape(obj));
            } else {
                Base::Console().Message("SE::getShapes - dig deeper\n");
                std::vector<TopoDS_Shape> shapeList = getShapesFromObject(obj);
                sourceShapes.insert(sourceShapes.end(), shapeList.begin(), shapeList.end());
            }
        }

        if (isExplodedAssembly(origObj)) {
            restoreExplodedAssembly(origObj);
        }
    }

    BRep_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);
    Base::Console().Message("SE::getShapes - returning %d sourceShapes\n", sourceShapes.size());
    for (auto& s:sourceShapes) {
        if (SU::isShapeReallyNull(s)) {
            continue;
        } else if (s.ShapeType() < TopAbs_SOLID) {
            //clean up composite shapes
            TopoDS_Shape cleanShape = stripInfiniteShapes(s);
            if (!cleanShape.IsNull()) {
                builder.Add(comp, cleanShape);
            }
        } else if (Part::TopoShape(s).isInfinite()) {
            continue;    //simple shape is infinite
        } else {
            //a simple shape - add to compound
            builder.Add(comp, s);
        }
    }
    //it appears that an empty compound is !IsNull(), so we need to check a different way
    if (!SU::isShapeReallyNull(comp)) {
//    BRepTools::Write(comp, "SEResult.brep");            //debug
        return comp;
    }

    return TopoDS_Shape();
}

//! get all the shapes in the sub tree at xLinkRootRoot
std::vector<TopoDS_Shape> ShapeExtractor::getShapesFromXRoot(const App::DocumentObject *xLinkRoot)
{
    Base::Console().Message("SE::getShapesFromXRoot()\n");
    if (!xLinkRoot) {
        Base::Console().Message("SE::getShapesFromXRoot - parameter is null\n");
        return {};
    }

    Base::Console().Message("SE::getShapesFromXRoot() - %s\n", xLinkRoot->getNameInDocument());

    std::vector<TopoDS_Shape> xSourceShapes;
    std::string rootName = xLinkRoot->getNameInDocument();

    // look up
    auto xLinkRootGlobalTransform = getGlobalTransform(xLinkRoot);
    Base::Console().Message("SE::getShapesFromXRoot - global Placement: %s\n",  Measure::ShapeFinder::PlacementAsString(xLinkRootGlobalTransform.first));

    std::vector<App::DocumentObject*> linkedChildren = getLinkedChildren(xLinkRoot);
    auto linkedObject = getLinkedObject(xLinkRoot);
    if (linkedChildren.empty()) {
        Base::Console().Message("SE::getShapesFromXRoot - xLinkRoot has NO children - object is Link: %d\n", isLinkLike(linkedObject));

//        if (linkedObject && isLinkLike(linkedObject)) {
//            // this root points to another link, so we need to follow the linked object??  simple case
//            // seems to work without doing anything special.
//            Base::Console().Message("SE::getShapesFromXRoot - following linked object: %s/%s\n", linkedObject->getNameInDocument(), linkedObject->Label.getValue());
//            auto linkedPaths = nodeVisitor2(linkedObject, 0, 0);
//            for (auto& p : linkedPaths){
//                Base::Console().Message("SE::getShapesFromXRoot - a path: %s\n", p.c_str());
//                // something = doSomething(linkedPaths)
//            }
//        }

        // link points to a regular object, not another link? no sublinks?
        TopoDS_Shape xLinkRootShape = getShapeFromChildlessXLink(xLinkRoot);
        xSourceShapes.push_back(xLinkRootShape);

    } else {
        Base::Console().Message("SE::getShapesFromXRoot - xLinkRoot has children\n");
        // is it worthwhile to special case child that is the end of the chain?
        for (auto& child : linkedChildren) {
            Base::Console().Message("SE::getShapesFromXRoot - a linked child: %s / %s\n",
                                                    child->getNameInDocument(), child->Label.getValue());
            auto childGlobalTransform = getGlobalTransform(child);
            Base::Console().Message("SE::getShapesFromXRoot - child global Placement: %s\n",  Measure::ShapeFinder::PlacementAsString(childGlobalTransform.first));
            // get the unlocated, unscaled shape for child.  Does getShape always get a shape for a Link?
            // can each branch have a different shape?
            // is this shape a compound of the leaves?  wf: this appears to be the case, at least sometimes.

            // this has to be the shape at the end of the branch, if we use an intermediate shape,
            // we will get duplicates?  intermediate shapes don't seem to have everything positioned
            // correctly?
//            auto shape = Part::Feature::getShape(child);    //
//            Part::TopoShape tshape(shape);
//            if (tshape.isInfinite()) {
//                shape = stripInfiniteShapes(shape);
//            }
            auto transforms = nodeVisitor3(child, child, 0, 0);
            Base::Console().Message("SE::getShapesFromXRoot - nodeVisitor branch transforms: %d\n", transforms.size());
            for (auto& tx : transforms) {
                // we should get a leaf shape and a net transform for each branch
                TopoDS_Shape shape = tx.shape();
                Base::Console().Message("SE::getShapesFromXRoot - tx.shape isnull: %d  really: %d\n",
                                        shape.IsNull(), SU::isShapeReallyNull(shape));
                if (SU::isShapeReallyNull(shape))  {
                    continue;
                }
                Part::TopoShape tshape{shape};
                if (tshape.isInfinite()) {
                    shape = stripInfiniteShapes(tx.shape());
                }

                // copying the shape prevents "non-orthogonal GTrsf" errors in some versions
                // of OCC.  Something to do with triangulation of shape??
                // it may be that incremental mesh would work here too.
                BRepBuilderAPI_Copy copier(shape);
                tshape = Part::TopoShape(copier.Shape());
                if(tshape.isNull()) {
                    Base::Console().Message("SE::getShapesFromXRoot - no shape from getXShape\n");
                    continue;
                }
                Base::Placement branchNetPlm = tx.placement();
                Base::Matrix4D branchNetScale = tx.scale();
                Base::Console().Message("SE::getShapesFromXRoot - child Placement: %s\n", Measure::ShapeFinder::PlacementAsString(branchNetPlm));
                Base::Console().Message("SE::getShapesFromXRoot - branchPlacement: %s\n", Measure::ShapeFinder::PlacementAsString(childGlobalTransform.first * branchNetPlm));
                try {
                    tshape.transformGeometry(childGlobalTransform.second * branchNetScale);
                    tshape.setPlacement(childGlobalTransform.first * branchNetPlm);
                }
                catch (...) {
                    Base::Console().Error("ShapeExtractor failed to retrieve shape from %s\n", child->getNameInDocument());
                    continue;
                }
                xSourceShapes.push_back(tshape.getShape());
            }   // transform
        }  // children
    }
    return xSourceShapes;
}

// get the located shape for a single childless App::Link. (A link to an object in another
// document?)

TopoDS_Shape ShapeExtractor::getShapeFromChildlessXLink(const App::DocumentObject* xLink)
{
    Base::Console().Message("SE::getShapeFromChildlessXLink()\n");
    if (!xLink) {
        Base::Console().Message("SE::getShapeFromChildlessXLink - xLink is null\n");
        return {};
    }

    // the linked object might have a placement?
    auto globalTransform = getGlobalTransform(xLink);
    App::DocumentObject* linkedObject = getLinkedObject(xLink);
    if (linkedObject) {
        // have a linked object, get the shape
        TopoDS_Shape shape = Part::Feature::getShape(linkedObject);
        if (shape.IsNull()) {
            // this is where we need to parse the target for objects with a shape??
            return TopoDS_Shape();
        }
        Part::TopoShape tshape(shape);
        if (tshape.isInfinite()) {
            shape = stripInfiniteShapes(shape);
            tshape = Part::TopoShape(shape);
        }
        //tshape might be garbage now, better check
        if (tshape.isNull()) {
            return {};
        }
        // copying the shape prevents "non-orthogonal GTrsf" errors in some versions
        // of OCC.  Something to do with triangulation of shape??
        BRepBuilderAPI_Copy copier(tshape.getShape());
        tshape = Part::TopoShape(copier.Shape());

        auto linkedObjectPlacement = getPlacement(linkedObject);
        auto linkedObjectScale = getScale(linkedObject);  // this is likely a noop
        Base::Console().Message("SE::getShapeFromChildlessXLink - netPlacement: %s\n",
                ShapeFinder::PlacementAsString(globalTransform.first * linkedObjectPlacement));

        try {
            tshape.transformGeometry(globalTransform.second * linkedObjectScale);
            tshape.setPlacement(globalTransform.first * linkedObjectPlacement);
        }
        catch (...) {
            Base::Console().Error("ShapeExtractor failed to retrieve shape from %s\n", xLink->getNameInDocument());
            return TopoDS_Shape();
        }
        return tshape.getShape();
    }
    return TopoDS_Shape();
}

std::vector<TopoDS_Shape> ShapeExtractor::getShapesFromObject(const App::DocumentObject* docObj)
{
    Base::Console().Message("SE::getShapesFromObject(%s)\n", docObj->getNameInDocument());
    std::vector<TopoDS_Shape> result;

    const App::GroupExtension* gex = dynamic_cast<const App::GroupExtension*>(docObj);
    App::Property* gProp = docObj->getPropertyByName("Group");
    App::Property* sProp = docObj->getPropertyByName("Shape");
    if (docObj->isDerivedFrom<Part::Feature>()) {
        result.push_back(getLocatedShape(docObj));
    } else if (gex) {           //is a group extension
        std::vector<App::DocumentObject*> objs = gex->Group.getValues();
        std::vector<TopoDS_Shape> shapes;
        for (auto& d: objs) {
            shapes = getShapesFromObject(d);
            if (!shapes.empty()) {
                result.insert(result.end(), shapes.begin(), shapes.end());
            }
        }
    //the next 2 bits are mostly for Arch module objects
    } else if (gProp) {       //has a Group property
        App::PropertyLinkList* list = dynamic_cast<App::PropertyLinkList*>(gProp);
        if (list) {
            std::vector<App::DocumentObject*> objs = list->getValues();
            std::vector<TopoDS_Shape> shapes;
            for (auto& d: objs) {
                shapes = getShapesFromObject(d);
                if (!shapes.empty()) {
                    result.insert(result.end(), shapes.begin(), shapes.end());
                }
            }
        }
    } else if (sProp) {       //has a Shape property
        Part::PropertyPartShape* shape = dynamic_cast<Part::PropertyPartShape*>(sProp);
        if (shape) {
            result.push_back(getLocatedShape(docObj));
        }
    }
    return result;
}

TopoDS_Shape ShapeExtractor::getShapesFused(const std::vector<App::DocumentObject*> links)
{
//    Base::Console().Message("SE::getShapesFused()\n");
    // get only the 3d shapes and fuse them
    TopoDS_Shape baseShape = getShapes(links, false);
    if (!baseShape.IsNull()) {
        TopoDS_Iterator it(baseShape);
        TopoDS_Shape fusedShape = it.Value();
        it.Next();
        for (; it.More(); it.Next()) {
            const TopoDS_Shape& aChild = it.Value();
            BRepAlgoAPI_Fuse mkFuse(fusedShape, aChild);
            // Let's check if the fusion has been successful
            if (!mkFuse.IsDone()) {
                Base::Console().Error("SE - Fusion failed\n");
                return baseShape;
            }
            fusedShape = mkFuse.Shape();
        }
        baseShape = fusedShape;
    }
    BRepTools::Write(baseShape, "SEbaseShape.brep");

    // if there are 2d shapes in the links they will not fuse with the 3d shapes,
    // so instead we return a compound of the fused 3d shapes and the 2d shapes
    std::vector<TopoDS_Shape> shapes2d = getShapes2d(links);
    BRepTools::Write(DrawUtil::shapeVectorToCompound(shapes2d, false), "SEshapes2d.brep");

    if (!shapes2d.empty()) {
        shapes2d.push_back(baseShape);
        return DrawUtil::shapeVectorToCompound(shapes2d, false);
    }

    return baseShape;
}


//! special retrieval method for handling the steps in an exploded assembly.  The intermediate shapes
//! are created dynamically in the Assembly module, but we only draw still pictures so we need a
//! copy of eache step.
App::DocumentObject* ShapeExtractor::getExplodedAssembly(std::vector<TopoDS_Shape>& sourceShapes, App::DocumentObject* link)
{
    // Copy the pointer as not const so it can be changed if needed.
    // wf: ???
    App::DocumentObject* obj = link;

    auto proxy = dynamic_cast<App::PropertyPythonObject*>(link->getPropertyByName("Proxy"));
    Base::PyGILStateLocker lock;
    if (proxy && proxy->getValue().hasAttr("saveAssemblyAndExplode")) {
        Py::Object explodedViewPy = proxy->getValue();
        Py::Object attr = explodedViewPy.getAttr("saveAssemblyAndExplode");

        if (attr.ptr() && attr.isCallable()) {
            Py::Tuple args(1);
            args.setItem(0, Py::asObject(link->getPyObject()));
            Py::Callable methode(attr);
            Py::Object pyResult = methode.apply(args);

            if (PyObject_TypeCheck(pyResult.ptr(), &(Part::TopoShapePy::Type))) {
                auto* shapepy = static_cast<Part::TopoShapePy*>(pyResult.ptr());
                const TopoDS_Shape& shape = shapepy->getTopoShapePtr()->getShape();
                sourceShapes.push_back(shape);
            }
        }

        for (auto* inObj : link->getInList()) {
            if (inObj->isDerivedFrom(App::Part::getClassTypeId())) {
                // we replace obj by the assembly
                obj = inObj;
                break;
            }
        }
    }
    return obj;
}

//! put the assembly (link) back the way it was
void ShapeExtractor::restoreExplodedAssembly(App::DocumentObject* link)
{
    auto proxy = dynamic_cast<App::PropertyPythonObject*>(link->getPropertyByName("Proxy"));
    Base::PyGILStateLocker lock;
    if (proxy  && proxy->getValue().hasAttr("saveAssemblyAndExplode")) {
        Py::Object explodedViewPy = proxy->getValue();

        Py::Object attr = explodedViewPy.getAttr("restoreAssembly");
        if (attr.ptr() && attr.isCallable()) {
            Py::Tuple args(1);
            args.setItem(0, Py::asObject(link->getPyObject()));
            Py::Callable(attr).apply(args);
        }
    }
}

//inShape is a compound
//The shapes of datum features (Axis, Plan and CS) are infinite
//Infinite shapes can not be projected, so they need to be removed.
TopoDS_Shape ShapeExtractor::stripInfiniteShapes(TopoDS_Shape inShape)
{
//    Base::Console().Message("SE::stripInfiniteShapes()\n");
    BRep_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);

    TopoDS_Iterator it(inShape);
    for (; it.More(); it.Next()) {
        TopoDS_Shape s = it.Value();
        if (s.ShapeType() < TopAbs_SOLID) {
            //look inside composite shapes
            s = stripInfiniteShapes(s);
        } else if (Part::TopoShape(s).isInfinite()) {
            continue;
        } else {
            //simple shape
        }
        builder.Add(comp, s);
    }
    return TopoDS_Shape(std::move(comp));
}

bool ShapeExtractor::is2dObject(const App::DocumentObject* obj)
{
    if (isSketchObject(obj)) {
        return true;
    }

    if (isEdgeType(obj) || isPointType(obj)) {
        return true;
    }
    return false;
}

// just these for now
bool ShapeExtractor::isEdgeType(const App::DocumentObject* obj)
{
    bool result = false;
    Base::Type t = obj->getTypeId();
    if (t.isDerivedFrom(Part::Line::getClassTypeId()) ) {
        result = true;
    } else if (t.isDerivedFrom(Part::Circle::getClassTypeId())) {
        result = true;
    } else if (t.isDerivedFrom(Part::Ellipse::getClassTypeId())) {
        result = true;
    } else if (t.isDerivedFrom(Part::RegularPolygon::getClassTypeId())) {
        result = true;
    }
    return result;
}

bool ShapeExtractor::isPointType(const App::DocumentObject* obj)
{
    // Base::Console().Message("SE::isPointType(%s)\n", obj->getNameInDocument());
    if (obj) {
        Base::Type t = obj->getTypeId();
        if (t.isDerivedFrom(Part::Vertex::getClassTypeId())) {
            return true;
        } else if (isDraftPoint(obj)) {
            return true;
        } else if (isDatumPoint(obj)) {
            return true;
        }
    }
    return false;
}

bool ShapeExtractor::isDraftPoint(const App::DocumentObject* obj)
{
//    Base::Console().Message("SE::isDraftPoint()\n");
    //if the docObj doesn't have a Proxy property, it definitely isn't a Draft point
    App::PropertyPythonObject* proxy = dynamic_cast<App::PropertyPythonObject*>(obj->getPropertyByName("Proxy"));
    if (proxy) {
        std::string  pp = proxy->toString();
//        Base::Console().Message("SE::isDraftPoint - pp: %s\n", pp.c_str());
        if (pp.find("Point") != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool ShapeExtractor::isDatumPoint(const App::DocumentObject* obj)
{
    std::string objTypeName = obj->getTypeId().getName();
    std::string pointToken("Point");
    if (objTypeName.find(pointToken) != std::string::npos) {
        return true;
    }
    return false;
}

//! true if the link is an exploded assembly
bool ShapeExtractor::isExplodedAssembly(const App::DocumentObject* link)
{
    auto proxy = dynamic_cast<App::PropertyPythonObject*>(link->getPropertyByName("Proxy"));
    Base::PyGILStateLocker lock;
    if (!proxy) {
        return false;
    }

    auto proxyValue = proxy->getValue();
    if (proxyValue == Py::None()) {
        return false;
    }

    if (proxy->getValue().hasAttr("saveAssemblyAndExplode")) {
        return true;
    }
    return false;
}


//! get the location of a point object
Base::Vector3d ShapeExtractor::getLocation3dFromFeat(const App::DocumentObject* obj)
{
    // Base::Console().Message("SE::getLocation3dFromFeat()\n");
    if (!isPointType(obj)) {
        return Base::Vector3d(0.0, 0.0, 0.0);
    }
//    if (isDraftPoint(obj) {
//        //Draft Points are not necc. Part::PartFeature??
//        //if Draft option "use part primitives" is not set are Draft points still PartFeature?

    const Part::Feature* pf = dynamic_cast<const Part::Feature*>(obj);
    if (pf) {
        Part::TopoShape pts = pf->Shape.getShape();
        pts.setPlacement(pf->globalPlacement());
        TopoDS_Shape ts = pts.getShape();
        if (ts.ShapeType() == TopAbs_VERTEX)  {
            TopoDS_Vertex v = TopoDS::Vertex(ts);
            return DrawUtil::vertex2Vector(v);
        }
    }

    return Base::Vector3d(0.0, 0.0, 0.0);
}


//! get the located and oriented version of docObj's shape.  globalPlacement only works for
//! objects derived from App::GeoFeature.
TopoDS_Shape ShapeExtractor::getLocatedShape(const App::DocumentObject* docObj)
{
        Part::TopoShape shape = Part::Feature::getShape(docObj);
        auto  geoFeat = dynamic_cast<const App::GeoFeature*>(docObj);
        if (geoFeat) {
            shape.setPlacement(geoFeat->globalPlacement());
        }
        return shape.getShape();
}

bool ShapeExtractor::isSketchObject(const App::DocumentObject* obj)
{
// TODO:: the check for an object being a sketch should be done as in the commented
// if statement below. To do this, we need to include Mod/Sketcher/SketchObject.h,
// but that makes TechDraw dependent on Eigen libraries which we don't use.  As a
// workaround we will inspect the object's class name.
//    if (obj->isDerivedFrom(Sketcher::SketchObject::getClassTypeId())) {
    std::string objTypeName = obj->getTypeId().getName();
    std::string sketcherToken("Sketcher");
    if (objTypeName.find(sketcherToken) != std::string::npos) {
        return true;
    }
    return false;
}

// visits each node below "node" in a sub tree of link dependencies.
void ShapeExtractor::nodeVisitor(const App::DocumentObject* node, int level, int sibling)
{
    Base::Console().Message("SE::nodeVisitor(%s/%s, %d, %d)\n", node->getNameInDocument(), node->Label.getValue(), level, sibling);
    std::vector<App::DocumentObject*> linkedChildren;
    auto namedProperty = node->getPropertyByName("ElementList");
    auto elementListProperty = dynamic_cast<App::PropertyLinkList*>(namedProperty);
    if (namedProperty && elementListProperty) {
        linkedChildren = elementListProperty->getValues();
    }
    if (linkedChildren.empty()) {
        Base::Console().Message("SE::nodeVisitor - end of branch\n");
        return;
    }

    int iSibling{0};
    int newLevel = level + 1;
    for (auto& child : linkedChildren) {
        nodeVisitor(child, newLevel, iSibling);
        iSibling++;
    }
}

// visits each node below "node" in a sub tree of link dependencies.
std::vector<std::string> ShapeExtractor::nodeVisitor2(const App::DocumentObject* node, int level, int sibling)
{
    if (!node) {
        return {};
    }

    Base::Console().Message("SE::nodeVisitor2(%s/%s, %d, %d)\n", node->getNameInDocument(), node->Label.getValue(), level, sibling);
    std::vector<std::string> result;
    std::string nodeName = node->getNameInDocument();

    std::vector<App::DocumentObject*> linkedChildren = getLinkedChildren(node);
    if (linkedChildren.empty()) {
        Base::Console().Message("SE::nodeVisitor2 - end of branch\n");
        result.emplace_back(nodeName + ".");
        return result;
    }

    int iSibling{0};
    int newLevel = level + 1;
    for (auto& child : linkedChildren) {
        auto childPaths = nodeVisitor2(child, newLevel, iSibling);
        for (auto& path : childPaths) {
            result.emplace_back(nodeName + "." + path);
        }
        iSibling++;
    }
    return result;
}

//! getters for link like objects


std::vector<App::DocumentObject*> ShapeExtractor::getLinkedChildren(const App::DocumentObject* root)
{
    if (!isLinkLike(root)) {
        return {};
    }

    auto namedProperty = root->getPropertyByName("ElementList");
    auto elementListProperty = dynamic_cast<App::PropertyLinkList*>(namedProperty);
    if (namedProperty && elementListProperty) {
        return elementListProperty->getValues();
    }
    return {};
}


//! this getter should work for any object, not just links
Base::Placement ShapeExtractor::getPlacement(const App::DocumentObject* root)
{
    auto namedProperty = root->getPropertyByName("Placement");
    auto placementProperty = dynamic_cast<App::PropertyPlacement*>(namedProperty);
    if (namedProperty && placementProperty) {
        return placementProperty->getValue();
    }
    return {};
}

//! get root's scale property.  If root is not a Link related object, then the identity matrrix will
//! be returned.
Base::Matrix4D ShapeExtractor::getScale(const App::DocumentObject* root)
{
    if (!isLinkLike(root)) {
        return {};
    }

    Base::Matrix4D linkScale;
    auto namedProperty = root->getPropertyByName("ScaleVector");
    auto scaleVectorProperty = dynamic_cast<App::PropertyVector*>(namedProperty);
    if (scaleVectorProperty) {
        linkScale.scale(scaleVectorProperty->getValue());
    }
    return linkScale;
}


//! get root's LinkedObject.  If root is not a Link related object, then nullptr will be returned.
App::DocumentObject* ShapeExtractor::getLinkedObject(const App::DocumentObject* root)
{
    if (!isLinkLike(root)) {
        return {};
    }

    App::DocumentObject* linkedObject{nullptr};
    auto namedProperty = root->getPropertyByName("LinkedObject");
    auto linkedObjectProperty = dynamic_cast<App::PropertyLink*>(namedProperty);
    if (linkedObjectProperty) {
        linkedObject = linkedObjectProperty->getValue();
    }
    return linkedObject;
}


//! there isn't convenient common ancestor for the members of the Link family.  We use isLinkLike(obj)
//! instead of obj->isDerivedFrom<ConvenientCommonAncestor>().  Some links have proxy objects and will
//! not be detected by isDerivedFrom().
bool ShapeExtractor::isLinkLike(const App::DocumentObject* obj)
{
    if (!obj) {
        return false;
    }

    if (obj->isDerivedFrom<App::Link>() ||
        obj->isDerivedFrom<App::LinkElement>() ||
        obj->isDerivedFrom<App::LinkGroup>()) {
        return true;
    }

    auto namedProperty = obj->getPropertyByName("LinkedObject");
    auto linkedObjectProperty = dynamic_cast<App::PropertyLink*>(namedProperty);
    if (linkedObjectProperty) {
        return true;
    }

    namedProperty = obj->getPropertyByName("ElementList");
    auto elementListProperty = dynamic_cast<App::PropertyLinkList*>(namedProperty);
    if (elementListProperty) {
        return true;
    }

    return false;
}

//! get the net placement and net scale that should be applied to cursorObject due to all the objects above it
//! in the tree.
std::pair<Base::Placement, Base::Matrix4D> ShapeExtractor::getGlobalTransform(const App::DocumentObject* cursorObject)
{
    Base::Console().Message("SE::getGlobalTransform()\n");
    if (!cursorObject) {
        return {};
    }
    // plain ordinary geometry objects (App::Part, PartWB, PartDesignWB, Sketcher, ...)
    if (cursorObject->isDerivedFrom<App::GeoFeature>()) {
        auto geoCursor = dynamic_cast<const App::GeoFeature*>(cursorObject);
        return {geoCursor->globalPlacement(), {}};
    }

    // group of plain ordinary geometry
    if (cursorObject->hasExtension(App::GeoFeatureGroupExtension::getExtensionClassTypeId())) {
        auto geoCursorGroupConst = dynamic_cast<const App::GeoFeatureGroupExtension*>(cursorObject);
        App::GeoFeatureGroupExtension* geoCursorGroup = const_cast<App::GeoFeatureGroupExtension*>(geoCursorGroupConst);
        return {geoCursorGroup->globalGroupPlacement(), {} };
    }

    // link related objects
    auto cursorParents = cursorObject->getParents();
    // an object is always its own parent in this situation(?) so cursorParents will never be empty?
    if (cursorParents.size() == 1  &&
        cursorParents.front().first == cursorObject) {
        // cursorObject is a top level object and there are no placements above it
        return { getPlacement(cursorObject), getScale(cursorObject)};
    }

    for (auto& parent : cursorParents) {
        Base::Console().Message("SE::getGlobalTransform - a cursorParent: %s / %s\n", parent.first->Label.getValue(), parent.second.c_str());
        if (parent.first->getDocument() != cursorObject->getDocument()) {
            // not in our document, so can't be our parent?  Links can cross documents no?
            continue;
        }

        auto resolve = ShapeFinder::resolveSelection(*(parent.first), parent.second);
        if (&(resolve.getTarget()) == cursorObject) {
            // this is the parent path we want to gather transforms from
            auto pathObjects = parent.first->getSubObjectList(parent.second.c_str());
            Base::Placement netPlacement;
            Base::Matrix4D netScale;
            Base::Console().Message("SE::getGlobalTransform - combining transforms for: %s/%s\n", cursorObject->getNameInDocument(), cursorObject->Label.getValue());
            combineTransforms(pathObjects, netPlacement, netScale);
            return { netPlacement, netScale };
        }
   }

    return {};
}


//! gather the placements and scale transforms of the item in pathObjects and sum them top-down.
void ShapeExtractor::combineTransforms(const std::vector<App::DocumentObject*>& pathObjects, Base::Placement& netPlacement, Base::Matrix4D& netScale)
{
    // apply the placements from the top down to us
    for (auto& obj : pathObjects) {
        if (!obj) {
            continue;
        }
        Base::Console().Message("SE::combineTransforms - a path object: %s/%s\n", obj->getNameInDocument(), obj->Label.getValue());
        netPlacement *= getPlacement(obj);
        netScale *= getScale(obj);
   }
}


//! visits each node below "node" in a sub tree of link dependencies.  Returns a transform for each
//! branch below "node".
TransformVector ShapeExtractor::nodeVisitor3(const App::DocumentObject* pathRoot, const App::DocumentObject *currentNode, int level, int sibling)
{
    if (!pathRoot || !pathRoot->isAttachedToDocument() ||
        !currentNode || !currentNode->isAttachedToDocument()) {
        Base::Console().Message("SE::nodeVisitor3 - bailing early\n");
        return {};
    }

    Base::Console().Message("SE::nodeVisitor3(%s/%s, %d, %d)\n", currentNode->getNameInDocument(), currentNode->Label.getValue(), level, sibling);
    TransformVector result;

    std::vector<App::DocumentObject*> linkedChildren = getLinkedChildren(currentNode);
    if (linkedChildren.empty()) {
        auto shape = getShapeFromChildlessXLink(currentNode);
        Base::Console().Message("SE::nodeVisitor3 - %s/%s isNull: %d\n", currentNode->getNameInDocument(), currentNode->Label.getValue(), shape.IsNull());
        if (SU::isShapeReallyNull(shape)) {
            // this is not an expected condition.
            throw Base::RuntimeError("Unexpected null shape in nodeVisitor3");
        }
        // if this the first and last node in the branch, we should not add currentNode's transform as it
        // was accounted for in the calculation of global placement for currentNode.
        // if (node == root) then we are the first and if no children then we are alsothe last.
        Base::Console().Message("SE::nodeVisitor3 - end of branch\n");
        if (pathRoot == currentNode)  {
            // TODO: is pointer comparison right here? better to compare names?
            // pathRoot is the start and end of this branch.  We do not want to add pathRoot's transform
            // as it has been accounted for in getGlobalPlacement.  We return identity transform here.
            TransformItem newTransform{ shape, {}, {} };
            result.emplace_back(newTransform);
            return result;
        }

        TransformItem newTransform{shape, getPlacement(currentNode), getScale(currentNode)};
        result.emplace_back(newTransform);
        return result;
    }

    int iSibling{0};
    int newLevel = level + 1;
    for (auto& child : linkedChildren) {
        auto childPaths = nodeVisitor3(currentNode, child, newLevel, iSibling);
        for (auto& path : childPaths) {
            // we keep the shape found by lower levels
            TransformItem newTransform{path.shape(), path.placement(), path.scale()};
            result.emplace_back(newTransform);
        }
        iSibling++;
    }
    return result;
}
