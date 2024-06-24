// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 wandererfan <wandererfan@gmail.com>                 *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <boost_regex.hpp>

#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Iterator.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <TopLoc_Location.hxx>

#include <App/Document.h>
#include <App/DocumentObjectGroup.h>
#include <App/Link.h>
#include <App/GeoFeature.h>
#include <App/GeoFeatureGroupExtension.h>
#include <App/Part.h>
#include <Base/Tools.h>

#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/AttachExtension.h>
#include <Mod/Part/App/Attacher.h>

#include "ShapeFinder.h"

using namespace Measure;

//! ResolveResult is a class to hold the result of resolving a selection into the actual target object
//! and traditional subElement name (Vertex1).

ResolveResult::ResolveResult(const App::DocumentObject* realTarget, const std::string& shortSubName, const App::DocumentObject* targetParent) :
    m_target(App::SubObjectT(realTarget, shortSubName.c_str())),
    m_targetParent(App::DocumentObjectT(targetParent))
{}

App::DocumentObject& ResolveResult::getTarget() const
{
    return *(m_target.getObject());
}

std::string ResolveResult::getShortSub() const
{
    return m_target.getSubName();
}

App::DocumentObject& ResolveResult::getTargetParent() const
{
    return *(m_targetParent.getObject());
}



//! ShapeFinder is a class to obtain the located shape pointed at by a DocumentObject and a
//! "new-style" long subelement name. It hides the complexities of obtaining the correct object
//! and its placement.

//! returns the actual target object and subname pointed to by selectObj and selectLongSub (which
//! is likely a result from getSelection or getSelectionEx)
ResolveResult ShapeFinder::resolveSelection(const App::DocumentObject& selectObj, const std::string& selectLongSub)
{
    App::DocumentObject* targetParent{nullptr};
    std::string childName{};
    const char* subElement{nullptr};
    App::DocumentObject* realTarget = selectObj.resolve(selectLongSub.c_str(),
                                                         &targetParent,
                                                         &childName,
                                                         &subElement);
    auto shortSub = getLastTerm(selectLongSub);
    return { realTarget, shortSub, targetParent };
}


//! returns the shape of rootObject+leafSub with all the transforms applied to it.  The rootObject and
//! leafSub are typically obtained from Selection as it provides the appropriate longSubname.  The leaf
//! sub string can also be constructed by walking the tree.
// this is just getshape, getglobaltransform, apply & return?
// TODO: to truly locate the shape, we need to consider attachments?
TopoDS_Shape ShapeFinder::getLocatedShape(const App::DocumentObject& rootObject, const std::string& leafSub)
{
    auto resolved = resolveSelection(rootObject, leafSub);
    auto target = &resolved.getTarget();
    auto shortSub = resolved.getShortSub();
    if (!target) {
        return {};
    }

    Base::Console().Message("SF::getLocatedShape(%s/%s - %s)\n", rootObject.getNameInDocument(),
                            rootObject.Label.getValue(), leafSub.c_str());

    TopoDS_Shape shape = Part::Feature::getShape(target);
    if (isShapeReallyNull(shape))  {
        return {};
    }

    // we prune the last term if it is a vertex, edge or face
    std::string newSub = removeGeometryTerm(leafSub);

    std::vector<Base::Placement> plmStack;
    std::vector<Base::Matrix4D> scaleStack;
    // get transforms below rootObject
    // Note: root object is provided by the caller and may or may not be a top level object
    crawlPlacementChain(plmStack, scaleStack, rootObject, newSub);
    auto pathTransform = sumTransforms(plmStack, scaleStack);

    // apply the placements in reverse order - top to bottom
    // should this be rootObject's local transform?
    auto rootTransform = getGlobalTransform(&rootObject);
    auto netPlm = rootTransform.first * pathTransform.first;
    auto netScale = rootTransform.second * pathTransform.second;

    shape = transformShape(shape, netPlm, netScale);
    Part::TopoShape tShape{shape};
    if (!shortSub.empty()) {
        return tShape.getSubTopoShape(shortSub.c_str()).getShape();
    }

    return tShape.getShape();
}


