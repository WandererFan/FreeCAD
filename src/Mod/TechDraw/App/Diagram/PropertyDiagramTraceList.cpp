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

#include "PropertyDiagramTraceList.h"
#include "DiagramTrace.h"
#include "DiagramTracePy.h"


using namespace App;
using namespace Base;
using namespace TechDraw;

//**************************************************************************
// PropertyDiagramTraceList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(TechDraw::PropertyDiagramTraceList, App::PropertyLists)

//**************************************************************************
// Construction/Destruction


PropertyDiagramTraceList::PropertyDiagramTraceList()
{

}

void PropertyDiagramTraceList::setSize(int newSize)
{
    for (unsigned int i = newSize; i < _lValueList.size(); i++)
        delete _lValueList[i];
    _lValueList.resize(newSize);
}

int PropertyDiagramTraceList::getSize() const
{
    return static_cast<int>(_lValueList.size());
}

void PropertyDiagramTraceList::setValue(DiagramTrace* lValue)
{
    if (lValue) {
        aboutToSetValue();
        _lValueList.resize(1);
        _lValueList[0] = lValue;
        hasSetValue();
    }
}

void PropertyDiagramTraceList::setValues(const std::vector<DiagramTrace*>& lValue)
{
    aboutToSetValue();
    _lValueList.resize(lValue.size());
    for (unsigned int i = 0; i < lValue.size(); i++)
        _lValueList[i] = lValue[i];
    hasSetValue();
}

PyObject *PropertyDiagramTraceList::getPyObject()
{
    PyObject* list = PyList_New(getSize());
    for (int i = 0; i < getSize(); i++)
        PyList_SetItem( list, i, _lValueList[i]->getPyObject());
    return list;
}

void PropertyDiagramTraceList::setPyObject(PyObject *value)
{
    // check container of this property to notify about changes

    if (PySequence_Check(value)) {
        Py_ssize_t nSize = PySequence_Size(value);
        std::vector<DiagramTrace*> values;
        values.resize(nSize);

        for (Py_ssize_t i=0; i < nSize; ++i) {
            PyObject* item = PySequence_GetItem(value, i);
            if (!PyObject_TypeCheck(item, &(DiagramTracePy::Type))) {
                std::string error = std::string("types in list must be 'DiagramTrace', not ");
                error += item->ob_type->tp_name;
                throw Base::TypeError(error);
            }

            values[i] = static_cast<DiagramTracePy*>(item)->getDiagramTracePtr();
        }

        setValues(values);
    }
    else if (PyObject_TypeCheck(value, &(DiagramTracePy::Type))) {
        DiagramTracePy  *pcObject = static_cast<DiagramTracePy*>(value);
        setValue(pcObject->getDiagramTracePtr());
    }
    else {
        std::string error = std::string("type must be 'DiagramTrace' or list of 'DiagramTrace', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyDiagramTraceList::Save(Writer &writer) const
{
    writer.Stream() << writer.ind() << "<DiagramTraceList count=\"" << getSize() << "\">"
                    << std::endl;
    writer.incInd();
    for (int i = 0; i < getSize(); i++) {
        writer.Stream() << writer.ind() << "<DiagramTrace  type=\""
                        << _lValueList[i]->getTypeId().getName() << "\">" << std::endl;
        writer.incInd();
        _lValueList[i]->Save(writer);
        writer.decInd();
        writer.Stream() << writer.ind() << "</DiagramTrace>" << std::endl;
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</DiagramTraceList>" << std::endl;
}

void PropertyDiagramTraceList::Restore(Base::XMLReader &reader)
{
    // read my element
    reader.clearPartialRestoreObject();
    reader.readElement("DiagramTraceList");
    // get the value of my attribute
    int count = reader.getAttributeAsInteger("count");
    std::vector<DiagramTrace*> values;
    values.reserve(count);
    for (int i = 0; i < count; i++) {
        reader.readElement("DiagramTrace");
        const char* TypeName = reader.getAttribute("type");
        DiagramTrace *newTrace = static_cast<DiagramTrace *>(Base::Type::fromName(TypeName).createInstance());
        newTrace->Restore(reader);

        if(reader.testStatus(Base::XMLReader::ReaderStatus::PartialRestoreInObject)) {
            Base::Console().Error("DiagramTrace \"%s\" within a PropertyDiagramTraceList was subject to a partial restore.\n", reader.localName());
            if(isOrderRelevant()) {
                // Pushes the best try by the DiagramTrace class
                values.push_back(newTrace);
            }
            else {
                delete newTrace;
            }
            reader.clearPartialRestoreObject();
        }
        else {
            values.push_back(newTrace);
        }

        reader.readEndElement("DiagramTrace");
    }

    reader.readEndElement("DiagramTraceList");

    // assignment
    setValues(values);
}

App::Property *PropertyDiagramTraceList::Copy() const
{
    PropertyDiagramTraceList *p = new PropertyDiagramTraceList();
    p->setValues(_lValueList);
    return p;
}

void PropertyDiagramTraceList::Paste(const Property &from)
{
    const PropertyDiagramTraceList& FromList = dynamic_cast<const PropertyDiagramTraceList&>(from);
    setValues(FromList._lValueList);
}

unsigned int PropertyDiagramTraceList::getMemSize() const
{
    int size = sizeof(PropertyDiagramTraceList);
    for (int i = 0; i < getSize(); i++)
        size += _lValueList[i]->getMemSize();
    return size;
}
