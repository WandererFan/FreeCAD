/***************************************************************************
 *   Copyright (c) 2022 WandererFan <wandererfan@gmail.com>                *
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

#ifndef TECHDRAW_SYMBOLREPRESENTATION_H
#define TECHDRAW_SYMBOLREPRESENTATION_H

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <App/FeaturePython.h>
#include <Base/Persistence.h>
#include <Base/Vector3D.h>

namespace TechDraw {

//The graphical protrayal of a symbol
class TechDrawExport SymbolRepresentation: Base::Persistence
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    SymbolRepresentation() {}
    SymbolRepresentation(std::string representation,
                         int flags);
    ~SymbolRepresentation() override = default;

    virtual double width() const;
    virtual double height() const;

    // setters & getters
    //TODO: name change to getInstructions. different derived classes can provide Svg string, QImage, QPainterPath
    std::string getFileReference() const { return m_fileReference; }
    void setFileReference(std::string fileReference) { m_fileReference = fileReference; }
    int getFlags() const { return m_flags; }
    void setFlags(int flags) { m_flags = flags; }

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override  { return 1; }
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;

    PyObject *getPyObject() override;

    void dump(const char* title);
    std::string toString() const;

private:

    std::string m_fileReference;
    int m_flags;

    Py::Object PythonObject;
};

using SymbolRepresentationPython = App::FeaturePythonT<SymbolRepresentation>;

}  //end namespace

#endif  //TECHDRAW_DIAGRAMTRACE_H