//! convenient version of previous method
Part::TopoShape ShapeFinder::getLocatedTopoShape(const App::DocumentObject& rootObject, const std::string& leafSub)
{
    return {getLocatedShape(rootObject, leafSub)};
}


//! traverse the tree from leafSub up to rootObject, obtaining placements along the way.  Note that
//! the placements will need to be applied in the reverse order (ie top down) of what is delivered in
//! plm stack.  leafSub is a dot separated longSubName which DOES NOT include rootObject.  the result
//! does not include rootObject's transform.
void ShapeFinder::crawlPlacementChain(std::vector<Base::Placement>& plmStack,
                                      std::vector<Base::Matrix4D>& scaleStack,
                                      const App::DocumentObject& rootObject,
                                      const std::string& leafSub)
{
    auto currentSub = leafSub;
    std::string previousSub{};
    while (!currentSub.empty() &&
           currentSub != previousSub) {
        auto resolved = resolveSelection(rootObject, currentSub);
        auto target = &resolved.getTarget();
        if (!target) {
            return;
        }
        auto currentPlacement = getPlacement(target);
        auto currentScale = getScale(target);
        if (!currentPlacement.isIdentity() ||
            !currentScale.isUnity()) {
            plmStack.push_back(currentPlacement);
            scaleStack.push_back(currentScale);
        }
        previousSub = currentSub;
        currentSub = pruneLastTerm(currentSub);
    }
}


//! return inShape with placement and scaler applied.  If inShape contains any infinite subshapes
//! (such as Datum planes), the infinite shapes will not be included in the result.
TopoDS_Shape ShapeFinder::transformShape(TopoDS_Shape& inShape,
                                         const Base::Placement& placement,
                                         const Base::Matrix4D scaler)
{
    if (isShapeReallyNull(inShape))  {
        return {};
    }
    // we modify the parameter shape here.  we don't claim to be const, but may be better to copy the
    // shape?
    Part::TopoShape tshape{inShape};
    if (tshape.isInfinite()) {
        inShape = stripInfiniteShapes(inShape);
    }
    Base::Console().Message("SF::transformShape - plm: %s  loc: %s\n", PlacementAsString(placement).c_str(),
                            LocationAsString(inShape.Location()).c_str());

    // copying the shape prevents "non-orthogonal GTrsf" errors in some versions
    // of OCC.  Something to do with triangulation of shape??
    // it may be that incremental mesh would work here too.
    BRepBuilderAPI_Copy copier(inShape);
    tshape = Part::TopoShape(copier.Shape());
    if(tshape.isNull()) {
        return {};
    }

    tshape.transformGeometry(scaler);
    tshape.setPlacement(placement);

    return tshape.getShape();
}


//! this getter should work for any object, not just links
Base::Placement ShapeFinder::getPlacement(const App::DocumentObject* root)
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
Base::Matrix4D ShapeFinder::getScale(const App::DocumentObject* root)
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


//! there isn't convenient common ancestor for the members of the Link family.  We use isLinkLike(obj)
//! instead of obj->isDerivedFrom<ConvenientCommonAncestor>().  Some links have proxy objects and will
//! not be detected by isDerivedFrom().
bool ShapeFinder::isLinkLike(const App::DocumentObject* obj)
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



