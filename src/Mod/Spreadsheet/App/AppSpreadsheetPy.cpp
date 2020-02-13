/***************************************************************************
 *   Copyright (c) 2020 WandererFan <wandererfan@gmail.com>                *
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
# include <Python.h>
#endif

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

#include <Base/Console.h>
#include <Base/PyObjectBase.h>
#include <Base/Exception.h>

#include "Sheet.h"

namespace Spreadsheet {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("Spreadsheet")
    {
        add_varargs_method("sample",&Module::sample,
            "sample(string) -- prints string to FC console."
        );

        initialize("This module is the Spreadsheet module."); // register with Python
    }

    virtual ~Module() {}

private:
private:
    virtual Py::Object invoke_method_varargs(void *method_def, const Py::Tuple &args)
    {
        try {
            return Py::ExtensionModule<Module>::invoke_method_varargs(method_def, args);
        }
        catch (const Base::Exception &e) {
            std::string str;
            str += "FreeCAD exception thrown (";
            str += e.what();
            str += ")";
            e.ReportException();
            throw Py::RuntimeError(str);
        }
        catch (const std::exception &e) {
            std::string str;
            str += "C++ exception thrown (";
            str += e.what();
            str += ")";
            Base::Console().Error("%s\n", str.c_str());
            throw Py::RuntimeError(str);
        }
    }

    Py::Object sample(const Py::Tuple& args)
    {
        PyObject *inString;
        const char* printThis;

        if (!PyArg_ParseTuple(args.ptr(), "O", &inString)) {
            throw Py::TypeError("expected (string");
        }
        if (PyUnicode_Check(inString)) {
            printThis = PyUnicode_AsUTF8(inString);
            Base::Console().Message("AppSpreadhsheetPy::sample - %s\n", printThis);
        } else {
            Base::Console().Message("AppSpreadhsheetPy::sample - invalid input\n");
        }
        return Py::None();
    }


};

PyObject* initModule()
{
    return (new Module)->module().ptr();
}
} // namespace Spreadsheet

