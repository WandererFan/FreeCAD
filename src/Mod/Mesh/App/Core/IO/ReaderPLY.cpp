// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <istream>
#endif

#include "Core/MeshIO.h"
#include "Core/MeshKernel.h"
#include <Base/Stream.h>
#include <Base/Tools.h>

#include "ReaderPLY.h"


using namespace MeshCore;

namespace MeshCore::Ply
{

enum Number
{
    int8,
    uint8,
    int16,
    uint16,
    int32,
    uint32,
    float32,
    float64
};

struct Property
{
    using first_argument_type = std::pair<std::string, int>;
    using second_argument_type = std::string;
    using result_type = bool;

    bool operator()(const std::pair<std::string, int>& x, const std::string& y) const
    {
        return x.first == y;
    }
};

}  // namespace MeshCore::Ply

using namespace MeshCore::Ply;

ReaderPLY::ReaderPLY(MeshKernel& kernel, Material* material)
    : _kernel(kernel)
    , _material(material)
{}

bool ReaderPLY::Load(std::istream& input)
{
    // http://local.wasp.uwa.edu.au/~pbourke/dataformats/ply/
    std::size_t v_count = 0, f_count = 0;
    MeshPointArray meshPoints;
    MeshFacetArray meshFacets;

    enum
    {
        unknown,
        ascii,
        binary_little_endian,
        binary_big_endian
    } format = unknown;

    if (!input || input.bad()) {
        return false;
    }

    std::streambuf* buf = input.rdbuf();
    if (!buf) {
        return false;
    }

    // read in the first three characters
    char ply[3];
    input.read(ply, 3);
    input.ignore(1);
    if (!input) {
        return false;
    }
    if ((ply[0] != 'p') || (ply[1] != 'l') || (ply[2] != 'y')) {
        return false;  // wrong header
    }

    std::vector<std::pair<std::string, Ply::Number>> vertex_props;
    std::vector<Ply::Number> face_props;
    std::string line, element;

    MeshIO::Binding rgb_value = MeshIO::OVERALL;
    while (std::getline(input, line)) {
        std::istringstream str(line);
        str.unsetf(std::ios_base::skipws);
        str >> std::ws;
        if (str.eof()) {
            continue;  // empty line
        }
        std::string kw;
        str >> kw;
        if (kw == "format") {
            std::string format_string, version;
            char space_format_string {}, space_format_version {};
            str >> space_format_string >> std::ws >> format_string >> space_format_version
                >> std::ws >> version;
            if (/*!str || !str.eof() ||*/
                !std::isspace(space_format_string) || !std::isspace(space_format_version)) {
                return false;
            }
            if (format_string == "ascii") {
                format = ascii;
            }
            else if (format_string == "binary_big_endian") {
                format = binary_big_endian;
            }
            else if (format_string == "binary_little_endian") {
                format = binary_little_endian;
            }
            else {
                // wrong format version
                return false;
            }
            if (version != "1.0") {
                // wrong version
                return false;
            }
        }
        else if (kw == "element") {
            std::string name;
            std::size_t count {};
            char space_element_name {}, space_name_count {};
            str >> space_element_name >> std::ws >> name >> space_name_count >> std::ws >> count;
            if (/*!str || !str.eof() ||*/
                !std::isspace(space_element_name) || !std::isspace(space_name_count)) {
                return false;
            }
            if (name == "vertex") {
                element = name;
                v_count = count;
                meshPoints.reserve(count);
            }
            else if (name == "face") {
                element = name;
                f_count = count;
                meshFacets.reserve(count);
            }
            else {
                element.clear();
            }
        }
        else if (kw == "property") {
            std::string type, name;
            char space {};
            if (element == "vertex") {
                str >> space >> std::ws >> type >> space >> std::ws >> name >> std::ws;

                Ply::Number number {};
                if (type == "char" || type == "int8") {
                    number = int8;
                }
                else if (type == "uchar" || type == "uint8") {
                    number = uint8;
                }
                else if (type == "short" || type == "int16") {
                    number = int16;
                }
                else if (type == "ushort" || type == "uint16") {
                    number = uint16;
                }
                else if (type == "int" || type == "int32") {
                    number = int32;
                }
                else if (type == "uint" || type == "uint32") {
                    number = uint32;
                }
                else if (type == "float" || type == "float32") {
                    number = float32;
                }
                else if (type == "double" || type == "float64") {
                    number = float64;
                }
                else {
                    // no valid number type
                    return false;
                }

                // store the property name and type
                vertex_props.emplace_back(name, number);
            }
            else if (element == "face") {
                std::string list, uchr;
                str >> space >> std::ws >> list >> std::ws;
                if (list == "list") {
                    str >> uchr >> std::ws >> type >> std::ws >> name >> std::ws;
                }
                else {
                    // not a 'list'
                    type = list;
                    str >> name;
                }
                if (name != "vertex_indices" && name != "vertex_index") {
                    Number number {};
                    if (type == "char" || type == "int8") {
                        number = int8;
                    }
                    else if (type == "uchar" || type == "uint8") {
                        number = uint8;
                    }
                    else if (type == "short" || type == "int16") {
                        number = int16;
                    }
                    else if (type == "ushort" || type == "uint16") {
                        number = uint16;
                    }
                    else if (type == "int" || type == "int32") {
                        number = int32;
                    }
                    else if (type == "uint" || type == "uint32") {
                        number = uint32;
                    }
                    else if (type == "float" || type == "float32") {
                        number = float32;
                    }
                    else if (type == "double" || type == "float64") {
                        number = float64;
                    }
                    else {
                        // no valid number type
                        return false;
                    }

                    // store the property name and type
                    face_props.push_back(number);
                }
            }
        }
        else if (kw == "end_header") {
            break;  // end of the header, now read the data
        }
    }

    // check if valid 3d points
    Property property;
    std::size_t num_x = std::count_if(vertex_props.begin(),
                                      vertex_props.end(),
                                      [&property](const std::pair<std::string, int>& p) {
                                          return property(p, "x");
                                      });
    if (num_x != 1) {
        return false;
    }

    std::size_t num_y = std::count_if(vertex_props.begin(),
                                      vertex_props.end(),
                                      [&property](const std::pair<std::string, int>& p) {
                                          return property(p, "y");
                                      });
    if (num_y != 1) {
        return false;
    }

    std::size_t num_z = std::count_if(vertex_props.begin(),
                                      vertex_props.end(),
                                      [&property](const std::pair<std::string, int>& p) {
                                          return property(p, "z");
                                      });
    if (num_z != 1) {
        return false;
    }

    for (auto& it : vertex_props) {
        if (it.first == "diffuse_red") {
            it.first = "red";
        }
        else if (it.first == "diffuse_green") {
            it.first = "green";
        }
        else if (it.first == "diffuse_blue") {
            it.first = "blue";
        }
    }

    // check if valid colors are set
    std::size_t num_r = std::count_if(vertex_props.begin(),
                                      vertex_props.end(),
                                      [&property](const std::pair<std::string, int>& p) {
                                          return property(p, "red");
                                      });
    std::size_t num_g = std::count_if(vertex_props.begin(),
                                      vertex_props.end(),
                                      [&property](const std::pair<std::string, int>& p) {
                                          return property(p, "green");
                                      });
    std::size_t num_b = std::count_if(vertex_props.begin(),
                                      vertex_props.end(),
                                      [&property](const std::pair<std::string, int>& p) {
                                          return property(p, "blue");
                                      });
    std::size_t rgb_colors = num_r + num_g + num_b;
    if (rgb_colors != 0 && rgb_colors != 3) {
        return false;
    }

    // only if set per vertex
    if (rgb_colors == 3) {
        rgb_value = MeshIO::PER_VERTEX;
        if (_material) {
            _material->binding = MeshIO::PER_VERTEX;
            _material->diffuseColor.reserve(v_count);
        }
    }

    if (format == ascii) {
        boost::regex rx_d("(([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?))\\s*");
        boost::regex rx_s("\\b([-+]?[0-9]+)\\s*");
        boost::regex rx_u("\\b([0-9]+)\\s*");
        boost::regex rx_f(R"(^\s*3\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s*)");
        boost::smatch what;

        for (std::size_t i = 0; i < v_count && std::getline(input, line); i++) {
            // go through the vertex properties
            std::map<std::string, float> prop_values;
            for (const auto& it : vertex_props) {
                switch (it.second) {
                    case int8:
                    case int16:
                    case int32: {
                        if (boost::regex_search(line, what, rx_s)) {
                            int v {};
                            v = boost::lexical_cast<int>(what[1]);
                            prop_values[it.first] = static_cast<float>(v);
                            line = line.substr(what[0].length());
                        }
                        else {
                            return false;
                        }
                    } break;
                    case uint8:
                    case uint16:
                    case uint32: {
                        if (boost::regex_search(line, what, rx_u)) {
                            int v {};
                            v = boost::lexical_cast<int>(what[1]);
                            prop_values[it.first] = static_cast<float>(v);
                            line = line.substr(what[0].length());
                        }
                        else {
                            return false;
                        }
                    } break;
                    case float32:
                    case float64: {
                        if (boost::regex_search(line, what, rx_d)) {
                            double v {};
                            v = boost::lexical_cast<double>(what[1]);
                            prop_values[it.first] = static_cast<float>(v);
                            line = line.substr(what[0].length());
                        }
                        else {
                            return false;
                        }
                    } break;
                    default:
                        return false;
                }
            }

            Base::Vector3f pt;
            pt.x = (prop_values["x"]);
            pt.y = (prop_values["y"]);
            pt.z = (prop_values["z"]);
            meshPoints.push_back(pt);

            if (_material && (rgb_value == MeshIO::PER_VERTEX)) {
                float r = (prop_values["red"]) / 255.0F;
                float g = (prop_values["green"]) / 255.0F;
                float b = (prop_values["blue"]) / 255.0F;
                _material->diffuseColor.emplace_back(r, g, b);
            }
        }

        int f1 {}, f2 {}, f3 {};
        for (std::size_t i = 0; i < f_count && std::getline(input, line); i++) {
            if (boost::regex_search(line, what, rx_f)) {
                f1 = boost::lexical_cast<int>(what[1]);
                f2 = boost::lexical_cast<int>(what[2]);
                f3 = boost::lexical_cast<int>(what[3]);
                meshFacets.push_back(MeshFacet(f1, f2, f3));
            }
        }
    }
    // binary
    else {
        Base::InputStream is(input);
        if (format == binary_little_endian) {
            is.setByteOrder(Base::Stream::LittleEndian);
        }
        else {
            is.setByteOrder(Base::Stream::BigEndian);
        }

        for (std::size_t i = 0; i < v_count; i++) {
            // go through the vertex properties
            std::map<std::string, float> prop_values;
            for (const auto& it : vertex_props) {
                switch (it.second) {
                    case int8: {
                        int8_t v {};
                        is >> v;
                        prop_values[it.first] = static_cast<float>(v);
                    } break;
                    case uint8: {
                        uint8_t v {};
                        is >> v;
                        prop_values[it.first] = static_cast<float>(v);
                    } break;
                    case int16: {
                        int16_t v {};
                        is >> v;
                        prop_values[it.first] = static_cast<float>(v);
                    } break;
                    case uint16: {
                        uint16_t v {};
                        is >> v;
                        prop_values[it.first] = static_cast<float>(v);
                    } break;
                    case int32: {
                        int32_t v {};
                        is >> v;
                        prop_values[it.first] = static_cast<float>(v);
                    } break;
                    case uint32: {
                        uint32_t v {};
                        is >> v;
                        prop_values[it.first] = static_cast<float>(v);
                    } break;
                    case float32: {
                        float v {};
                        is >> v;
                        prop_values[it.first] = v;
                    } break;
                    case float64: {
                        double v {};
                        is >> v;
                        prop_values[it.first] = static_cast<float>(v);
                    } break;
                    default:
                        return false;
                }
            }

            Base::Vector3f pt;
            pt.x = (prop_values["x"]);
            pt.y = (prop_values["y"]);
            pt.z = (prop_values["z"]);
            meshPoints.push_back(pt);

            if (_material && (rgb_value == MeshIO::PER_VERTEX)) {
                float r = (prop_values["red"]) / 255.0F;
                float g = (prop_values["green"]) / 255.0F;
                float b = (prop_values["blue"]) / 255.0F;
                _material->diffuseColor.emplace_back(r, g, b);
            }
        }

        unsigned char n {};
        uint32_t f1 {}, f2 {}, f3 {};
        for (std::size_t i = 0; i < f_count; i++) {
            is >> n;
            if (n == 3) {
                is >> f1 >> f2 >> f3;
                if (f1 < v_count && f2 < v_count && f3 < v_count) {
                    meshFacets.push_back(MeshFacet(f1, f2, f3));
                }
                for (auto it : face_props) {
                    switch (it) {
                        case int8: {
                            int8_t v {};
                            is >> v;
                        } break;
                        case uint8: {
                            uint8_t v {};
                            is >> v;
                        } break;
                        case int16: {
                            int16_t v {};
                            is >> v;
                        } break;
                        case uint16: {
                            uint16_t v {};
                            is >> v;
                        } break;
                        case int32: {
                            int32_t v {};
                            is >> v;
                        } break;
                        case uint32: {
                            uint32_t v {};
                            is >> v;
                        } break;
                        case float32: {
                            is >> n;
                            float v {};
                            for (unsigned char j = 0; j < n; j++) {
                                is >> v;
                            }
                        } break;
                        case float64: {
                            is >> n;
                            double v {};
                            for (unsigned char j = 0; j < n; j++) {
                                is >> v;
                            }
                        } break;
                        default:
                            return false;
                    }
                }
            }
        }
    }

    _kernel.Clear();  // remove all data before

    MeshCleanup meshCleanup(meshPoints, meshFacets);
    if (_material) {
        meshCleanup.SetMaterial(_material);
    }
    meshCleanup.RemoveInvalids();
    MeshPointFacetAdjacency meshAdj(meshPoints.size(), meshFacets);
    meshAdj.SetFacetNeighbourhood();
    _kernel.Adopt(meshPoints, meshFacets);

    return true;
}