//! Infinite shapes can not be projected, so they need to be removed. inShape is usually a compound.
//! Datum features (Axis, Plane and CS) are examples of infinite shapes.
TopoDS_Shape ShapeFinder::stripInfiniteShapes(TopoDS_Shape inShape)
{
    // if this is a compound, the children will lose their location as it will not be combined with
    // that of the senior shapes.
    TopLoc_Location inLocation{inShape.Location()};
    Base::Console().Message("SF::stripInfiniteShapes - inShape - loc: %s\n", LocationAsString(inLocation).c_str());

    BRep_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);

    TopoDS_Iterator it(inShape);
    for (; it.More(); it.Next()) {
        TopoDS_Shape s = it.Value();
        Base::Console().Message("SF::stripInfiniteShapes - a shape before - loc: %s\n", LocationAsString(s.Location()).c_str());
        if (s.ShapeType() < TopAbs_SOLID) {
            //look inside composite shapes
            s = stripInfiniteShapes(s);
        } else if (Part::TopoShape(s).isInfinite()) {
            Base::Console().Message("SF::stripInfiniteShapes - skipping infinite\n");
            continue;
        }
        //simple shape & finite
        Base::Console().Message("SF::stripInfiniteShapes - a shape after - loc: %s\n", LocationAsString(s.Location()).c_str());
        builder.Add(comp, s);
    }

    // rebuilding the shape loses its location, so we need to reapply here.
    comp.Location(inLocation);

    return TopoDS_Shape(std::move(comp));
}


//! check for shape is null or shape has no subshapes(vertex/edge/face/etc)
//! this handles the case of an empty compound which is not IsNull, but has no
//! content.
// Note: the same code exists in TechDraw::ShapeUtils
bool  ShapeFinder::isShapeReallyNull(TopoDS_Shape shape)
{
    // if the shape is null or it has no subshapes, then it is really null
    return shape.IsNull() || !TopoDS_Iterator(shape).More();
}


//! temporary solution.  depends on getFullPath's ability to find the right path from a root level
//! down the tree to cursorObject. works ok in some conditions.
//! Returns the net transformation of a path from root level to cursor object.
std::pair<Base::Placement, Base::Matrix4D> ShapeFinder::getGlobalTransform(const App::DocumentObject* cursorObject)
{
    if (!cursorObject) {
        return {};
    }
    Base::Console().Message("SF::getGlobalTransform(%s/%s)\n",
                            cursorObject->getNameInDocument(),
                            cursorObject->Label.getValue());
    Base::Console().Message("SF::getGlobalTransform - islinkLike: %d\n", isLinkLike(cursorObject));
    // if cursorObject is really a geoFeature, we should be able to use globalPosition() here.  But
    // if it is a link that is pretending to be a geoFeature() then globalPosition will not be correct.

    auto pathToCursor = getFullPath(cursorObject);

    if (pathToCursor.empty()) {
        Base::Console().Message("SF::getGlobalTransform - no path Plm: %s\n", PlacementAsString(getPlacement(cursorObject)).c_str());
        return { getPlacement(cursorObject), getScale(cursorObject) };
    }

    Base::Console().Message("SF::getGlobalTransform - pathToCursor: %s\n", pathToCursor.c_str());

    auto rootName = getFirstTerm(pathToCursor);
    auto doc = cursorObject->getDocument();
    // rootObject is a top level object thanks to getFullPath()
    auto rootObject = doc->getObject(rootName.c_str());
    if (rootObject == cursorObject) {
        // we are root.
        Base::Console().Message("SF::getGlobalTransform - **** root Plm: %s\n", PlacementAsString(getPlacement(rootObject)).c_str());
        return {getPlacement(rootObject), getScale(rootObject)};
    }

    pathToCursor = pruneFirstTerm(pathToCursor);

    std::vector<Base::Placement> plmStack;
    std::vector<Base::Matrix4D> scaleStack;
    if (!pathToCursor.empty()) {
        crawlPlacementChain(plmStack, scaleStack, *rootObject, pathToCursor);
    }

    auto pathTransform = sumTransforms(plmStack, scaleStack);
    Base::Placement netPlm = getPlacement(rootObject) * pathTransform.first;
    Base::Matrix4D netScale = getScale(rootObject) * pathTransform.second;

    Base::Placement geoPlm;
    auto geoCursor = dynamic_cast<const App::GeoFeature*>(cursorObject);
    if (!isLinkLike(cursorObject) && geoCursor) {
        Base::Console().Message("SF::getGlobalTransform - have a geoCursor\n");
        // geoPlm = geoCursor->globalPlacement();
        netPlm = geoCursor->globalPlacement();
        // scale with pathScale?
    }

    Base::Console().Message("SF::getGlobalTransform - netPlm: %s\n", PlacementAsString(netPlm).c_str());
    Base::Console().Message("SF::getGlobalTransform - geoPlm: %s\n", PlacementAsString(geoPlm).c_str());

    return { netPlm, netScale };
}


