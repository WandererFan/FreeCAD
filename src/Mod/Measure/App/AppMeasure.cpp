/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#include <Base/Console.h>
#include <Base/Interpreter.h>

#include "Measurement.h"
#include "MeasurementPy.h"

// unified measurement facility
#include "MeasureBase.h"
#include "MeasureBasePy.h"

#include "MeasureAngle.h"
#include "MeasureDistance.h"
#include "MeasurePosition.h"
#include "MeasureLength.h"
#include "MeasureArea.h"
#include "MeasureRadius.h"

namespace Measure {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("Measure")
    {
        initialize("This module is the Measure module."); // register with Python
    }

private:
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

} // namespace Measure

using namespace Measure;

/* Python entry */
PyMOD_INIT_FUNC(Measure)
{
    // load dependent module
    try {
        Base::Interpreter().runString("import Part");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(nullptr);
    }
    PyObject* mod = Measure::initModule();
    // Add Types to module
    Base::Interpreter().addType(&Measure::MeasurementPy::Type, mod, "Measurement");
    Base::Interpreter().addType(&Measure::MeasureBasePy::Type, mod, "MeasureBase");

    Measure::Measurement            ::init();

    // umf classes
    Measure::MeasureBase            ::init();
    Measure::MeasurePython          ::init();
    Measure::MeasureAngle           ::init();
    Measure::MeasureDistance        ::init();
    Measure::MeasurePosition        ::init();
    Measure::MeasureLength          ::init();
    Measure::MeasureArea            ::init();
    Measure::MeasureRadius          ::init();

    // Add fundamental umf Measure Types
    App::Application& app = App::GetApplication();

    app.addMeasureType("DISTANCE",
                       "Distance",
                       "Measure::MeasureDistance",
                       MeasureDistance::isValidSelection,
                       MeasureDistance::isPrioritizedSelection
        );

    app.addMeasureType(
            "ANGLE",
            "Angle",
            "Measure::MeasureAngle",
            MeasureAngle::isValidSelection,
            MeasureAngle::isPrioritizedSelection
        );
        
    app.addMeasureType(
            "LENGTH",
            "Length",
            "Measure::MeasureLength",
            MeasureLength::isValidSelection,
            nullptr
        );

    app.addMeasureType(
            "POSITION",
            "Position",
            "Measure::MeasurePosition",
            MeasurePosition::isValidSelection,
            nullptr
        );

    app.addMeasureType(
            "AREA",
            "Area",
            "Measure::MeasureArea",
            MeasureArea::isValidSelection,
            nullptr
        );
        
    app.addMeasureType(
            "RADIUS",
            "Radius",
            "Measure::MeasureRadius",
            MeasureRadius::isValidSelection,
            MeasureRadius::isPrioritizedSelection
        );

    Base::Console().Log("Loading Measure module... done\n");
    PyMOD_Return(mod);
}

// debug print for sketchsolv 
void debugprint(const std::string& text)
{
    Base::Console().Log("%s", text.c_str());
}
