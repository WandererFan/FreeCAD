/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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
# include <QPainterPath>
# include <QPainterPathStroker>
#include <QKeyEvent>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Gui/Control.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawViewPart.h>

#include "QGIEdge.h"
#include "PreferencesGui.h"
#include "TaskLineDecor.h"
#include "QGIView.h"

using namespace TechDrawGui;
using namespace TechDraw;

QGIEdge::QGIEdge(int index) :
    projIndex(index),
    isCosmetic(false),
    isHiddenEdge(false),
    isSmoothEdge(false)
{
    setFlag(QGraphicsItem::ItemIsFocusable, true);      // to get key press events
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    // installSceneEventFilter(this);

    m_width = 1.0;
    setCosmetic(isCosmetic);
    setFill(Qt::NoBrush);
}


bool QGIEdge::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        Base::Console().Message("QGIE::sceneEventFilter - key press\n");
        auto keyEvent = dynamic_cast<QKeyEvent*>(event);
        if (!keyEvent) {
            return QGraphicsItem::sceneEventFilter(watched, event);
        }
        int ourKey = keyEvent->key();
        // Qt::Key_Delete
        Base::Console().Message("QGIE::sceneEventFilter - key is %d\n", ourKey);
    }
    return QGraphicsItem::sceneEventFilter(watched, event);
}


bool QGIEdge::sceneEvent(QEvent *event)
{
//    Base::Console().Message("QGIE::sceneEvent() - type: %d\n", event->type());
    if (event->type() == QEvent::KeyPress) {
        Base::Console().Message("QGIE::sceneEvent - key press\n");
        auto keyEvent = dynamic_cast<QKeyEvent*>(event);
        if (!keyEvent) {
            QGraphicsItem::sceneEvent(event);
        }
        int ourKey = keyEvent->key();
        // Qt::Key_Delete
        Base::Console().Message("QGIE::sceneEvent - key is %d\n", ourKey);
    }
    return QGraphicsItem::sceneEvent(event);
}

void QGIEdge::keyPressEvent(QKeyEvent *event)
{
    Base::Console().Message("QGIE::keyPressEvent()\n");
    int ourKey = event->key();
    // Qt::Key_Delete
    Base::Console().Message("QGIE::keyPressEvent - key is %d\n", ourKey);
    QGraphicsItem::keyPressEvent(event);
}


// NOTE this refers to Qt cosmetic lines (a line with minimum width),
// not FreeCAD cosmetic lines
void QGIEdge::setCosmetic(bool state)
{
//    Base::Console().Message("QGIE::setCosmetic(%d)\n", state);
    isCosmetic = state;
    if (state) {
        setWidth(0.0);
    }
}

void QGIEdge::setHiddenEdge(bool b) {
    isHiddenEdge = b;
    if (b) {
        m_styleCurrent = getHiddenStyle();
    } else {
        m_styleCurrent = Qt::SolidLine;
    }
}

void QGIEdge::setPrettyNormal() {
//    Base::Console().Message("QGIE::setPrettyNormal()\n");
    if (isHiddenEdge) {
        m_colCurrent = getHiddenColor();
    } else {
        m_colCurrent = getNormalColor();
    }
    //should call QGIPP::setPrettyNormal()?
}

QColor QGIEdge::getHiddenColor()
{
    App::Color fcColor = App::Color((uint32_t) Preferences::getPreferenceGroup("Colors")->GetUnsigned("HiddenColor", 0x000000FF));
    return PreferencesGui::getAccessibleQColor(fcColor.asValue<QColor>());
}

Qt::PenStyle QGIEdge::getHiddenStyle()
{
    //Qt::PenStyle - NoPen, Solid, Dashed, ...
    //Preferences::General - Solid, Dashed
    // Dashed lines should use ISO Line #2 instead of Qt::DashedLine
    Qt::PenStyle hidStyle = static_cast<Qt::PenStyle> (Preferences::getPreferenceGroup("General")->GetInt("HiddenLine", 0) + 1);
    return hidStyle;
}

 double QGIEdge::getEdgeFuzz() const
{
    return PreferencesGui::edgeFuzz();
}


QRectF QGIEdge::boundingRect() const
{
    return shape().controlPointRect();
}

QPainterPath QGIEdge::shape() const
{
    QPainterPath outline;
    QPainterPathStroker stroker;
    stroker.setWidth(getEdgeFuzz());
    outline = stroker.createStroke(path());
    return outline;
}

void QGIEdge::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event)
    QGIView *parent = dynamic_cast<QGIView *>(parentItem());
    if (parent && parent->getViewObject() && parent->getViewObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
        TechDraw::DrawViewPart *baseFeat = static_cast<TechDraw::DrawViewPart *>(parent->getViewObject());
        std::vector<std::string> edgeName(1, DrawUtil::makeGeomName("Edge", getProjIndex()));

        Gui::Control().showDialog(new TaskDlgLineDecor(baseFeat, edgeName));
    }
}



void QGIEdge::setLinePen(QPen linePen)
{
    m_pen = linePen;
}
