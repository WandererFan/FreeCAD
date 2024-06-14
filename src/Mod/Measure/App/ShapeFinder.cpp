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

#include <App/Document.h>
#include <App/DocumentObjectGroup.h>
#include <App/Link.h>
#include <App/GeoFeature.h>
#include <App/GeoFeatureGroupExtension.h>
#include <App/Part.h>
#include <Base/Tools.h>

#include <Mod/Part/App/PartFeature.h>

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


//! returns the shape of rootObject+leafSub with all the relevant transforms applied to it.  The rootObject and
//! leafSub are typically obtained from Selection as it provides the appropriate longSubname.
TopoDS_Shape ShapeFinder::getLocatedShape(const App::DocumentObject& rootObject, const std::string& leafSub)
{
    Base::Console().Message("SF::getLocatedShape(%s/%s, %s)\n", rootObject.getNameInDocument(), rootObject.Label.getValue(), leafSub.c_str());
    auto resolved = resolveSelection(rootObject, leafSub);
    auto target = &resolved.getTarget();
    auto shortSub = resolved.getShortSub();
    if (!target) {
        Base::Console().Message("SF::getLocatedShape - resolve  failed to produce target\n");
    }
    Base::Console().Message("SF::getLocatedShape - target: %s/%s \n", target->getNameInDocument(), target->Label.getValue());

    Base::Placement netPlm = getPlacement(&rootObject);
    Base::Matrix4D netScale = getScale(&rootObject);
//    auto geoFeat = dynamic_cast<App::GeoFeature*>(&resolved.getTarget());
//    if (geoFeat) {
//        // globalPlacment includes target's placement and the placement of all the objects above it
//        Base::Console().Message("SF::getLocatedShape - is geofeat\n");
//        netPlm = geoFeat->globalPlacement();
//    } else {
        // get the total transform above the leaf and below the root.  This is where we deal with things like LinkElements
        // that are not GeoFeatures and do not provide a global placement
        Base::Console().Message("SF::getLocatedShape - is NOT geofeat\n");
        std::vector<Base::Placement> plmStack;
        std::vector<Base::Matrix4D> scaleStack;
        // we should only prune the last term if it is a vertex, edge or face
        // std::string newSub = pruneLastTerm(leafSub);    // remove "Vertex1"
        std::string newSub = removeGeometryTerm(leafSub);
        Base::Console().Message("SF::getLocatedShape - newSub: %s\n", newSub.c_str());
        crawlPlacementChain(plmStack, scaleStack, rootObject, newSub);
        // apply the placements from the top down to us
        auto itRevPlm = plmStack.rbegin();
        for (; itRevPlm != plmStack.rend(); itRevPlm++) {
            netPlm *= *itRevPlm;
        }
        auto itRevScale = scaleStack.rbegin();
        for (; itRevScale != scaleStack.rend(); itRevScale++) {
            netScale *= *itRevScale;
        }

        // apply the target's placement last
//        auto targetPlacement = getPlacement(target);
//        netPlm *= targetPlacement;
//        auto targetScale = getScale(target);
//        netScale *= targetScale;
//    }

        Base::Console().Message("SF::getLocatedShape - netPlm: %s\n", PlacementAsString(netPlm));

    TopoDS_Shape shape = Part::Feature::getShape(target);
    if (isShapeReallyNull(shape))  {
        return {};
    }

    Part::TopoShape tShape{shape};
    if (tShape.isInfinite()) {
        shape = stripInfiniteShapes(shape);
    }

    // copying the shape prevents "non-orthogonal GTrsf" errors in some versions
    // of OCC.  Something to do with triangulation of shape??
    // it may be that incremental mesh would work here too.
    BRepBuilderAPI_Copy copier(shape);
    tShape = Part::TopoShape(copier.Shape());
    if(tShape.isNull()) {
        Base::Console().Message("SF::getLocatedShape - no shape found\n");
        return {};
    }

    try {
        tShape.transformGeometry(netScale);
        tShape.setPlacement(netPlm);
    }
    catch (...) {
        Base::Console().Error("ShapeFinder failed to retrieve shape from %s\n", target->getNameInDocument());
        return {};
    }

    if (!shortSub.empty()) {
        return tShape.getSubTopoShape(shortSub.c_str()).getShape();
    }

    return tShape.getShape();
}

