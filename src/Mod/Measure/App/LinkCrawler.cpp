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

#include <App/Link.h>
#include <App/GeoFeature.h>
#include <Mod/Part/App/PartFeature.h>

#include "LinkCrawler.h"

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



//! LinKCrawler is a class to obtain the located shape pointed at by a DocumentObject and a
//! "new-style" long subelement name. It hides the complexities of obtaining the correct object
//! and its placement.

//! returns the actual target object and subname pointed to by selectObj and selectLongSub (which
//! is likely a result from getSelection or getSelectionEx)
ResolveResult LinkCrawler::resolveSelection(const App::DocumentObject& selectObj, const std::string& selectLongSub)
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


//! returns the shape of rootObject+leafSub with all the relevant placements applied to it.  The rootObject and
//! leafSub are typically obtained from Selection.
// TODO: only GeoFeature and LinkElement root objects tested so far
TopoDS_Shape LinkCrawler::getLocatedShape(const App::DocumentObject& rootObject, const std::string& leafSub)
{
    auto resolved = resolveSelection(rootObject, leafSub);
    auto shortSub = resolved.getShortSub();

    Base::Placement netPlm;
    auto geoFeat = dynamic_cast<App::GeoFeature*>(&resolved.getTarget());
    if (geoFeat) {
        // globalPlacment includes target's placement and the placement of all the objects above it
        netPlm = geoFeat->globalPlacement();
    } else {
        // get the total placement above the target.  This is where we deal with things like LinkElements
        // that are not GeoFeatures and do not provide a global placement
        std::vector<Base::Placement> plmStack;
        std::string newSub = pruneLastTerm(leafSub);    // remove "Vertex1"
        crawlPlacementChain(plmStack, rootObject, newSub);
        // apply the placements from the top down to us
        auto itRev = plmStack.rbegin();
        for (; itRev != plmStack.rend(); itRev++) {
            netPlm *= *itRev;
        }

        // apply the target's placment last
        auto target = &resolved.getTarget();
        auto targetProperty = target->getPropertyByName("Placement");
        auto targetPlmProperty = dynamic_cast<App::PropertyPlacement*>(targetProperty);
        if (targetProperty && targetPlmProperty) {
            netPlm *= targetPlmProperty->getValue();
        }
    }

    Part::TopoShape outShape = Part::Feature::getTopoShape(&resolved.getTarget());
    outShape.setPlacement(netPlm);
    if (!shortSub.empty()) {
        outShape = outShape.getSubTopoShape(shortSub.c_str());
    }

    return outShape.getShape();
}

Part::TopoShape LinkCrawler::getLocatedTopoShape(const App::DocumentObject& rootObject, const std::string& leafSub)
{
    return {getLocatedShape(rootObject, leafSub)};
}

//! traverse the tree from leafSub up to rootObject, obtaining placements along the way.  Note that
//! the placements will need to be applied in the reverse order (ie top down) of what is delivered in
//! plm stack
void LinkCrawler::crawlPlacementChain(std::vector<Base::Placement>& plmStack,
                                                              const App::DocumentObject& rootObject,
                                                              const std::string& leafSub)
{
    Base::Placement summedPlm;
    auto currentSub = leafSub;
    auto currentObject = &rootObject;
    std::string previousSub{};
    while (currentObject &&
           !currentSub.empty() &&
           currentSub != previousSub) {
        // get a placement from the current object
        auto resolved = resolveSelection(rootObject, currentSub);
        auto target = &resolved.getTarget();
        if (!target) {
            return;
        }
        auto property = target->getPropertyByName("Placement");
        auto plmProperty = dynamic_cast<App::PropertyPlacement*>(property);
        if (property && plmProperty) {
            // save this placement
            auto ancestorPlm = plmProperty->getValue();
            if (!ancestorPlm.isIdentity()) {
                plmStack.push_back(plmProperty->getValue());
            }
        }
        previousSub = currentSub;
        currentSub = pruneLastTerm(currentSub);
    }
}

//! return the last term of a dot separated string - A.B.C returns C
std::string LinkCrawler::getLastTerm(const std::string& inString)
{
    auto result{inString};
    size_t lastDot = inString.rfind('.');
    if (lastDot != std::string::npos) {
        result = result.substr(lastDot + 1);
    }
    return result;
}

//! return a dot separated string without its last term - A.B.C returns A.B
std::string LinkCrawler::pruneLastTerm(const std::string& inString)
{
    auto result{inString};
    if (result.back() == '.') {
        // skip a trailing '.'
        result = result.substr(0, result.length() - 1);
    }
    size_t lastDot = inString.rfind('.');
    if (lastDot != std::string::npos) {
        result = result.substr(0, lastDot);
    }
    return result;
}

//! return the Vertex part of a subName like Vertex1
// TODO: this method already exists in TechDraw, but we can't call that as it would set up a
//       circular dependency between Measure and TD
std::string LinkCrawler::getGeomTypeFromName(const std::string& geomName)
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
std::string LinkCrawler::PlacementAsString(const Base::Placement& inPlacement)
{
    auto position = inPlacement.getPosition();
    auto rotation = inPlacement.getRotation();
    Base::Vector3d axis;
    double angle{0.0};
    rotation.getValue(axis, angle);
    std::stringstream  ss;
    ss << "pos: (" << position.x << ", " << position.y << ", " << position.z <<
        ")  axis: (" << axis.x << ", " << axis.y << ", " << axis.z << ")  angle: " << angle;
    return ss.str();
}

