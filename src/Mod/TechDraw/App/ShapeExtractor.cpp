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
#include <Mod/Part/App/Tools.h>
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

        // Copy the pointer as not const so it can be changed if needed.
        App::DocumentObject* obj = origObj;
        if (isExplodedAssembly(obj)) {
            obj = getExplodedAssembly(sourceShapes, origObj);
        }

        if (ShapeFinder::isLinkLike(obj)) {
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
        }

        if (s.ShapeType() < TopAbs_SOLID) {
            //clean up composite shapes
            TopoDS_Shape cleanShape = ShapeFinder::stripInfiniteShapes(s);
            if (!cleanShape.IsNull()) {
                builder.Add(comp, cleanShape);
            }
        } else {
            //a simple shape - add to compound
            if (Part::TopoShape(s).isInfinite()) {
                continue;    //simple shape is infinite
            }

            builder.Add(comp, s);
        }
    }
    //it appears that an empty compound is !IsNull(), so we need to check a different way
    if (!SU::isShapeReallyNull(comp)) {
        return comp;
    }

    return {};
}

//! get all the shapes in the sub tree starting at xLinkRoot.
std::vector<TopoDS_Shape> ShapeExtractor::getShapesFromXRoot(const App::DocumentObject *xLinkRoot)
{
    if (!xLinkRoot) {
        return {};
    }

    std::vector<TopoDS_Shape> xSourceShapes;
    std::string rootName = xLinkRoot->getNameInDocument();

    TopoDS_Shape xLinkRootShape = Part::Feature::getShape(xLinkRoot);
    if (SU::isShapeReallyNull(xLinkRootShape)) {
        return {};
    }

    // in the case of an attachment created by links (using properties such as b3AttChildSubobjPlacement
    // and c3AttChildResultPlc), Part::Feature::getShape returns the shape with transforms already
    // applied.
    auto attachedParent = ShapeFinder::getLinkAttachParent(xLinkRoot);      // there is probably a better test
    if (attachedParent) {
        xSourceShapes.push_back(xLinkRootShape);
        return xSourceShapes;
    }

    // cases other than link attachments

    // case: link -> link -> shape
    auto linkedObject = xLinkRoot->getLinkedObject();
    if (ShapeFinder::isLinkLike(linkedObject)) {
        Base::Console().Message("SE::getShapesFromXRoot - linked object is link\n");
            // static TopoDS_Shape getShape(const App::DocumentObject *obj,
            // const char *subname=nullptr, bool needSubElement=false, Base::Matrix4D *pmat=nullptr,
            // App::DocumentObject **owner=nullptr, bool resolveLink=true, bool transform=true);
        // auto linkShape = Part::Feature::getShape(linkedObject, nullptr, false, nullptr, nullptr, false, true);
        auto linkShape = Part::Feature::getShape(linkedObject);
        xSourceShapes.push_back(linkShape);
        return xSourceShapes;

    }

    // the common case
    auto transform = ShapeFinder::getGlobalTransform(xLinkRoot);

    xLinkRootShape = ShapeFinder::transformShape(xLinkRootShape, transform.first, transform.second);

    xSourceShapes.push_back(xLinkRootShape);
    return xSourceShapes;
}


