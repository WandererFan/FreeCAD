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

#include <Base/Console.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include "PropertyDiagramSymbolList.h"
#include "DiagramSymbol.h"
#include "DiagramSymbolPy.h"


using namespace App;
using namespace Base;
using namespace TechDraw;

//**************************************************************************
// PropertyDiagramSymbolList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(TechDraw::PropertyDiagramSymbolList, App::PropertyLists)

//**************************************************************************
// Construction/Destruction


PropertyDiagramSymbolList::PropertyDiagramSymbolList()
{

}

void PropertyDiagramSymbolList::setSize(int newSize)
{
    for (unsigned int i = newSize; i < _lValueList.size(); i++)
        delete _lValueList[i];
    _lValueList.resize(newSize);
}

int PropertyDiagramSymbolList::getSize() const
{
    return static_cast<int>(_lValueList.size());
}

void PropertyDiagramSymbolList::setValue(DiagramSymbol* lValue)
{
    if (lValue) {
        aboutToSetValue();
        _lValueList.resize(1);
        _lValueList[0] = lValue;
        hasSetValue();
    }
}

void PropertyDiagramSymbolList::setValues(const std::vector<DiagramSymbol*>& lValue)
{
    aboutToSetValue();
    _lValueList.resize(lValue.size());
    for (unsigned int i = 0; i < lValue.size(); i++)
        _lValueList[i] = lValue[i];
    hasSetValue();
}

PyObject *PropertyDiagramSymbolList::getPyObject()
{
    PyObject* list = PyList_New(getSize());
    for (int i = 0; i < getSize(); i++)
        PyList_SetItem( list, i, _lValueList[i]->getPyObject());
    return list;
}

void PropertyDiagramSymbolList::setPyObject(PyObject *value)
{
    // check container of this property to notify about changes

    if (PySequence_Check(value)) {
        Py_ssize_t nSize = PySequence_Size(value);
        std::vector<DiagramSymbol*> values;
        values.resize(nSize);

        for (Py_ssize_t i=0; i < nSize; ++i) {
            PyObject* item = PySequence_GetItem(value, i);
            if (!PyObject_TypeCheck(item, &(DiagramSymbolPy::Type))) {
                std::string error = std::string("types in list must be 'DiagramSymbol', not ");
                error += item->ob_type->tp_name;
                throw Base::TypeError(error);
            }

            values[i] = static_cast<DiagramSymbolPy*>(item)->getDiagramSymbolPtr();
        }

        setValues(values);
    }
    else if (PyObject_TypeCheck(value, &(DiagramSymbolPy::Type))) {
        DiagramSymbolPy  *pcObject = static_cast<DiagramSymbolPy*>(value);
        setValue(pcObject->getDiagramSymbolPtr());
    }
    else {
        std::string error = std::string("type must be 'DiagramSymbol' or list of 'DiagramSymbol', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyDiagramSymbolList::Save(Writer &writer) const
{
    writer.Stream() << writer.ind() << "<DiagramSymbolList count=\"" << getSize() << "\">"
                    << std::endl;
    writer.incInd();
    for (int i = 0; i < getSize(); i++) {
        writer.Stream() << writer.ind() << "<DiagramSymbol  type=\""
                        << _lValueList[i]->getTypeId().getName() << "\">" << std::endl;
        writer.incInd();
        _lValueList[i]->Save(writer);
        writer.decInd();
        writer.Stream() << writer.ind() << "</DiagramSymbol>" << std::endl;
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</DiagramSymbolList>" << std::endl;
}

void PropertyDiagramSymbolList::Restore(Base::XMLReader &reader)
{
    // read my element
    reader.clearPartialRestoreObject();
    reader.readElement("DiagramSymbolList");
    // get the value of my attribute
    int count = reader.getAttributeAsInteger("count");
    std::vector<DiagramSymbol*> values;
    values.reserve(count);
    for (int i = 0; i < count; i++) {
        reader.readElement("DiagramSymbol");
        const char* TypeName = reader.getAttribute("type");
        DiagramSymbol *newSymbol = static_cast<DiagramSymbol *>(Base::Type::fromName(TypeName).createInstance());
        newSymbol->Restore(reader);

        if(reader.testStatus(Base::XMLReader::ReaderStatus::PartialRestoreInObject)) {
            Base::Console().Error("DiagramSymbol \"%s\" within a PropertyDiagramSymbolList was subject to a partial restore.\n", reader.localName());
            if(isOrderRelevant()) {
                // Pushes the best try by the DiagramSymbol class
                values.push_back(newSymbol);
            }
            else {
                delete newSymbol;
            }
            reader.clearPartialRestoreObject();
        }
        else {
            values.push_back(newSymbol);
        }

        reader.readEndElement("DiagramSymbol");
    }

    reader.readEndElement("DiagramSymbolList");

    // assignment
    setValues(values);
}

App::Property *PropertyDiagramSymbolList::Copy() const
{
    PropertyDiagramSymbolList *p = new PropertyDiagramSymbolList();
    p->setValues(_lValueList);
    return p;
}

void PropertyDiagramSymbolList::Paste(const Property &from)
{
    const PropertyDiagramSymbolList& FromList = dynamic_cast<const PropertyDiagramSymbolList&>(from);
    setValues(FromList._lValueList);
}

unsigned int PropertyDiagramSymbolList::getMemSize() const
{
    int size = sizeof(PropertyDiagramSymbolList);
    for (int i = 0; i < getSize(); i++)
        size += _lValueList[i]->getMemSize();
    return size;
}
