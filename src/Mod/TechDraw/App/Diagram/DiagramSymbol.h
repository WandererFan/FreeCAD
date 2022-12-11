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

#ifndef TECHDRAW_DIAGRAMSYMBOL_H
#define TECHDRAW_DIAGRAMSYMBOL_H

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <App/FeaturePython.h>
#include <Base/Persistence.h>
#include <Base/Vector3D.h>

#include "SymbolRepresentation.h"

using SymbolId = int;

namespace TechDraw {
class Diagram;
class SymbolPort;

//description of a symbol for a diagram
class TechDrawExport DiagramSymbol : Base::Persistence
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    DiagramSymbol() ;
    DiagramSymbol(std::string name,
                  SymbolRepresentation* representation,
                  Base::Vector3d location,
                  int flags);
    DiagramSymbol(DiagramSymbol& other);
    ~DiagramSymbol() override;

    // setters & getters
    Diagram* getParentDiagram() const { return m_parentDiagram; }
    void setParentDiagram(Diagram* parent) { m_parentDiagram = parent; }

    SymbolId getSymbolId() const { return m_symbolId; }
    void setSymbolId(SymbolId id) { m_symbolId = id; }

    std::string getName() const { return m_name; }
    void setName(std::string name) { m_name = name; }

    std::vector<SymbolPort*> getPorts() const { return m_ports; }
    SymbolPort* getPort(std::string portName) const;
    void setPorts(std::vector<SymbolPort*> ports) { m_ports = ports; }

    SymbolRepresentation* getRepresentation() const { return m_representation; }
    void setRepresentation(SymbolRepresentation* representation) { m_representation = representation; }

    Base::Vector3d getLocation() const { return m_location; }
    void setLocation(Base::Vector3d location);

    int getFlags() const { return m_flags; }
    void setFlags(int flags) { m_flags = flags; }

    Base::Vector3d getSize() const;

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override  { return 1; }
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;

    PyObject *getPyObject() override;

    void dump(const char* title);
    std::string toString() const;

private:
    void makeDefaultPort();

    TechDraw::Diagram* m_parentDiagram;
    SymbolId m_symbolId;
    std::string m_name;
    std::vector<SymbolPort*> m_ports;
    SymbolRepresentation* m_representation;
    Base::Vector3d m_location;
    int m_flags;

    SymbolPort* m_defaultPort;

    double m_width;
    double m_height;

    Py::Object PythonObject;
};

using DiagramSymbolPython = App::FeaturePythonT<DiagramSymbol>;

} //end namespace TechDraw

#endif //TECHDRAW_DIAGRAMSYMBOL_H