//! get a dot separated path from a root level object to the input object.  result starts at a root
//! and ends at object - root.intermediate1.intermediate2.object
//! since we have no real way of deciding when there are multiple paths from a root to object,
//! we will use shortest path as a temporary selection heuristic
// "and a miracle occurs here"
std::string ShapeFinder::getFullPath(const App::DocumentObject* object)
{

    Base::Console().Message("SF::getFullPath(%s / %s)\n", object->getNameInDocument(), object->Label.getValue());
    auto doc = object->getDocument();
    auto rootObjects = getGeometryRootObjects(doc);
    for (auto& root : rootObjects) {
        if (root == object) {
            // easy case, object is a root.
            std::string objName{ object->getNameInDocument() };
            objName += ".";
            return { objName };
        }
    }

    // object is not a root object
    auto candidatePaths = getGeometryPathsFromOutList(object);
    Base::Console().Message("SF::getFullPath - candidates: %d\n", candidatePaths.size());

    // placeholder algo for best path
    size_t shortestCount{INT_MAX};
    std::list<App::DocumentObject*> shortestPath;
    for (auto& path : candidatePaths) {
        if (path.size() < shortestCount) {
            shortestCount = path.size();
            shortestPath = path;
        }
    }

    return pathToLongSub(shortestPath);
}


//! if an App::Part has an InList, it will not be considered a RootObject by Document::get RootObjects().
//! For example, a TechDraw object that points at a root level App::Part will give the Part an InList and make
//! it not a RootObject! we are only interested in roots that affect geometry.
std::vector<App::DocumentObject*> ShapeFinder::getGeometryRootObjects(const App::Document* doc)
{
    std::vector < App::DocumentObject* > ret;
    auto objectsAll = doc->getObjects();
    for (auto object : objectsAll) {
        // skip anything from a non-geometric module
        if (ignoreObject(object)) {
            continue;
        }

        auto inlist = tidyInList(object->getInList());
        if (inlist.empty()) {
            ret.push_back(object);
        }

        inlist = tidyInListAttachment(object, inlist);
        if (inlist.empty()) {
            ret.push_back(object);
        }
    }
    return ret;
}


//! combine a series of placement & scale transforms.  The input stacks are expected in leaf to root
//! order, but the result is in the expected root to leaf order.
std::pair<Base::Placement, Base::Matrix4D> ShapeFinder::sumTransforms(const std::vector<Base::Placement>& plmStack,
                                                                      const std::vector<Base::Matrix4D>& scaleStack)
{
    Base::Placement netPlm;
    Base::Matrix4D netScale;

    auto itRevPlm = plmStack.rbegin();
    for (; itRevPlm != plmStack.rend(); itRevPlm++) {
        netPlm *= *itRevPlm;
    }
    auto itRevScale = scaleStack.rbegin();
    for (; itRevScale != scaleStack.rend(); itRevScale++) {
        netScale *= *itRevScale;
    }

    return { netPlm, netScale };
}


//! get all the paths from top level objects to object.  Paths that pass through objects that do not
//! affect geometry are dropped.  Removing false paths should make finding the right one easier(?)
std::vector<std::list<App::DocumentObject*> >  ShapeFinder::getGeometryPathsFromOutList(const App::DocumentObject* object)
{
    auto doc = object->getDocument();
    auto rootObjects = getGeometryRootObjects(doc);

    std::vector<std::list<App::DocumentObject*> > geometryPaths;
    for (auto& root : rootObjects) {
        auto pathsAll = doc->getPathsByOutList(root, object);
        if (pathsAll.empty()) {
            continue;
        }

        for (auto& path : pathsAll) {
            bool usePath{true};
            for (auto& object : path) {
                if (ignoreObject(object)) {
                    usePath = false;
                    break;
                }
            }
            if (usePath) {
                geometryPaths.emplace_back(path);
            }
        }
    }
    return geometryPaths;
}


