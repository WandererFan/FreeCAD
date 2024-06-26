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

#ifndef MEASURE_LINKCRAWLER_H
#define MEASURE_LINKCRAWLER_H

#include <Mod/Measure/MeasureGlobal.h>

#include <TopoDS_Shape.hxx>

#include <App/DocumentObject.h>
#include <App/DocumentObserver.h>
#include <Base/Placement.h>
#include <Base/Matrix.h>

#include <Mod/Part/App/TopoShape.h>

namespace Measure
{

//! a class to hold the result of resolving a selection into the actual target object
//! and traditional subElement name (Vertex1)

class MeasureExport ResolveResult
{
public:
    ResolveResult();
    ResolveResult(const App::DocumentObject* realTarget,
                  const std::string& shortSubName,
                  const App::DocumentObject* targetParent);

    App::DocumentObject& getTarget() const;
    std::string getShortSub() const;
    App::DocumentObject& getTargetParent() const;

private:
    App::SubObjectT m_target;
    App::DocumentObjectT m_targetParent;
};


//! a class to obtain the located shape pointed at by a selection
class MeasureExport ShapeFinder
{
public:
    static TopoDS_Shape getLocatedShape(const App::DocumentObject& rootObject,
                                        const std::string& leafSub);
    static Part::TopoShape getLocatedTopoShape(const App::DocumentObject& rootObject,
                                               const std::string& leafSub);

    static void crawlPlacementChain(std::vector<Base::Placement>& plmStack,
                                    std::vector<Base::Matrix4D>& scaleStack,
                                    const App::DocumentObject& rootObj,
                                    const std::string& leafSub);

    static ResolveResult resolveSelection(const App::DocumentObject& selectObj,
                                          const std::string& selectLongSub);

    static std::pair<Base::Placement, Base::Matrix4D>
    getGlobalTransform(const App::DocumentObject* cursorObject);

    static Base::Placement getPlacement(const App::DocumentObject* root);
    static Base::Matrix4D getScale(const App::DocumentObject* root);

    static std::string getFullPath(const App::DocumentObject* object);
    static std::vector<App::DocumentObject*> getGeometryRootObjects(const App::Document* doc);
    static std::vector<std::list<App::DocumentObject*> >  getGeometryPathsFromOutList(const App::DocumentObject* object);

    static TopoDS_Shape transformShape(TopoDS_Shape& inShape,
                                       const Base::Placement& placement,
                                       const Base::Matrix4D scaler);

    static bool isLinkLike(const App::DocumentObject* obj);
    static std::string PlacementAsString(const Base::Placement& inPlacement);
    static std::string LocationAsString(TopLoc_Location location);


    static std::string getLastTerm(const std::string& inString);
    static TopoDS_Shape stripInfiniteShapes(const TopoDS_Shape &inShape);
    static bool isShapeReallyNull(TopoDS_Shape shape);

    static std::pair<Base::Placement, Base::Matrix4D> sumTransforms(const std::vector<Base::Placement>& plmStack,
                                                                    const std::vector<Base::Matrix4D>& scaleStack);
    static App::DocumentObject* getLinkAttachParent(const App::DocumentObject* attachedObject);
    static Base::Placement getLinkAttachPlacement(const App::DocumentObject* attachedLinkObject);

    static Base::Placement getAttachedPlacement(const App::DocumentObject* cursorObject);


private:
    static std::string getFirstTerm(const std::string& inString);
    static std::string namesToLongSub(const std::vector<std::string>& pathElementNames);
    static std::string pruneLastTerm(const std::string& inString);
    static std::string pruneFirstTerm(const std::string& inString);
    static std::string removeGeometryTerm(const std::string& longSubname);
    static std::string pathToLongSub(std::list<App::DocumentObject*> path);

    static bool ignoreModule(const std::string& moduleName);
    static bool ignoreObject(const App::DocumentObject* object);
    static bool ignoreLinkAttachedObject(const App::DocumentObject* object,
                                     const App::DocumentObject* inlistObject);
    static std::vector<App::DocumentObject*>
    tidyInList(const std::vector<App::DocumentObject*>& inlist);
    static std::vector<App::DocumentObject*> tidyInListAttachment(const App::DocumentObject* owner,
                                                                  const std::vector<App::DocumentObject*>& inlist);


    static void combineTransforms(const std::vector<App::DocumentObject*>& pathObjects,
                                  Base::Placement& netPlacement,
                                  Base::Matrix4D& netScale);
};

}  // namespace Measure

#endif  // MEASURE_LINKCRAWLER_H