Part::TopoShape ShapeFinder::getLocatedTopoShape(const App::DocumentObject& rootObject, const std::string& leafSub)
{
    return {getLocatedShape(rootObject, leafSub)};
}

//! traverse the tree from leafSub up to rootObject, obtaining placements along the way.  Note that
//! the placements will need to be applied in the reverse order (ie top down) of what is delivered in
//! plm stack.  leafSub is a dot separated longSubName which DOES NOT include rootObject.
void ShapeFinder::crawlPlacementChain(std::vector<Base::Placement>& plmStack,
                                      std::vector<Base::Matrix4D>& scaleStack,
                                      const App::DocumentObject& rootObject,
                                      const std::string& leafSub)
{
    Base::Console().Message("SF::crawlPlacementChain(%s, %s)\n", rootObject.getNameInDocument(), leafSub.c_str());
    auto currentSub = leafSub;
    std::string previousSub{};
    while (!currentSub.empty() &&
           currentSub != previousSub) {
        auto resolved = resolveSelection(rootObject, currentSub);
        auto target = &resolved.getTarget();
        if (!target) {
            return;
        }
        Base::Console().Message("SF::crawlPlacementChain - target: %s/%s currentSub: %s\n",
                                target->getNameInDocument(), target->Label.getValue(), currentSub.c_str());
        auto currentPlacement = getPlacement(target);
        Base::Console().Message("SF::crawlPlacementChain - current Plm: %s\n", PlacementAsString(currentPlacement));
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

//! return the last term of a dot separated string - A.B.C returns C
// A.B.C. returns empty string?
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
    Base::Console().Message("SF::pruneLastTerm(%s)\n", inString.c_str());
    auto result{inString};
    if (result.back() == '.') {
        // remove the trailing dot
        result = result.substr(0, result.length() - 1);
    }
    Base::Console().Message("SF::pruneLastTerm - result (1): %s\n", result.c_str());
    size_t lastDotPos = result.rfind('.');
    if (lastDotPos != std::string::npos) {
        result = result.substr(0, lastDotPos + 1);
    } else {
        // no dot in string, remove everything!
        result = "";
    }

    Base::Console().Message("SF::pruneLastTerm - result (2): %s\n", result.c_str());
    return result;
}

//! return the Vertex part of a subName like Vertex1
// TODO: this method already exists in TechDraw, but we can't call that as it would set up a
//       circular dependency between Measure and TD
std::string ShapeFinder::getGeomTypeFromName(const std::string& geomName)
{
    if (geomName.empty()) {
        throw Base::ValueError("getGeomTypeFromName - empty geometry name");
    }

    boost::regex re("^[a-zA-Z]*");      //one or more letters at start of string
    boost::match_results<std::string::const_iterator> what;
    boost::match_flag_type flags = boost::match_default;
    std::string::const_iterator begin = geomName.begin();
    auto pos = geomName.rfind('.');
    if (pos != std::string::npos) {
        begin += pos + 1;
    }
    std::string::const_iterator end = geomName.end();
    std::stringstream ErrorMsg;

    if (boost::regex_search(begin, end, what, re, flags)) {
        return what.str();
    }

    ErrorMsg << "In getGeomTypeFromName: malformed geometry name - " << geomName;
    throw Base::ValueError(ErrorMsg.str());
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


//! this getter should work for any object, not just links
Base::Placement ShapeFinder::getPlacement(const App::DocumentObject* root)
{
    Base::Console().Message("SF::getPlacement(%s/%s)\n", root->getNameInDocument(), root->Label.getValue());
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


//inShape is a compound
//The shapes of datum features (Axis, Plan and CS) are infinite
//Infinite shapes can not be projected, so they need to be removed.
TopoDS_Shape ShapeFinder::stripInfiniteShapes(TopoDS_Shape inShape)
{
//    Base::Console().Message("SF::stripInfiniteShapes()\n");
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


//! check for shape is null or shape has no subshapes(vertex/edge/face/etc)
//! this handles the case of an empty compound which is not IsNull, but has no
//! content.
// Note: the same code exists in TechDraw::ShapeUtils
bool  ShapeFinder::isShapeReallyNull(TopoDS_Shape shape)
{
    // if the shape is null or it has no subshapes, then it is really null
    return shape.IsNull() || !TopoDS_Iterator(shape).More();
}


//! get a relative who is a top level object. used by TechDraw::ShapeExtractor
App::DocumentObject* ShapeFinder::getTopLevelObject(const App::DocumentObject* descendant)
{
    if (!isLinkLike(descendant)) {
        // will this work for features in bodies or App::Part??
        // we probably should not claim that descendant is const??
        return const_cast<App::DocumentObject*>(descendant);
    }

    // link related objects
    auto parentsAll = descendant->getParents();
    if (parentsAll.empty()) {
        return const_cast<App::DocumentObject*>(descendant);
    }


    // an object is always its own parent in this situation so parentsAll will never be empty(?)
    if (parentsAll.size() == 1  &&
        parentsAll.front().first == descendant) {
        // descendant is a top level object
        return parentsAll.front().first;
    }

    App::DocumentObject* firstGoodParent{nullptr};
    for (auto& parent : parentsAll) {
        if (parent.first->getDocument() != descendant->getDocument()) {
            // not in our document, so can't be our parent?  Links can cross documents no?
            continue;
        }
        if (!firstGoodParent) {
            firstGoodParent = parent.first;
        }

        auto resolve = ShapeFinder::resolveSelection(*(parent.first), parent.second);
    }

    if (!firstGoodParent) {
        return const_cast<App::DocumentObject*>(descendant);;
    }

    return firstGoodParent;
}


//! temporary solution.  returns cursorObject's transform + a random parent's transform.  works ok in some conditions,
//! but we don't have the full path to cursorObject so if cursorObject occurs more than once in the document,
//! we will not be able to get the correct placement.
std::pair<Base::Placement, Base::Matrix4D> ShapeFinder::getGlobalTransform(const App::DocumentObject* cursorObject)
{
    if (!cursorObject) {
        return {};
    }
    Base::Console().Message("SF::getGlobalTransform(%s/%s)\n", cursorObject->getNameInDocument(), cursorObject->Label.getValue());

    auto doc = cursorObject->getDocument();
    auto pathToCursor = getFullPath(cursorObject);
    Base::Placement netPlm;
    Base::Matrix4D netScale;
    Base::Console().Message("SF::getGlobalTransform - pathToCursor(1): **%s**\n", pathToCursor.c_str());
    if (!pathToCursor.empty()) {
        std::vector<Base::Placement> plmStack;
        std::vector<Base::Matrix4D> scaleStack;
        auto rootName = getFirstTerm(pathToCursor);
        auto rootObject = doc->getObject(rootName.c_str());
        pathToCursor = pruneFirstTerm(pathToCursor);
        Base::Console().Message("SF::getGlobalTransform - pathToCursor(2): **%s**\n", pathToCursor.c_str());
        if (!pathToCursor.empty()) {
            crawlPlacementChain(plmStack, scaleStack, *rootObject, pathToCursor);
        }
        netPlm = getPlacement(rootObject);
        netScale = getScale(rootObject);
        // apply the placements from the top down to us
        auto itRevPlm = plmStack.rbegin();
        for (; itRevPlm != plmStack.rend(); itRevPlm++) {
            netPlm *= *itRevPlm;
        }
        auto itRevScale = scaleStack.rbegin();
        for (; itRevScale != scaleStack.rend(); itRevScale++) {
            netScale *= *itRevScale;
        }

        Base::Console().Message("SF::getGlobalTransform - Method 1 pathToCursor plm; %s\n", PlacementAsString(netPlm).c_str());
//        return {netPlm, netScale};
    } else {
        Base::Console().Message("SF::getGlobalTransform - pathToCursor is empty\n");
    }

    auto parent = getTopLevelObject(cursorObject);
    auto plm = getPlacement(parent);
    auto scale = getScale(parent);
    if (parent != cursorObject)  {
        plm *= getPlacement(cursorObject);
        scale *= getScale(cursorObject);
    }
    Base::Console().Message("SF::getGlobalTransform - Method 2 parent: %s/%s  classic net plm; %s\n",
                            parent->getNameInDocument(), parent->Label.getValue(), PlacementAsString(plm).c_str());
    return { plm, scale };
}


//! if an App::Part has an InList, it will not be considered a RootObject by Document::getRootObjects().
//! For example, a TechDraw object that points at a App::Part will give the Part an InList and make
//! it not a RootObject!
std::vector<App::DocumentObject*> ShapeFinder::getRootObjects(const App::Document* doc)
{
    std::vector < App::DocumentObject* > ret;
    auto objectsAll = doc->getObjects();
    for (auto object : objectsAll) {
        auto className = object->getTypeId().getName();
        auto objModule = Base::Type::getModuleName(className);
        if (ignoreModule(objModule)) {
            continue;
        }
        auto inlist = tidyInList(object->getInList());
        if (inlist.empty()) {
            ret.push_back(object);
        }
    }
    return ret;
}


//! remove objects to be ignored from inlist.  These are objects that have links to a potential root
//! object, but are not relevant to establishing the root objects place in the dependency tree.
std::vector<App::DocumentObject*> ShapeFinder::tidyInList(const std::vector<App::DocumentObject*>& inlist)
{
    std::vector<App::DocumentObject*> cleanList;
    for (auto& obj : inlist) {
        auto className = obj->getTypeId().getName();
        auto objModule = Base::Type::getModuleName(className);
        if (ignoreModule(objModule)) {
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


//! get a dot separated path from a root level object to the input object.
// works for top level objects and objects with only one path from a root.  no solution yet
// for multiple paths.
std::string ShapeFinder::getFullPath(const App::DocumentObject* object)
{
    Base::Console().Message("SF::getFullPath(%s/%s)\n", object->getNameInDocument(), object->Label.getValue());
    auto doc = object->getDocument();
    auto rootObjects = getRootObjects(doc);
    for (auto& root : rootObjects) {
        Base::Console().Message("SF::getFullPath - a rootObject (1): %s/%s\n", root->getNameInDocument(), root->Label.getValue());
        if (root == object) {
            // easy case, object is a root.
            std::string objName{ object->getNameInDocument() };
            objName += ".";
            return { objName};
        }
    }

    // object is not a root object
    for (auto& root : rootObjects) {
        auto className = root->getTypeId().getName();
        std::string mod = Base::Type::getModuleName(className);
        Base::Console().Message("SF::getFullPath - a rootObject (2): %s/%s mod: %s\n",
                                root->getNameInDocument(), root->Label.getValue(), mod.c_str());
        auto paths = doc->getPathsByOutList(root, object);
        if (paths.empty()) {
            continue;
        }
        if (paths.size() == 1)  {
            auto longSub = pathToLongSub(paths.front());
            Base::Console().Message("SF::getFullPath - longsub from single path: %s\n", longSub.c_str());
            return longSub;
        }

        Base::Console().Message("SF::getgetFullPath - %d paths from: %s to %s\n", paths.size(),
                                root->getNameInDocument(), object->getNameInDocument());

        // we have multiple paths from a root object to object.  need to decide which path is best?
        // if App::Part + 10 points, App::Link + 5 points, Part::Feature - 10 points???
        std::vector<std::list<App::DocumentObject*> >  candidates;
        for (auto& p : paths) {
            for (auto& item : p) {
                Base::Console().Message("SF::getFullPath - DB an item: %s/%s\n",
                                        item->getNameInDocument(), item->Label.getValue());
            }
        }
    }

    return {};

}

//! remove that part of a long subelement name that refers to a geometric subshape.  "myObj.Vertex1"
//! would return "myObj.", "myObj.mySubObj." would return itself unchanged.  If there is no geometric
//! reference the original input is returned.
std::string ShapeFinder::removeGeometryTerm(const std::string& longSubname)
{
    Base::Console().Message("SF::removeGeometryTerm(%s)\n", longSubname.c_str());
    auto lastTerm = getLastTerm(longSubname);
    // auto geomType = getGeomTypeFromName(lastTerm);  // throws if not geometry?
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
        Base::Console().Message("SF::removeGeometryTerm - found edge @ pos: %d - return: %s\n", pos, pruneLastTerm(longSubname));
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


//! return inShape with placement and scaler applied.  If inShape contains any infinite subshapes
//! (such as Datum planes), the infinite shapes will not be included in the result.
TopoDS_Shape ShapeFinder::transformShape(TopoDS_Shape inShape,
                                            const Base::Placement& placement,
                                            const Base::Matrix4D scaler)
{
    if (isShapeReallyNull(inShape))  {
        return {};
    }
    Part::TopoShape tshape{inShape};
    if (tshape.isInfinite()) {
        inShape = stripInfiniteShapes(inShape);
    }

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