std::vector<TopoDS_Shape> ShapeExtractor::getShapesFromObject(const App::DocumentObject* docObj)
{
    Base::Console().Message("SE::getShapesFromObject(%s)\n", docObj->getNameInDocument());
    std::vector<TopoDS_Shape> result;

    auto gex = dynamic_cast<const App::GroupExtension*>(docObj);
    App::Property* gProp = docObj->getPropertyByName("Group");
    App::Property* sProp = docObj->getPropertyByName("Shape");
    if (docObj->isDerivedFrom<Part::Feature>()) {
        result.push_back(getLocatedShape(docObj));
    } else if (gex) {           //is a group extension
        std::vector<App::DocumentObject*> groupObjects = gex->Group.getValues();
        std::vector<TopoDS_Shape> shapes;
        for (auto& obj: groupObjects) {
            shapes = getShapesFromObject(obj );
            if (!shapes.empty()) {
                result.insert(result.end(), shapes.begin(), shapes.end());
            }
        }
    //the next 2 bits are mostly for Arch module objects
    } else if (gProp) {       //has a Group property
        auto list = dynamic_cast<App::PropertyLinkList*>(gProp);
        if (list) {
            std::vector<App::DocumentObject*> groupObjects = list->getValues();
            std::vector<TopoDS_Shape> shapes;
            for (auto& obj: groupObjects) {
                shapes = getShapesFromObject(obj);
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

    // if there are 2d shapes in the links they will not fuse with the 3d shapes,
    // so instead we return a compound of the fused 3d shapes and the 2d shapes
    std::vector<TopoDS_Shape> shapes2d = getShapes2d(links);
//    BRepTools::Write(DrawUtil::shapeVectorToCompound(shapes2d, false), "SEshapes2d.brep");

    if (!shapes2d.empty()) {
        shapes2d.push_back(baseShape);
        return DrawUtil::shapeVectorToCompound(shapes2d, false);
    }

    return baseShape;
}


//! special retrieval method for handling the steps in an exploded assembly.  The intermediate shapes
//! are created dynamically in the Assembly module, but we only draw still pictures so we need a
//! copy of each step.
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
// TODO: migrate to ShapeFinder::getLocatedShape?
TopoDS_Shape ShapeExtractor::getLocatedShape(const App::DocumentObject* docObj)
{
        Part::TopoShape shape = Part::Feature::getShape(docObj);
        auto  geoFeat = dynamic_cast<const App::GeoFeature*>(docObj);
        if (geoFeat) {
            Base::Console().Message("SE::getLocatedShape - plm: %s\n",
                                    ShapeFinder::PlacementAsString(geoFeat->globalPlacement()));
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


//! visits each node below "node" in a branch of link dependencies. returns a dot separated string
//! that describes the branch. The strings are not quite what selection provides yet - obj1.obj2.obj3.
//! without numbers for link elements.
// not currently in use
std::vector<std::string> ShapeExtractor::nodeVisitor2(const App::DocumentObject* node, int level, int sibling)
{
    if (!node || !node->isAttachedToDocument()) {
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
    if (!ShapeFinder::isLinkLike(root)) {
        return {};
    }

    auto namedProperty = root->getPropertyByName("ElementList");
    auto elementListProperty = dynamic_cast<App::PropertyLinkList*>(namedProperty);
    if (namedProperty && elementListProperty) {
        return elementListProperty->getValues();
    }
    return {};
}


//! get root's LinkedObject.  If root is not a Link related object, then nullptr will be returned.
App::DocumentObject* ShapeExtractor::getLinkedObject(const App::DocumentObject* root)
{
    if (!ShapeFinder::isLinkLike(root)) {
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




//! visits each node below "node" in a sub tree of link dependencies.  Returns a transform for each
//! branch below "node". currentNodeIn can be used to change the start point of the traversal, but
//! is normally defaulted.
//TransformVector ShapeExtractor::nodeVisitor3(const App::DocumentObject* pathRoot,
//                                             const App::DocumentObject* currentNodeIn,
//                                             int level, int sibling)
//{
//    App::DocumentObject* currentNode = const_cast<App::DocumentObject*>(currentNodeIn);
//    if (!currentNodeIn) {
//        currentNode = const_cast<App::DocumentObject*>(pathRoot);
//    }
//    if (!pathRoot || !pathRoot->isAttachedToDocument() ||
//        !currentNode->isAttachedToDocument()) {
//        return {};
//    }

//    Base::Console().Message("SE::nodeVisitor3(%s/%s, %d, %d)\n", currentNode->getNameInDocument(), currentNode->Label.getValue(), level, sibling);
//    TransformVector result;

//    std::vector<App::DocumentObject*> linkedChildren = getLinkedChildren(currentNode);
//    if (linkedChildren.empty()) {
//        auto shape = getShapeFromChildlessXLink(currentNode);
//        Base::Console().Message("SE::nodeVisitor3 - %s/%s isNull: %d\n", currentNode->getNameInDocument(), currentNode->Label.getValue(), shape.IsNull());
//        if (SU::isShapeReallyNull(shape)) {
//            throw Base::RuntimeError("Unexpected null shape in nodeVisitor3");
//        }
//        // if this the first and last node in the branch, we should not add currentNode's transform as it
//        // was accounted for in the calculation of global placement for currentNode.
//        // if (node == root) then we are the first and if no children then we are also the last.
//        Base::Console().Message("SE::nodeVisitor3 - end of branch\n");
//        if (pathRoot == currentNode)  {
//            // pathRoot is the start and end of this branch.  We do not want to add pathRoot's transform
//            // as it has been accounted for in getGlobalPlacement.  We return identity transform here.
//            TransformItem newTransform{ shape, {}, {} };
//            result.emplace_back(newTransform);
//            return result;
//        }

//        TransformItem newTransform{shape, ShapeFinder::getPlacement(currentNode), ShapeFinder::getScale(currentNode)};
//        result.emplace_back(newTransform);
//        return result;
//    }

//    int iSibling{0};
//    int newLevel = level + 1;
//    for (auto& child : linkedChildren) {
//        auto childPaths = nodeVisitor3(currentNode, child, newLevel, iSibling);
//        for (auto& path : childPaths) {
//            // we keep the shape found by lower levels
//            TransformItem newTransform{path.shape(), path.placement(), path.scale()};
//            result.emplace_back(newTransform);
//        }
//        iSibling++;
//    }
//    return result;
//}


