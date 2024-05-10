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

namespace Measure {

//! a class to hold the result of resolving a selection into the actual target object
//! and traditional subElement name (Vertex1)

class MeasureExport ResolveResult
{
public:
    ResolveResult();
    ResolveResult(const App::DocumentObject* realTarget, const std::string& shortSubName, const App::DocumentObject* targetParent);
    
    App::DocumentObject& getTarget() const;
    std::string getShortSub() const;
    App::DocumentObject& getTargetParent() const;

private:
    App::SubObjectT m_target;
    App::DocumentObjectT m_targetParent;
};


//! a class to obtain the located shape pointed at by a selection
class MeasureExport LinkCrawler
{
public:
    static TopoDS_Shape getLocatedShape(const App::DocumentObject& rootObject, const std::string& leafSub);

    static void crawlPlacementChain(std::vector<Base::Placement>& plmStack,
                                                            const App::DocumentObject& rootObj, const
                                                            std::string& leafSub);

    static ResolveResult resolveSelection(const App::DocumentObject& selectObj, const std::string& selectLongSub);
    static std::string getLastTerm(const std::string& inString);

private:
    static std::string pruneLastTerm(const std::string& inString);
    static std::string getGeomTypeFromName(const std::string& geomName);
    static std::string PlacementAsString(const Base::Placement& inPlacement);
};

} // namespace Measure

#endif // MEASURE_LINKCRAWLER_H
