/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
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


#ifndef GUI_VIEWPROVIDERMEASUREANGLE_H
#define GUI_VIEWPROVIDERMEASUREANGLE_H

#include <Mod/Measure/MeasureGlobal.h>

#include <QObject>

#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFMatrix.h>
#include <Inventor/fields/SoSFVec3f.h>

#include <Mod/Measure/App/MeasureAngle.h>

#include "ViewProviderMeasureBase.h"

//NOLINTBEGIN
class SoText2;
class SoTranslation;
class SoCoordinate3;
class SoIndexedLineSet;
class SoTransform;
//NOLINTEND

namespace MeasureGui
{

class MeasureGuiExport ViewProviderMeasureAngle : public MeasureGui::ViewProviderMeasureBase
{
    PROPERTY_HEADER_WITH_OVERRIDE(MeasureGui::ViewProviderMeasureAngle);

public:
    /// Constructor
    ViewProviderMeasureAngle();

    Measure::MeasureAngle* getMeasureAngle();
    void redrawAnnotation() override;
    void onGuiInit(const Measure::MeasureBase* measureObject) override;

private:
    // Fields
    SoSFFloat fieldAngle; //radians.

    SbMatrix getMatrix();
};


} //namespace MeasureGui



#endif // GUI_VIEWPROVIDERMEASUREANGLE_H
