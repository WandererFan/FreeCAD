/***************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen (eivind@kvedalen.name)             *
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QIcon>
# include <QImage>
# include <QFileInfo>
#endif

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

#include <Base/BaseClass.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <App/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/MDIViewPy.h>
#include <Gui/Document.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Language/Translator.h>

//******************************************************************************
//NEW
//includes added as required
#include <Mod/Spreadsheet/App/Sheet.h>
#include <Mod/Spreadsheet/App/SheetPy.h>
//******************************************************************************

#include "Workbench.h"
#include "ViewProviderSpreadsheet.h"
#include "SpreadsheetView.h"


// use a different name to CreateCommand()
void CreateSpreadsheetCommands(void);

void loadSpreadsheetResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Spreadsheet);
    Gui::Translator::instance()->refresh();
}

namespace SpreadsheetGui {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("SpreadsheetGui")
    {
        add_varargs_method("open",&Module::open
        );
//******************************************************************************        
        //NEW
        //we are adding "getSheet" method to the SpreadsheetGui module
        add_varargs_method("getSheet",&Module::getSheet,
            "sheet = getSheet(view) - returns the spreadsheet that is the subject of a MDIView"
        );
//******************************************************************************
        initialize("This module is the SpreadsheetGui module."); // register with Python
    }

    virtual ~Module() {}

private:
    Py::Object open(const Py::Tuple& args)
    {
        char* Name;
        const char* DocName=0;
        if (!PyArg_ParseTuple(args.ptr(), "et|s","utf-8",&Name,&DocName))
            throw Py::Exception();
        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        try {
            Base::FileInfo file(EncodedName);
            App::Document *pcDoc = App::GetApplication().newDocument(DocName ? DocName : QT_TR_NOOP("Unnamed"));
            Spreadsheet::Sheet *pcSheet = static_cast<Spreadsheet::Sheet *>(pcDoc->addObject("Spreadsheet::Sheet", file.fileNamePure().c_str()));

            pcSheet->importFromFile(EncodedName, '\t', '"', '\\');
            pcSheet->execute();
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }

        return Py::None();
    }

//******************************************************************************
    //NEW
    //begin getSheet implementation
    Py::Object getSheet(const Py::Tuple& args)
    {
        //PyArg_ParseTuple is pretty much the first thing to do for any extension
        //  it extracts the parameters from the input tuple and stuffs them into local variables. 
        //  it also does some checking that of types
        PyObject* pyView;
        if (!PyArg_ParseTuple(args.ptr(), "O",&pyView))
            throw Py::Exception();
 
        Gui::MDIView* mdi;
        Spreadsheet::Sheet* sheet;

        //usually we check here if the Py objects we received are the right type
        // don't know why MDIViewPy doesn't have a type. 
//        if (PyObject_TypeCheck(pyView, &(Gui::MDIViewPy::Type))) {   //MDIViewPy has no Type?
                                                                       //how to check pyView?

            //the py object has a pointer to the c++ object
            mdi = static_cast<Gui::MDIViewPy*>(pyView)->getMDIViewPtr();

            //the next bit is standard c++ 
            SpreadsheetGui::SheetView * sheetView = 
                    Base::freecad_dynamic_cast<SpreadsheetGui::SheetView>(mdi);
            if (sheetView != nullptr) {
                sheet = sheetView->getSheet();

                //make a py object from the c++ object
                return Py::asObject(new Spreadsheet::SheetPy(sheet));
            }

//        }
        //unsuccessful result
        return Py::None();
    }
    //end of getSheet implementation
//******************************************************************************

};

PyObject* initModule()
{
    return (new Module)->module().ptr();
}

} // namespace SpreadsheetGui


/* Python entry */
PyMOD_INIT_FUNC(SpreadsheetGui)
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        PyMOD_Return(0);
    }

    // instantiating the commands
    CreateSpreadsheetCommands();

    SpreadsheetGui::ViewProviderSheet::init();
    SpreadsheetGui::Workbench::init();
    SpreadsheetGui::SheetView::init();

    // add resources and reloads the translators
    loadSpreadsheetResource();

    PyObject* mod = SpreadsheetGui::initModule();
    Base::Console().Log("Loading GUI of Spreadsheet module... done\n");
    PyMOD_Return(mod);
}