//! remove objects to be ignored from inlist.  These are objects that have links to a potential root
//! object, but are not relevant to establishing the root objects place in the dependency tree.
// not sure this does the intended job.
std::vector<App::DocumentObject*> ShapeFinder::tidyInList(const std::vector<App::DocumentObject*>& inlist)
{
    std::vector<App::DocumentObject*> cleanList;
    for (auto& obj : inlist) {
        if (ignoreObject(obj)) {
                continue;
        }

        cleanList.emplace_back(obj);
    }
    return cleanList;
}


//! remove objects to be ignored from inlist.  These are objects that have attacher connections
//! to a potential root object, but are not relevant to establishing the root objects place in
//! the dependency tree.
// not sure this does the intended job.
std::vector<App::DocumentObject*> ShapeFinder::tidyInListAttachment(const App::DocumentObject* owner,
                                                                    const std::vector<App::DocumentObject*>& inlist)
{
    std::vector<App::DocumentObject*> cleanList;
    for (auto& obj : inlist) {
        if (ignoreAttachedObject(owner, obj)) {
                continue;
        }

        cleanList.emplace_back(obj);
    }
    return cleanList;
}


//! lookup moduleName in the list of modules to ignore.
bool ShapeFinder::ignoreModule(const std::string& moduleName)
{
    std::vector<std::string> ModulesToIgnore {
                                              "TechDraw",
                                              "Spreadsheet",
                                              "Measure" };
    for (auto& module : ModulesToIgnore) {
        if (moduleName == module) {
            return true;
        }
    }
    return false;
}

//! a more convenient version of ignoreModule
bool ShapeFinder::ignoreObject(const App::DocumentObject* object)
{
    if (!object) {
        return true;
    }
    auto className = object->getTypeId().getName();
    auto objModule = Base::Type::getModuleName(className);
    if (ignoreModule(objModule)) {
        return true;
    }
    return false;
}


//! true if inlistObject is attached to cursorObject.  In this case the attachment creates a inlist
//! entry that prevents it from being considered a root object.
bool ShapeFinder::ignoreAttachedObject(const App::DocumentObject* object,
                                       const App::DocumentObject* inlistObject)
{
    if (!object || !inlistObject) {
        return true;  // ????
    }

    auto parent = getLinkAttachParent(inlistObject);
    if (parent == object) {
        return true;
    }
    return false;
}


App::DocumentObject* ShapeFinder::getLinkAttachParent(const App::DocumentObject* attachedObject)
{
    auto namedProperty = attachedObject->getPropertyByName("a1AttParent");
    auto attachProperty = dynamic_cast<App::PropertyLink*>(namedProperty);
    if (namedProperty && attachProperty) {
        return attachProperty->getValue();
    }
    return {};
}


Base::Placement ShapeFinder::getAttachedPlacement(const App::DocumentObject* cursorObject)
{
    auto extension = cursorObject->getExtensionByType<Part::AttachExtension>();
    if (!extension) {
        Base::Console().Message("SF::getAttachedPlacement - no extension\n");
        return {};
    }
    auto attachExt = dynamic_cast<Part::AttachExtension*>(extension);
    if (!attachExt  ||
        !attachExt->isAttacherActive())  {
        Base::Console().Message("SF::getAttachedPlacement - no AttachExtension or not active\n");
        return {};
    }

//    App::PropertyPlacement placementProperty;
//    placementProperty = attachExt->getPlacement();
    auto origPlacement = getPlacement(cursorObject);
    Base::Console().Message("SF::getAttachedPlacement - original Plm: %s\n", PlacementAsString(origPlacement));

    bool subChanged{false};
    auto attachedPlm = attachExt->attacher().calculateAttachedPlacement(origPlacement, &subChanged);

    Base::Console().Message("SF::getAttachedPlacement - result: %s\n", PlacementAsString(attachedPlm));
    return attachedPlm;
}


