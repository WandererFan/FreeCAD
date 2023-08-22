/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2014 Luke Parry <l.parry@warwick.ac.uk>                 *
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
# include <sstream>
# include <QFile>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>     //TODO: should be in DrawTemplate.h??
#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <Base/Quantity.h>
#include <Base/Tools.h>

#include "DrawPage.h"
#include "DrawSVGTemplate.h"
#include "DrawSVGTemplatePy.h"
#include "DrawUtil.h"
#include "XMLQuery.h"
#include "Preferences.h"

using namespace TechDraw;

// TODO: we aren't using defines for constants any more.
#define NOTFOUNDTOKEN "NoSuchValue"

PROPERTY_SOURCE(TechDraw::DrawSVGTemplate, TechDraw::DrawTemplate)

DrawSVGTemplate::DrawSVGTemplate()
{
    static const char *group = "Template";

    ADD_PROPERTY_TYPE(PageResult, (nullptr),  group, App::Prop_Output,    "Embedded SVG code for template. For system use.");   //n/a for users
    ADD_PROPERTY_TYPE(Template,   (""), group, App::Prop_None, "Template file name.");

    // Width and Height properties shouldn't be set by the user
    Height.setStatus(App::Property::ReadOnly, true);
    Width.setStatus(App::Property::ReadOnly, true);
    Orientation.setStatus(App::Property::ReadOnly, true);

    std::string svgFilter("Svg files (*.svg *.SVG);;All files (*)");
    Template.setFilter(svgFilter);
}

DrawSVGTemplate::~DrawSVGTemplate()
{
}

PyObject *DrawSVGTemplate::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawSVGTemplatePy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

void DrawSVGTemplate::onChanged(const App::Property* prop)
{
    if (prop == &Template && !isRestoring()) {
        //if we are restoring an existing file we just want the properties set as they were save,
        //but if we are not restoring, we need to replace the embedded file and extract the new
        //EditableTexts.
        // an argument could be made for preserving previous values when
        // changing template. Would need to grab current EditableTexts, then
        // replace matching keys after setDefaultTextValues()
        replaceFileIncluded(Template.getValue());
        // set EditableTexts to the placeholders in the Svg source
        EditableTexts.setValues(getEditableTextsFromTemplate());
        // replace matching field names with preference values
        setDefaultTextValues();
        QDomDocument templateDocument;
        if (getTemplateDocument(Template.getValue(), templateDocument)) {
            extractTemplateAttributes(templateDocument);
        }
    } else if (prop == &EditableTexts) {
        //handled by ViewProvider
    }

    TechDraw::DrawTemplate::onChanged(prop);
}

//parse the Svg code, inserting current EditableTexts values, and return the result as a QString.
//While parsing, note the Orientation, Width and Height values in the Svg code.
QString DrawSVGTemplate::processTemplate()
{
//    Base::Console().Message("DSVGT::processTemplate() - isRestoring: %d\n", isRestoring());
    if (isRestoring()) {
        //until everything is fully restored, the embedded file is not available, so we
        //can't do anything
        return QString();
    }
    QDomDocument templateDocument;
    if (!getTemplateDocument(PageResult.getValue(), templateDocument)) {
        return QString();
    }

    XMLQuery query(templateDocument);
    std::map<std::string, std::string> substitutions = EditableTexts.getValues();

    // XPath query to select all <tspan> nodes whose <text> parent
    // has "freecad:editable" attribute
    query.processItems(QString::fromUtf8(
        "declare default element namespace \"" SVG_NS_URI "\"; "
        "declare namespace freecad=\"" FREECAD_SVG_NS_URI "\"; "
        "//text[@freecad:editable]/tspan"),
        [&substitutions, &templateDocument](QDomElement& tspan) -> bool {
        // Replace the editable text spans with new nodes holding actual values
        QString editableName = tspan.parentNode().toElement().attribute(QString::fromUtf8("freecad:editable"));
        std::map<std::string, std::string>::iterator item =
            substitutions.find(editableName.toStdString());
        if (item != substitutions.end()) {
            // Keep all spaces in the text node
            tspan.setAttribute(QString::fromUtf8("xml:space"), QString::fromUtf8("preserve"));

            // Remove all child nodes and append text node with editable replacement as the only descendant
            while (!tspan.lastChild().isNull()) {
                tspan.removeChild(tspan.lastChild());
            }
            tspan.appendChild(templateDocument.createTextNode(QString::fromUtf8(item->second.c_str())));
        }
        return true;
    });

    extractTemplateAttributes(templateDocument);

    //all Qt holds on files should be released on exit #4085
    return templateDocument.toString();
}

