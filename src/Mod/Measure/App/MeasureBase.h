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


#ifndef MEASURE_MEASUREBASE_H
#define MEASURE_MEASUREBASE_H

#include <Mod/Measure/MeasureGlobal.h>

#include <App/Application.h>
#include <App/DocumentObject.h>
#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <App/FeaturePython.h>
#include <Base/Quantity.h>
#include <Base/Placement.h>
#include <QString>
#include <Base/Interpreter.h>

#include "MeasureInfo.h"
#include "MapKeeper.h"
#include "HandlerTable.h"

namespace Measure
{

class MeasureExport MeasureBase : public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Measure::MeasureBase);

public:
    MeasureBase();
    ~MeasureBase() override = default;

    App::PropertyPlacement Placement;
    // App::PropertyPosition Position;

    boost::signals2::signal<void (const MeasureBase*)> signalGuiInit;
    boost::signals2::signal<void (const MeasureBase*)> signalGuiUpdate;

    //return PyObject as MeasureBasePy
    PyObject *getPyObject() override;

    // Initialize measurement properties from selection
    virtual void parseSelection(const App::MeasureSelection& selection);


    virtual QString getResultString();

    virtual std::vector<std::string> getInputProps();
    virtual App::Property* getResultProp() {return {};};
    virtual Base::Placement getPlacement();

    // Return the objects that are measured
    virtual std::vector<App::DocumentObject*> getSubject() const;

protected:
    void onDocumentRestored() override;
    void initialize();

private:

    Py::Object getProxyObject() const;

};

// Create a scriptable object based on MeasureBase
using MeasurePython = App::FeaturePythonT<MeasureBase>;

// using GeometryHandlerCB = std::function<MeasureInfo* (std::string*, std::string*)>;

template <class T>
class MeasureExport MeasureBaseExtendable : public MeasureBase
{
public:
    //! create or replace the callback for a module
    static void addGeometryHandlerCB(const std::string& measureType, const std::string& module, GeometryHandlerCB callback) {
        Measure::MapKeeper::addCallback(measureType, module, callback);
    }

    //! assign many modules to the same callback
    static void addGeometryHandlerCBs(const std::string& measureType, const std::vector<std::string>& modules, GeometryHandlerCB callback){
        // TODO: this will replace a callback with a later one.  Should we check that there isn't already a
        // handler defined for this module?
        Measure::MapKeeper::addCallbacks(measureType, modules, callback);
    }

    //! get the address of callback std::function
    static GeometryHandlerCB getGeometryHandlerCB(const std::string& measureType, const std::string& module) {
        return Measure::MapKeeper::getCallback(measureType, module);
    }

    static bool hasGeometryHandler(const std::string& measureType, const std::string& module) {
        return Measure::MapKeeper::hasCallback(measureType, module);
    }

};


} //namespace Measure


#endif // MEASURE_MEASUREBASE_H