//! unravel the mysteries of attachment as implemented by Links.  Not the same as Part::AttachExtension.
Base::Placement ShapeFinder::getLinkAttachPlacement(const App::DocumentObject* attachedLinkObject)
{
    if (!isLinkLike(attachedLinkObject)) {
        Base::Console().Message("SF::getLinkAttachPlacement - %s is not a link\n", attachedLinkObject->Label.getValue());
        return {};
    }

    // are we really attached
    auto namedProperty = attachedLinkObject->getPropertyByName("a1AttParent");
    auto attachProperty = dynamic_cast<App::PropertyLink*>(namedProperty);
    if (!namedProperty || !attachProperty) {
        Base::Console().Message("SF::getLinkAttachPlacement - %s has no a1AttParent\n", attachedLinkObject->Label.getValue());
        return {};
    }
    auto parentObj = attachProperty->getValue();
    auto parentObjPlm = getPlacement(parentObj);

    // the placement of the cs to which we are attached
    namedProperty = attachedLinkObject->getPropertyByName("a3AttParentSubobjPlacement");
    auto parentPlmProperty = dynamic_cast<App::PropertyPlacement*>(namedProperty);
    if (!namedProperty || !parentPlmProperty) {
        Base::Console().Message("SF::getLinkAttachPlacement - %s has no a3AttParentSubobjPlacement\n", attachedLinkObject->Label.getValue());
        return {};
    }
    auto parentConnectPlm = parentPlmProperty->getValue();

    // the placement of our end of the attachment
    // namedProperty = attachedLinkObject->getPropertyByName("c3AttChildResultPlc");
    namedProperty = attachedLinkObject->getPropertyByName("b3AttChildSubobjPlacement");
    auto childPlmProperty = dynamic_cast<App::PropertyPlacement*>(namedProperty);
    if (!namedProperty || !childPlmProperty) {
//        Base::Console().Message("SF::getLinkAttachPlacement - %s has no c3AttChildResultPlc\n", attachedLinkObject->Label.getValue());
        Base::Console().Message("SF::getLinkAttachPlacement - %s has no b3AttChildSubobjPlacement\n", attachedLinkObject->Label.getValue());
        return {};
    }
    auto childConnectPlm = childPlmProperty->getValue();

    auto childObjPlm = getPlacement(attachedLinkObject);

//    Base::Console().Message("SP::getLinkAttachPlacement - parentPlm: %s\n", PlacementAsString(parentPlm).c_str());
//    Base::Console().Message("SP::getLinkAttachPlacement - childPlm: %s\n", PlacementAsString(childPlm).c_str());
//    Base::Console().Message("SP::getLinkAttachPlacement - netPlm: %s\n", PlacementAsString(parentPlm * childPlm).c_str());

    return parentObjPlm * parentConnectPlm * childObjPlm * childConnectPlm;
}

// long subname manipulators



// class TechDrawExport ShapeFinder : SubnameManipulator {}
// class TechDrawExport SubnameManipulator {}

std::string ShapeFinder::pathToLongSub(std::list<App::DocumentObject*> path)
{
    std::vector<std::string> elementNames;
    for (auto& item : path) {
        auto name = item->getNameInDocument();
        if (!name)  {
            continue;
        }
        elementNames.emplace_back(name);
    }
    return namesToLongSub(elementNames);
}


//! construct dot separated long subelement name from a list of elements.  the elements should be
//! in topological order.
std::string ShapeFinder::namesToLongSub(const std::vector<std::string>& pathElementNames)
{
    std::string result;
    for (auto& name : pathElementNames) {
        result += (name + ".");
    }
    return result;
}


//! return the last term of a dot separated string - A.B.C returns C
std::string ShapeFinder::getLastTerm(const std::string& inString)
{
    auto result{inString};
    size_t lastDot = inString.rfind('.');
    if (lastDot != std::string::npos) {
        result = result.substr(lastDot + 1);
    }
    return result;
}

