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

#include "PreCompiled.h"
#ifndef _PreComp_
#endif

//#include <QSvgRenderer>

#include <Base/Console.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include "SymbolRepresentation.h"
#include "SymbolRepresentationPy.h"

using namespace TechDraw;

TYPESYSTEM_SOURCE(TechDraw::SymbolRepresentation, Base::Persistence)

SymbolRepresentation::SymbolRepresentation(std::string representation,
                                           int flags)
{
    m_fileReference = representation;
    m_flags = flags;
}

double SymbolRepresentation::width() const
{
    //QSvgRenderer renderer(getFileReference());
    return 0.0;
}

double SymbolRepresentation::height() const
{
    //QSvgRenderer renderer(getFileReference());
    return 0.0;
}

void SymbolRepresentation::Save(Base::Writer &writer) const
{

    writer.Stream() << writer.ind() << "<SymbolRepresentation>" << std::endl;
    writer.incInd();
    writer.Stream() << writer.ind() << "<FileReference value=\"" <<  m_fileReference << "\"/>" << std::endl;
    writer.Stream() << writer.ind() << "<Flags value=\"" <<  m_flags << "\"/>" << std::endl;
    writer.decInd();
    //TODO: why do we need to add the closing entry only for some objects?
    writer.Stream() << writer.ind() << "</SymbolRepresentation>" << std::endl;
}

void SymbolRepresentation::Restore(Base::XMLReader &reader)
{
    reader.readElement("SymbolRepresentation");
    reader.readElement("FileReference");
    m_fileReference = reader.getAttribute("value");
    reader.readElement("Flags");
    m_flags = reader.getAttributeAsInteger("value");
    reader.readEndElement("SymbolRepresentation");
}

void SymbolRepresentation::dump(const char* title)
{
    Base::Console().Message("SR::dump - %s \n", title);
    Base::Console().Message("SR::dump - %s \n", toString().c_str());
}

std::string SymbolRepresentation::toString() const
{
    std::stringstream ss;
    ss << m_fileReference << ", " <<
          m_flags;
    return ss.str();
}


PyObject *SymbolRepresentation::getPyObject(void)
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new SymbolRepresentationPy(this), true);
    }

    return Py::new_reference_to(PythonObject);
}