// find the width, height and orientation of the template and update the properties
void DrawSVGTemplate::extractTemplateAttributes(QDomDocument& templateDocument)
{
    QDomElement docElement = templateDocument.documentElement();
    Base::Quantity quantity;

    // Obtain the width
    QString str = docElement.attribute(QString::fromLatin1("width"));
    quantity = Base::Quantity::parse(str);
    quantity.setUnit(Base::Unit::Length);

    Width.setValue(quantity.getValue());

    str = docElement.attribute(QString::fromLatin1("height"));
    quantity = Base::Quantity::parse(str);
    quantity.setUnit(Base::Unit::Length);

    Height.setValue(quantity.getValue());

    bool isLandscape = getWidth() / getHeight() >= 1.;

    Orientation.setValue(isLandscape ? 1 : 0);
}

// load the included template file as a QDomDocument
bool DrawSVGTemplate::getTemplateDocument(std::string sourceFile, QDomDocument& templateDocument) const
{
    if (sourceFile.empty()) {
        return false;
    }
    QFile templateFile(Base::Tools::fromStdString(sourceFile));
    if (!templateFile.open(QIODevice::ReadOnly)) {
        Base::Console().Error("DrawSVGTemplate::processTemplate can't read embedded template %s!\n", PageResult.getValue());
        return false;
    }

    if (!templateDocument.setContent(&templateFile)) {
        Base::Console().Error("DrawSVGTemplate::processTemplate - failed to parse file: %s\n",
            PageResult.getValue());
        return false;
    }
    // no errors templateDocument is loaded
    return true;
}

double DrawSVGTemplate::getWidth() const
{
    return Width.getValue();
}

double DrawSVGTemplate::getHeight() const
{
    return Height.getValue();
}

void DrawSVGTemplate::replaceFileIncluded(std::string newTemplateFileName)
{
//    Base::Console().Message("DSVGT::replaceFileIncluded(%s)\n", newTemplateFileName.c_str());
    if (newTemplateFileName.empty()) {
        return;
    }

    Base::FileInfo tfi(newTemplateFileName);
    if (tfi.isReadable()) {
        PageResult.setValue(newTemplateFileName.c_str());
    } else {
        throw Base::RuntimeError("Could not read the new template file");
    }
}

std::map<std::string, std::string> DrawSVGTemplate::getEditableTextsFromTemplate()
{
//    Base::Console().Message("DSVGT::getEditableTextsFromTemplate()\n");
    std::map<std::string, std::string> editables;

// if we pass the filename we can reuse getTemplateDocument here
    QDomDocument templateDocument;
    if (!getTemplateDocument(Template.getValue(), templateDocument)) {
        return editables;
    }

    XMLQuery query(templateDocument);

    // XPath query to select all <tspan> nodes whose <text> parent
    // has "freecad:editable" attribute
    query.processItems(QString::fromUtf8(
        "declare default element namespace \"" SVG_NS_URI "\"; "
        "declare namespace freecad=\"" FREECAD_SVG_NS_URI "\"; "
        "//text[@freecad:editable]/tspan"),
        [&editables](QDomElement& tspan) -> bool {
        QString editableName = tspan.parentNode().toElement().attribute(QString::fromUtf8("freecad:editable"));
        QString editableValue = tspan.firstChild().nodeValue();

        editables[std::string(editableName.toUtf8().constData())] =
            std::string(editableValue.toUtf8().constData());
        return true;
    });

    return editables;
}

/// update EditableTexts with any default values that match template field names
void DrawSVGTemplate::setDefaultTextValues()
{
    std::map<std::string, std::string> textsMap = EditableTexts.getValues();
    for (auto entry : textsMap) {
        auto newValue = getDefaultValueForKey(entry.first);
        if (newValue != NOTFOUNDTOKEN) {
            // replace the field value if a default is available
            textsMap[entry.first] = newValue;
        }
    }
    EditableTexts.setValues(textsMap);
}

/// simple look up in user.cfg
std::string DrawSVGTemplate::getDefaultValueForKey(std::string key)
{
    // this routine would be different in a json version where it would
    // retrieve and unpack the packed key-value pairs (if not available yet)
    // then look up the key

    return Preferences::getPreferenceGroup("Template")->GetASCII(key.c_str(), NOTFOUNDTOKEN);
}

//! get a translated label string from the context (ex TaskActiveView), the base name (ex ActiveView) and
//! the unique name within the document (ex ActiveView001), and use it to update the Label property.
void DrawSVGTemplate::translateLabel(std::string context, std::string baseName, std::string uniqueName)
{
    Label.setValue(DrawUtil::translateArbitrary(context, baseName, uniqueName));
}

// Python Template feature ---------------------------------------------------------
namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawSVGTemplatePython, TechDraw::DrawSVGTemplate)
template<> const char* TechDraw::DrawSVGTemplatePython::getViewProviderName() const {
    return "TechDrawGui::ViewProviderPython";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawSVGTemplate>;
}