//! return the first term of a dot separated string - A.B.C returns A
std::string ShapeFinder::getFirstTerm(const std::string& inString)
{
    auto result{inString};
    size_t lastDot = inString.find('.');
    if (lastDot != std::string::npos) {
        result = result.substr(0, lastDot);
    }
    return result;
}

//! remove the first term of a dot separated string - A.B.C returns B.C
std::string ShapeFinder::pruneFirstTerm(const std::string& inString)
{
    auto result{inString};
    size_t lastDot = inString.find('.');
    if (lastDot != std::string::npos) {
        result = result.substr(lastDot + 1);
    }
    return result;
}

//! return a dot separated string without its last term - A.B.C returns A.B.
// A.B.C. returns A.B.C
std::string ShapeFinder::pruneLastTerm(const std::string& inString)
{
    auto result{inString};
    if (result.back() == '.') {
        // remove the trailing dot
        result = result.substr(0, result.length() - 1);
    }

    size_t lastDotPos = result.rfind('.');
    if (lastDotPos != std::string::npos) {
        result = result.substr(0, lastDotPos + 1);
    } else {
        // no dot in string, remove everything!
        result = "";
    }

    return result;
}

//! remove that part of a long subelement name that refers to a geometric subshape.  "myObj.Vertex1"
//! would return "myObj.", "myObj.mySubObj." would return itself unchanged.  If there is no geometric
//! reference the original input is returned.
std::string ShapeFinder::removeGeometryTerm(const std::string& longSubname)
{
    auto lastTerm = getLastTerm(longSubname);
    if (longSubname.back() == '.') {
        // not a geometric reference
        return longSubname;  // need a copy?
    }

    // brute force check for geometry names in the last term
    auto pos = lastTerm.find("Vertex");
    if (pos != std::string::npos) {
        return pruneLastTerm(longSubname);
    }

    pos = lastTerm.find("Edge");
    if (pos != std::string::npos) {
        return pruneLastTerm(longSubname);
    }

    pos = lastTerm.find("Face");
    if (pos != std::string::npos) {
        return pruneLastTerm(longSubname);
    }

    pos = lastTerm.find("Shell");
    if (pos != std::string::npos) {
        return pruneLastTerm(longSubname);
    }

    pos = lastTerm.find("Solid");
    if (pos != std::string::npos) {
        return pruneLastTerm(longSubname);
    }

    return longSubname;
}


//! debugging routine that returns a string representation of a placement.
// TODO: this should be in Base::Placement?
std::string ShapeFinder::PlacementAsString(const Base::Placement& inPlacement)
{
    auto position = inPlacement.getPosition();
    auto rotation = inPlacement.getRotation();
    Base::Vector3d axis;
    double angle{0.0};
    rotation.getValue(axis, angle);
    std::stringstream  ss;
    ss << "pos: (" << position.x << ", " << position.y << ", " << position.z <<
        ")  axis: (" << axis.x << ", " << axis.y << ", " << axis.z << ")  angle: " << Base::toDegrees(angle);
    return ss.str();
}


//! debug routine. return readable form of TopLoc_Location from OCC
std::string ShapeFinder::LocationAsString(TopLoc_Location location)
{
    auto position = Base::Vector3d{location.Transformation().TranslationPart().X(),
                                   location.Transformation().TranslationPart().Y(),
                                    location.Transformation().TranslationPart().Z()};
    gp_XYZ axisDir;
    double angle;
    auto isRotation = location.Transformation().GetRotation(axisDir, angle);
    Base::Vector3d axis{axisDir.X(), axisDir.Y(), axisDir.Z()};

    std::stringstream  ss;
    ss << "isRotation: " << isRotation << "  pos: (" << position.x << ", " << position.y << ", " << position.z <<
        ")  axis: (" << axisDir.X() << ", " << axisDir.Y() << ", " << axisDir.Z() << ")  angle: " << Base::toDegrees(angle);
    return ss.str();
}


