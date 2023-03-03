/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
 *   Copyright (c) 2019 Franck Jullien <franck.jullien@gmail.com>          *
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
# include <cmath>
# include <string>

# include <QGraphicsScene>
# include <QGraphicsSceneMouseEvent>
# include <QPaintDevice>
# include <QPainter>
# include <QPainterPath>
# include <QSvgGenerator>
#endif

#include <App/Application.h>
#include <Base/Parameter.h>
#include <Gui/Command.h>
#include <Gui/Tools.h>
#include <Mod/TechDraw/App/ArrowPropEnum.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawViewBalloon.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/Geometry.h>

#include "QGIViewBalloon.h"
#include "PreferencesGui.h"
#include "QGIArrow.h"
#include "QGIDimLines.h"
#include "Rez.h"
#include "ViewProviderBalloon.h"
#include "ViewProviderViewPart.h"
#include "ZVALUE.h"


//TODO: hide the Qt coord system (+y down).

using namespace TechDraw;
using namespace TechDrawGui;
using DU = DrawUtil;

#define NODRAG 0
#define DRAGSTARTED 1
#define DRAGGING 2

QGIBalloonLabel::QGIBalloonLabel()
{
    posX = 0;
    posY = 0;
    m_ctrl = false;
    m_drag = false;

    setCacheMode(QGraphicsItem::NoCache);
    setFlag(ItemSendsGeometryChanges, true);
    setFlag(ItemIsMovable, true);
    setFlag(ItemIsSelectable, true);
    setAcceptHoverEvents(true);

    m_labelText = new QGCustomText();
    m_labelText->setParentItem(this);

    verticalSep = false;
    hasHover = false;
    parent = nullptr;
}

QVariant QGIBalloonLabel::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemSelectedHasChanged && scene()) {
        if (isSelected()) {
            Q_EMIT selected(true);
            setPrettySel();
        }
        else {
            setPrettyNormal();
            Q_EMIT selected(false);
            if (m_drag) {
                //stop the drag if we are no longer selected.
                m_drag = false;
                Q_EMIT dragFinished();
            }
            update();
        }
    } else if (change == ItemPositionHasChanged && scene()) {
        setLabelCenter();
        if (m_drag) {
            Q_EMIT dragging(m_ctrl);
        }
    }

    return QGraphicsItem::itemChange(change, value);
}

void QGIBalloonLabel::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    Base::Console().Message("QGIBL::mousePressEvent()\n");
    m_ctrl = false;
    m_drag = true;
    if (event->modifiers() & Qt::ControlModifier) {
        m_ctrl = true;
    }
    Q_EMIT dragStarted(m_ctrl);
    QGraphicsItem::mousePressEvent(event);
}

void QGIBalloonLabel::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    Base::Console().Message("QGIBL::mouseReleaseEvent()\n");
    m_ctrl = false;
    if (m_drag) {
        m_drag = false;
        Q_EMIT dragFinished();
    }
    QGraphicsItem::mouseReleaseEvent(event);
}

void QGIBalloonLabel::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    QGIViewBalloon* qgivBalloon = dynamic_cast<QGIViewBalloon*>(parentItem());
    if (!qgivBalloon) {
        qWarning() << "QGIBalloonLabel::mouseDoubleClickEvent: No parent item";
        return;
    }

    auto ViewProvider = dynamic_cast<ViewProviderBalloon*>(
        qgivBalloon->getViewProvider(qgivBalloon->getViewObject()));
    if (!ViewProvider) {
        qWarning() << "QGIBalloonLabel::mouseDoubleClickEvent: No valid view provider";
        return;
    }

    ViewProvider->startDefaultEditMode();
    QGraphicsItem::mouseDoubleClickEvent(event);
}

void QGIBalloonLabel::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    Q_EMIT hover(true);
    hasHover = true;
    if (!isSelected()) {
        setPrettyPre();
    }
    else {
        setPrettySel();
    }
    QGraphicsItem::hoverEnterEvent(event);
}

void QGIBalloonLabel::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    QGIView* view = dynamic_cast<QGIView*>(parentItem());
    assert(view);
    Q_UNUSED(view);

    Q_EMIT hover(false);
    hasHover = false;
    if (!isSelected()) {
        setPrettyNormal();
    }
    else {
        setPrettySel();
    }
    QGraphicsItem::hoverLeaveEvent(event);
}

QRectF QGIBalloonLabel::boundingRect() const { return childrenBoundingRect(); }

void QGIBalloonLabel::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                            QWidget* widget)
{
    Q_UNUSED(widget);
    Q_UNUSED(painter);
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    //QGraphicsObject/QGraphicsItem::paint gives link error.
}

void QGIBalloonLabel::setPosFromCenter(const double& xCenter, const double& yCenter)
{
    Base::Console().Message("QGIBL::setPosFromCenter(%.3f, %.3f)\n", xCenter, yCenter);
    //set label's Qt position(top, left) given boundingRect center point
    Base::Console().Message("QGIBL::setPositionFromCenter - w: %.3f h: %.3f\n",
                            m_labelText->boundingRect().width(), m_labelText->boundingRect().height());
    setPos(xCenter - m_labelText->boundingRect().width() / 2.,
           yCenter - m_labelText->boundingRect().height() / 2.);
}

void QGIBalloonLabel::setLabelCenter()
{
    //save label's bRect center (posX, posY) given Qt position (top, left)
    posX = x() + m_labelText->boundingRect().width() / 2.;
    posY = y() + m_labelText->boundingRect().height() / 2.;
}

QPointF QGIBalloonLabel::getLabelCenter()
{
    double cx = x() + m_labelText->boundingRect().width() / 2.;
    double cy = y() + m_labelText->boundingRect().height() / 2.;
    return { cx, cy };
}

void QGIBalloonLabel::setFont(QFont font) { m_labelText->setFont(font); }

void QGIBalloonLabel::setDimString(QString text)
{
    prepareGeometryChange();
    m_labelText->setPlainText(text);
}

void QGIBalloonLabel::setDimString(QString text, qreal maxWidth)
{
    prepareGeometryChange();
    m_labelText->setPlainText(text);
    m_labelText->setTextWidth(maxWidth);
}

void QGIBalloonLabel::setPrettySel() { m_labelText->setPrettySel(); }

void QGIBalloonLabel::setPrettyPre() { m_labelText->setPrettyPre(); }

void QGIBalloonLabel::setPrettyNormal() { m_labelText->setPrettyNormal(); }

void QGIBalloonLabel::setColor(QColor color)
{
    m_colNormal = color;
    m_labelText->setColor(m_colNormal);
}

//**************************************************************
QGIViewBalloon::QGIViewBalloon()
    : dvBalloon(nullptr), hasHover(false), m_lineWidth(0.0), m_obtuse(false), parent(nullptr),
    m_originDragged(false), m_dragState(NODRAG)
{
    m_ctrl = false;

    setHandlesChildEvents(false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setCacheMode(QGraphicsItem::NoCache);

    balloonLabel = new QGIBalloonLabel();
    balloonLabel->setQBalloon(this);

    addToGroup(balloonLabel);
    balloonLabel->setColor(prefNormalColor());
    balloonLabel->setPrettyNormal();

    balloonLines = new QGIDimLines();
    addToGroup(balloonLines);
    balloonLines->setNormalColor(prefNormalColor());
    balloonLines->setPrettyNormal();

    balloonShape = new QGIDimLines();
    addToGroup(balloonShape);
    balloonShape->setNormalColor(prefNormalColor());
    balloonShape->setFill(Qt::transparent, Qt::SolidPattern);
    balloonShape->setFillOverride(true);
    balloonShape->setPrettyNormal();

    arrow = new QGIArrow();
    addToGroup(arrow);
    arrow->setNormalColor(prefNormalColor());
    arrow->setFillColor(prefNormalColor());
    arrow->setPrettyNormal();
    arrow->setStyle(prefDefaultArrow());

    balloonLabel->setZValue(ZVALUE::LABEL);
    arrow->setZValue(ZVALUE::DIMENSION);

    balloonLines->setZValue(ZVALUE::DIMENSION);
    balloonLines->setStyle(Qt::SolidLine);

    balloonShape->setZValue(ZVALUE::DIMENSION + 1);//above balloonLines!
    balloonShape->setStyle(Qt::SolidLine);

    // connecting the needed slots and signals
    QObject::connect(balloonLabel, &QGIBalloonLabel::dragStarted, this, &QGIViewBalloon::balloonLabelDragStarted);
    QObject::connect(balloonLabel, &QGIBalloonLabel::dragging, this, &QGIViewBalloon::balloonLabelDragged);

    QObject::connect(balloonLabel, &QGIBalloonLabel::dragFinished, this, &QGIViewBalloon::balloonLabelDragFinished);

    QObject::connect(balloonLabel, &QGIBalloonLabel::selected, this, &QGIViewBalloon::select);

    QObject::connect(balloonLabel, &QGIBalloonLabel::hover, this, &QGIViewBalloon::hover);

    setZValue(ZVALUE::DIMENSION);
}

QVariant QGIViewBalloon::itemChange(GraphicsItemChange change, const QVariant& value)
{
    Base::Console().Message("QGIVB::itemChange()\n");
    if (change == ItemSelectedHasChanged && scene()) {
        if (isSelected()) {
            balloonLabel->setSelected(true);
        }
        else {
            balloonLabel->setSelected(false);
        }
    }
    return QGraphicsItemGroup::itemChange(change, value);
}

//Set selection state for this and it's children
void QGIViewBalloon::setGroupSelection(bool isSelected)
{
    //    Base::Console().Message("QGIVB::setGroupSelection(%d)\n", b);
    setSelected(isSelected);
    balloonLabel->setSelected(isSelected);
    balloonLines->setSelected(isSelected);
    arrow->setSelected(isSelected);
}

void QGIViewBalloon::select(bool state)
{
    Base::Console().Message("QGIVBall::select(%d)\n", state);
    setSelected(state);
    draw();
}

void QGIViewBalloon::hover(bool state)
{
    hasHover = state;
    draw();
}

void QGIViewBalloon::setViewPartFeature(TechDraw::DrawViewBalloon* balloonFeat)
{
    Base::Console().Message("QGIVB::setViewPartFeature()\n");
    if (!balloonFeat) {
        Base::Console().Message("QGIVB::setViewPartFeature - feature is null\n");
        return;
    }

    setViewFeature(static_cast<TechDraw::DrawView*>(balloonFeat));

    balloonLabel->setColor(prefNormalColor());

    QString labelText = QString::fromUtf8(balloonFeat->Text.getStrValue().data());
    balloonLabel->setDimString(labelText, Rez::guiX(balloonFeat->TextWrapLen.getValue()));

    positionLabel();

    updateView();
}

void QGIViewBalloon::positionViewFromFeature()
{
    Base::Console().Message("QGIVB::positionViewFromFeature()\n");
    auto balloonFeat(dynamic_cast<TechDraw::DrawViewBalloon*>(getViewObject()));
    if (!balloonFeat) {
        Base::Console().Message("QGIVB::positionViewFromFeature - no feature yet\n");
        return;
    }

    DrawView* balloonParent = nullptr;
    double scale = 1.0;
    App::DocumentObject* docObj = balloonFeat->SourceView.getValue();
    if (docObj) {
        balloonParent = dynamic_cast<DrawView*>(docObj);
        if (balloonParent) {
            scale = balloonParent->getScale();
        }
    }

    float x = Rez::guiX(balloonFeat->X.getValue() * scale);
    float y = Rez::guiX(-balloonFeat->Y.getValue() * scale);
    balloonLabel->setPosFromCenter(x, y);
    // ??? why do we not set the arrow tip's position here?
}

void QGIViewBalloon::positionLabel()
{
    Base::Console().Message("QGIVB::positionLabel()\n");
    auto balloonFeat(dynamic_cast<TechDraw::DrawViewBalloon*>(getViewObject()));
    if (!balloonFeat) {
        Base::Console().Message("QGIVB::positionLabel - no feature yet\n");
        return;
    }

    auto vp = static_cast<ViewProviderBalloon*>(getViewProvider(getViewObject()));
    if (!vp) {
        Base::Console().Message("QGIVB::positionLabel - no vp yet\n");
        return;
    }

    double scale = 1.0;
    DrawView* balloonParent = getSourceView();
    if (balloonParent) {
        scale = balloonParent->getScale();
    }

    // need the font set to get the right size label.boundingRect.
    QFont font;
    font.setFamily(QString::fromUtf8(vp->Font.getValue()));
    font.setPixelSize(exactFontSize(vp->Font.getValue(), vp->Fontsize.getValue()));
    balloonLabel->setFont(font);

    float x = Rez::guiX(balloonFeat->X.getValue() * scale);
    float y = Rez::guiX(-balloonFeat->Y.getValue() * scale);

    balloonLabel->setPosFromCenter(x, y);
}

void QGIViewBalloon::setFeatureXYFromPos()
{
    Base::Console().Message("setFeatureXYFromPos()\n");
    auto dvb(dynamic_cast<TechDraw::DrawViewBalloon*>(getViewObject()));
    if (!dvb) {
        return;
    }

    double scale = 1.0;
    DrawView* balloonParent = getSourceView();
    if (balloonParent) {
        scale = balloonParent->getScale();
    }

    double x = Rez::appX(balloonLabel->X() / scale);
    double y = Rez::appX(balloonLabel->Y() / scale);

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Drag Balloon"));
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.X = %f",
                            dvb->getNameInDocument(), x);
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.Y = %f",
                            dvb->getNameInDocument(), -y);

    //update the arrow tip position if it was moved in the drag
    if (m_originDragged) {
        Base::Vector3d bubblePos(x, -y, 0.0);
        Base::Vector3d newOrigin = bubblePos + m_saveOffset;
        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.OriginX = %f",
                                dvb->getNameInDocument(), newOrigin.x);
        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.OriginY = %f",
                                dvb->getNameInDocument(), -newOrigin.y);
    }

    Gui::Command::commitCommand();
}

//get the bubble and arrow tip positions from the Balloon feature in local coordinates
std::pair <QPointF, QPointF> QGIViewBalloon::getPositionsFromFeature()
{
    Base::Console().Message("QGIVB::getPositionsFromFeature()\n");
    TechDraw::DrawViewBalloon* dvb = dynamic_cast<TechDraw::DrawViewBalloon*>(getViewObject());
    if (!dvb) {
        throw Base::RuntimeError("Balloon has no feature!");
    }

    const TechDraw::DrawView* dvParent = dvb->getParentView();
    if (!dvParent) {
        throw Base::RuntimeError("Balloon has no parent!");
    }

    double bubbleX = Rez::guiX(dvb->X.getValue() * dvParent->getScale());
    double bubbleY = -Rez::guiX(dvb->Y.getValue() * dvParent->getScale());
    double arrowTipX = Rez::guiX(dvb->OriginX.getValue() * dvParent->getScale());
    double arrowTipY = -Rez::guiX(dvb->OriginY.getValue() * dvParent->getScale());

    QPointF bubblePos(bubbleX, bubbleY);
    QPointF tipPos(arrowTipX, arrowTipY);

    return { bubblePos, tipPos };
}

//get the current bubble and arrow tip positions in the view
std::pair <QPointF, QPointF> QGIViewBalloon::getPositionsFromView()
{
    Base::Console().Message("QGIVB::getPositionsFromView()\n");
    QPointF bubblePos = balloonLabel->getLabelCenter();
    QPointF arrowTipPos = arrow->pos();
    QPointF offset = DU::toQPointF(m_saveOffset);
    if (m_originDragged) {
        arrowTipPos = bubblePos - offset;
    }
    return { bubblePos, arrowTipPos };
}

void QGIViewBalloon::updateView(bool update)
{
    Base::Console().Message("QGIVB::updateView() - drag: %d  originMoved: %d\n", m_dragState, m_originDragged);
    Q_UNUSED(update);
    auto balloon(dynamic_cast<TechDraw::DrawViewBalloon*>(getViewObject()));
    if (!balloon) {
        return;
    }

    auto vp = static_cast<ViewProviderBalloon*>(getViewProvider(getViewObject()));
    if (!vp) {
        return;
    }

    if (balloon->X.isTouched() || balloon->Y.isTouched()) {
        positionViewFromFeature();
    }

    if (balloon->isLocked()) {
        balloonLabel->setFlag(QGraphicsItem::ItemIsMovable, false);
    } else {
        balloonLabel->setFlag(QGraphicsItem::ItemIsMovable, true);
    }

    updateBalloon();
    draw();
}

//update the bubble contents
void QGIViewBalloon::updateBalloon(bool obtuse)
{
    //    Base::Console().Message("QGIVB::updateBalloon()\n");
    (void)obtuse;
    const auto balloon(dynamic_cast<TechDraw::DrawViewBalloon*>(getViewObject()));
    if (!balloon) {
        return;
    }
    auto vp = static_cast<ViewProviderBalloon*>(getViewProvider(getViewObject()));
    if (!vp) {
        return;
    }
    const TechDraw::DrawView* refObj = balloon->getParentView();
    if (!refObj) {
        return;
    }

    QFont font;
    font.setFamily(QString::fromUtf8(vp->Font.getValue()));
    font.setPixelSize(exactFontSize(vp->Font.getValue(), vp->Fontsize.getValue()));
    balloonLabel->setFont(font);

    QString labelText = QString::fromUtf8(balloon->Text.getStrValue().data());
    balloonLabel->setVerticalSep(false);
    balloonLabel->setSeps(std::vector<int>());

    if (strcmp(balloon->BubbleShape.getValueAsString(), "Rectangle") == 0) {
        std::vector<int> newSeps;
        while (labelText.contains(QString::fromUtf8("|"))) {
            int pos = labelText.indexOf(QString::fromUtf8("|"));
            labelText.replace(pos, 1, QString::fromUtf8("   "));
            QFontMetrics fm(balloonLabel->getFont());
            newSeps.push_back(Gui::QtTools::horizontalAdvance(fm, labelText.left(pos + 2)));
            balloonLabel->setVerticalSep(true);
        }
        balloonLabel->setSeps(newSeps);
    }

    balloonLabel->setDimString(labelText, Rez::guiX(balloon->TextWrapLen.getValue()));
}

void QGIViewBalloon::balloonLabelDragStarted(bool ctrl)
{
    m_dragState = DRAGSTARTED;
    m_ctrl = ctrl;
    m_originDragged = ctrl;
    if (ctrl) {
        // we are moving the arrow tip, so save the current offset
        auto balloonFeat(dynamic_cast<TechDraw::DrawViewBalloon*>(getViewObject()));
        if (!balloonFeat) {
            return;
        }
        float x = balloonFeat->X.getValue();
        float y = balloonFeat->Y.getValue();
        Base::Vector3d bubblePos(x, y, 0.0);
        float xOrg = balloonFeat->OriginX.getValue();
        float yOrg = balloonFeat->OriginY.getValue();
        Base::Vector3d originPos(xOrg, yOrg, 0.0);
        m_saveOffset = originPos - bubblePos;
    } else {
        m_saveOffset = Base::Vector3d(0.0, 0.0, 0.0);
    }
}


void QGIViewBalloon::balloonLabelDragged(bool ctrl)
{
    Q_UNUSED(ctrl)
    Base::Console().Message("QGIVB::BalloonLabelDragged()\n");
    if (m_dragState == DRAGSTARTED) {
        m_dragState = DRAGGING;
    }

    if (m_dragState == DRAGGING) {
        setFeatureXYFromPos();
    }
//    m_ctrl = ctrl;
//    m_originDragged = ctrl;
//    setFeatureXYFromPos();
//    drawBalloon(true);
//    auto dvb(dynamic_cast<TechDraw::DrawViewBalloon*>(getViewObject()));
//    if (!dvb) {
//        return;
//    }

//    if (!m_dragInProgress) {    //first drag movement
//        m_dragInProgress = true;
//        Base::Console().Message("QGIVB::balloonLabelDragged - center: %s arrow: %s\n",
//                                DU::formatVector(balloonLabel->getLabelCenter()).c_str(),
//                                DU::formatVector(arrow->pos()).c_str());
//        QPointF offset = arrow->pos() - balloonLabel->boundingRect().center();
//        m_saveOffset = DU::toVector3d(offset);
//    }

//    // store if origin is also moving to be able to later calc new origin and update feature
//    if (ctrl) {
//        m_originDragged = true;
//    }

//    DrawView* balloonParent = getSourceView();
//    if (balloonParent) {
//        // redraw the balloon at the new position
//        // note that we don't store the new position to the X/Y properties
//        // since the dragging is not yet finished
//        drawBalloon(true);
//    }
}

void QGIViewBalloon::balloonLabelDragFinished()
{
    Base::Console().Message("QGIVB::balloonLabelDragFinished\n");
    if (m_dragState == DRAGGING) {
        m_originDragged = false;
        m_dragState = NODRAG;
        setFeatureXYFromPos();
    }
}

//from QGVP::mouseReleaseEvent - pos = eventPos in scene coords
void QGIViewBalloon::placeBalloon(QPointF scenePos)
{
    Base::Console().Message("QGIVB::placeBalloon(%s)\n",
                            DrawUtil::formatVector(scenePos).c_str());
    auto balloonFeat(dynamic_cast<TechDraw::DrawViewBalloon*>(getViewObject()));
    if (!balloonFeat) {
        return;
    }

    DrawView* balloonParent = dynamic_cast<DrawView*>(balloonFeat->SourceView.getValue());
    if (!balloonParent) {
        return;
    }

    auto featPage = balloonParent->findParentPage();
    if (!featPage) {
        return;
    }

    int idx = featPage->getNextBalloonIndex();
    QString labelText = QString::number(idx);
    balloonFeat->Text.setValue(std::to_string(idx).c_str());

    auto vp = static_cast<ViewProviderBalloon*>(getViewProvider(getViewObject()));
    if (!vp) {
        return;
    }
    QFont font = balloonLabel->getFont();
    font.setPixelSize(calculateFontPixelSize(vp->Fontsize.getValue()));
    font.setFamily(QString::fromUtf8(vp->Font.getValue()));
    font.setPixelSize(exactFontSize(vp->Font.getValue(), vp->Fontsize.getValue()));
    balloonLabel->setFont(font);
    balloonLabel->setDimString(labelText, Rez::guiX(balloonFeat->TextWrapLen.getValue()));

    prepareGeometryChange();

    // Default label position
    Gui::ViewProvider* objVp = QGIView::getViewProvider(balloonParent);
    auto partVP = dynamic_cast<ViewProviderViewPart*>(objVp);
    if (!partVP) {
        return;
    }
    QGIView* qgivParent = partVP->getQView();
    if (!qgivParent) {
        return;
    }
    QPointF viewPos = qgivParent->mapFromScene(scenePos);

    //tip position is mouse release pos in parentView coords ==> OriginX, OriginY
    //bubble pos is some arbitrary shift from tip position ==> X, Y
    balloonFeat->OriginX.setValue(Rez::appX(viewPos.x()) / balloonParent->getScale());
    balloonFeat->OriginY.setValue(-Rez::appX(viewPos.y()) / balloonParent->getScale());
    balloonFeat->X.setValue(Rez::appX((viewPos.x() + 200.0) / balloonParent->getScale()));
    balloonFeat->Y.setValue(-Rez::appX((viewPos.y() - 200.0) / balloonParent->getScale()));
    balloonLabel->setPosFromCenter(viewPos.x() + 200, viewPos.y() - 200);

    draw();
}

void QGIViewBalloon::draw()
{
    Base::Console().Message("QGIVB::draw()\n");
    // just redirect
    drawBalloon(false);
}

void QGIViewBalloon::drawBalloon(bool dragged)
{
    Q_UNUSED(dragged)
    Base::Console().Message("QGIVB::drawBalloon(%d)\n", dragged);
    if (!isVisible()) {
        return;
    }

    TechDraw::DrawViewBalloon* balloon = dynamic_cast<TechDraw::DrawViewBalloon*>(getViewObject());
    if ((!balloon) ||//nothing to draw, don't try
        (!balloon->isDerivedFrom(TechDraw::DrawViewBalloon::getClassTypeId()))) {
        balloonLabel->hide();
        hide();
        return;
    }

    balloonLabel->show();
    show();

    const TechDraw::DrawView* refObj = balloon->getParentView();
    if (!refObj) {
        return;
    }

    auto vp = static_cast<ViewProviderBalloon*>(getViewProvider(getViewObject()));
    if (!vp) {
        return;
    }

    m_lineWidth = Rez::guiX(vp->LineWidth.getValue());

    double textWidth = balloonLabel->getDimText()->boundingRect().width();
    double textHeight = balloonLabel->getDimText()->boundingRect().height();

    // figure out where to draw the balloon
    std::pair<QPointF, QPointF> positions;
//    if (m_dragInProgress) {
//        Base::Console().Message("QGIVB::draw - positioning from view\n");
//        positions = getPositionsFromView();
//    } else {
//        positions = getPositionsFromFeature();
//        Base::Console().Message("QGIVB::draw - positioning from feature\n");
//    }
    positions = getPositionsFromFeature();
    double x = positions.first.x();
    double y = positions.first.y();
    double arrowTipX = positions.second.x();
    double arrowTipY = positions.second.y();
    Base::Console().Message("QGIVB::draw - x,y: %.3f, %.3f  tip: %.3f, %.3f\n", x, y, arrowTipX, arrowTipY);
    Base::Vector3d lblCenter(x, y, 0.0);

    Base::Vector3d dLineStart;
    Base::Vector3d kinkPoint;
    double kinkLength = Rez::guiX(balloon->KinkLength.getValue());

    const char* balloonType = balloon->BubbleShape.getValueAsString();

    float scale = balloon->ShapeScale.getValue();
    double offsetLR = 0;
    double offsetUD = 0;
    QPainterPath balloonPath;

    if (strcmp(balloonType, "Circular") == 0) {
        double balloonRadius = sqrt(pow((textHeight / 2.0), 2) + pow((textWidth / 2.0), 2));
        balloonRadius = balloonRadius * scale;
        balloonPath.moveTo(lblCenter.x, lblCenter.y);
        balloonPath.addEllipse(lblCenter.x - balloonRadius, lblCenter.y - balloonRadius,
                               balloonRadius * 2, balloonRadius * 2);
        offsetLR = balloonRadius;
    }
    else if (strcmp(balloonType, "None") == 0) {
        balloonPath = QPainterPath();
        offsetLR = (textWidth / 2.0) + Rez::guiX(2.0);
    }
    else if (strcmp(balloonType, "Rectangle") == 0) {
        //Add some room
        textHeight = (textHeight * scale) + Rez::guiX(1.0);
        // we add some textWidth later because we first need to handle the text separators
        if (balloonLabel->getVerticalSep()) {
            for (auto& sep : balloonLabel->getSeps()) {
                balloonPath.moveTo(lblCenter.x - (textWidth / 2.0) + sep,
                                   lblCenter.y - (textHeight / 2.0));
                balloonPath.lineTo(lblCenter.x - (textWidth / 2.0) + sep,
                                   lblCenter.y + (textHeight / 2.0));
            }
        }
        textWidth = (textWidth * scale) + Rez::guiX(2.0);
        balloonPath.addRect(lblCenter.x - (textWidth / 2.0), lblCenter.y - (textHeight / 2.0),
                            textWidth, textHeight);
        offsetLR = (textWidth / 2.0);
    }
    else if (strcmp(balloonType, "Triangle") == 0) {
        double radius = sqrt(pow((textHeight / 2.0), 2) + pow((textWidth / 2.0), 2));
        radius = radius * scale;
        radius += Rez::guiX(3.0);
        offsetLR = (tan(30 * M_PI / 180) * radius);
        QPolygonF triangle;
        double startAngle = -M_PI / 2;
        double angle = startAngle;
        for (int i = 0; i < 4; i++) {
            triangle +=
                QPointF(lblCenter.x + (radius * cos(angle)), lblCenter.y + (radius * sin(angle)));
            angle += (2 * M_PI / 3);
        }
        balloonPath.moveTo(lblCenter.x + (radius * cos(startAngle)),
                           lblCenter.y + (radius * sin(startAngle)));
        balloonPath.addPolygon(triangle);
    }
    else if (strcmp(balloonType, "Inspection") == 0) {
        //Add some room
        textWidth = (textWidth * scale) + Rez::guiX(2.0);
        textHeight = (textHeight * scale) + Rez::guiX(1.0);
        QPointF textBoxCorner(lblCenter.x - (textWidth / 2.0), lblCenter.y - (textHeight / 2.0));
        balloonPath.moveTo(textBoxCorner);
        balloonPath.lineTo(textBoxCorner.x() + textWidth, textBoxCorner.y());
        balloonPath.arcTo(textBoxCorner.x() + textWidth - (textHeight / 2.0), textBoxCorner.y(),
                          textHeight, textHeight, 90, -180);
        balloonPath.lineTo(textBoxCorner.x(), textBoxCorner.y() + textHeight);
        balloonPath.arcTo(textBoxCorner.x() - (textHeight / 2), textBoxCorner.y(), textHeight,
                          textHeight, -90, -180);
        offsetLR = (textWidth / 2.0) + (textHeight / 2.0);
    }
    else if (strcmp(balloonType, "Square") == 0) {
        //Add some room
        textWidth = (textWidth * scale) + Rez::guiX(2.0);
        textHeight = (textHeight * scale) + Rez::guiX(1.0);
        double max = std::max(textWidth, textHeight);
        balloonPath.addRect(lblCenter.x - (max / 2.0), lblCenter.y - (max / 2.0), max, max);
        offsetLR = (max / 2.0);
    }
    else if (strcmp(balloonType, "Hexagon") == 0) {
        double radius = sqrt(pow((textHeight / 2.0), 2) + pow((textWidth / 2.0), 2));
        radius = radius * scale;
        radius += Rez::guiX(1.0);
        offsetLR = radius;
        QPolygonF triangle;
        double startAngle = -2 * M_PI / 3;
        double angle = startAngle;
        for (int i = 0; i < 7; i++) {
            triangle +=
                QPointF(lblCenter.x + (radius * cos(angle)), lblCenter.y + (radius * sin(angle)));
            angle += (2 * M_PI / 6);
        }
        balloonPath.moveTo(lblCenter.x + (radius * cos(startAngle)),
                           lblCenter.y + (radius * sin(startAngle)));
        balloonPath.addPolygon(triangle);
    }
    else if (strcmp(balloonType, "Line") == 0) {
        textHeight = textHeight * scale + Rez::guiX(0.5);
        textWidth = textWidth * scale + Rez::guiX(1.0);

        offsetLR = textWidth / 2.0;
        offsetUD = textHeight / 2.0;

        balloonPath.moveTo(lblCenter.x - textWidth / 2.0, lblCenter.y + offsetUD);
        balloonPath.lineTo(lblCenter.x + textWidth / 2.0, lblCenter.y + offsetUD);
    }

    balloonShape->setPath(balloonPath);

    offsetLR = (lblCenter.x < arrowTipX) ? offsetLR : -offsetLR;

    if (DrawUtil::fpCompare(kinkLength, 0.0)
        && strcmp(balloonType,
                  "Line")) {//if no kink, then dLine start sb on line from center to arrow
        dLineStart = lblCenter;
        kinkPoint = dLineStart;
    }
    else {
        dLineStart.y = lblCenter.y + offsetUD;
        dLineStart.x = lblCenter.x + offsetLR;
        kinkLength = (lblCenter.x < arrowTipX) ? kinkLength : -kinkLength;
        kinkPoint.y = dLineStart.y;
        kinkPoint.x = dLineStart.x + kinkLength;
    }

    QPainterPath dLinePath;
    dLinePath.moveTo(dLineStart.x, dLineStart.y);
    dLinePath.lineTo(kinkPoint.x, kinkPoint.y);

    double xAdj = 0.0;
    double yAdj = 0.0;
    int endType = balloon->EndType.getValue();
    double arrowAdj = QGIArrow::getOverlapAdjust(
        endType, balloon->EndTypeScale.getValue() * QGIArrow::getPrefArrowSize());

    if (endType == ArrowType::NONE) {
        arrow->hide();
    }
    else {
        arrow->setStyle(endType);

        arrow->setSize(balloon->EndTypeScale.getValue() * QGIArrow::getPrefArrowSize());
        arrow->draw();

        Base::Vector3d arrowTipPos(arrowTipX, arrowTipY, 0.0);
        Base::Vector3d dirballoonLinesLine;
        if (!DrawUtil::fpCompare(kinkLength, 0.0)) {
            dirballoonLinesLine = (arrowTipPos - kinkPoint).Normalize();
        }
        else {
            dirballoonLinesLine = (arrowTipPos - dLineStart).Normalize();
        }

        float arAngle = atan2(dirballoonLinesLine.y, dirballoonLinesLine.x) * 180 / M_PI;

        arrow->setPos(arrowTipX, arrowTipY);
        if ((endType == ArrowType::FILLED_TRIANGLE) && (prefOrthoPyramid())) {
            if (arAngle < 0.0) {
                arAngle += 360.0;
            }
            //set the angle to closest cardinal direction
            if ((45.0 < arAngle) && (arAngle < 135.0)) {
                arAngle = 90.0;
            }
            else if ((135.0 < arAngle) && (arAngle < 225.0)) {
                arAngle = 180.0;
            }
            else if ((225.0 < arAngle) && (arAngle < 315.0)) {
                arAngle = 270.0;
            }
            else {
                arAngle = 0;
            }
            double radAngle = arAngle * M_PI / 180.0;
            double sinAngle = sin(radAngle);
            double cosAngle = cos(radAngle);
            xAdj = Rez::guiX(arrowAdj * cosAngle);
            yAdj = Rez::guiX(arrowAdj * sinAngle);
        }
        arrow->setRotation(arAngle);
        arrow->show();
    }
    dLinePath.lineTo(arrowTipX - xAdj, arrowTipY - yAdj);
    balloonLines->setPath(dLinePath);

    // This overwrites the previously created QPainterPath with empty one, in case it should be hidden.  Should be refactored.
    if (!vp->LineVisible.getValue()) {
        arrow->hide();
        balloonLines->setPath(QPainterPath());
    }

    // redraw the Balloon and the parent View
    if (hasHover && !isSelected()) {
        setPrettyPre();
    }
    else if (isSelected()) {
        setPrettySel();
    }
    else {
        setPrettyNormal();
    }

    if (parentItem()) {
        parentItem()->update();
    }
}

void QGIViewBalloon::setPrettyPre(void)
{
    arrow->setPrettyPre();
    //TODO: primPath needs override for fill
    //balloonShape->setFillOverride(true);   //don't fill with pre or select colours.
    //    balloonShape->setFill(Qt::white, Qt::NoBrush);
    balloonShape->setPrettyPre();
    balloonLines->setPrettyPre();
}

void QGIViewBalloon::setPrettySel(void)
{
    //    Base::Console().Message("QGIVBal::setPrettySel()\n");
    arrow->setPrettySel();
    //    balloonShape->setFill(Qt::white, Qt::NoBrush);
    balloonShape->setPrettySel();
    balloonLines->setPrettySel();
}

void QGIViewBalloon::setPrettyNormal(void)
{
    arrow->setPrettyNormal();
    //    balloonShape->setFill(Qt::white, Qt::SolidPattern);
    balloonShape->setPrettyNormal();
    balloonLines->setPrettyNormal();
}


void QGIViewBalloon::drawBorder(void)
{
    //Dimensions have no border!
    //    Base::Console().Message("TRACE - QGIViewDimension::drawBorder - doing nothing!\n");
}

void QGIViewBalloon::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                           QWidget* widget)
{
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    QPaintDevice* hw = painter->device();
    QSvgGenerator* svg = dynamic_cast<QSvgGenerator*>(hw);
    setPens();
    if (svg) {
        setSvgPens();
    }
    else {
        setPens();
    }
    QGIView::paint(painter, &myOption, widget);
    setPens();
}

void QGIViewBalloon::setSvgPens(void)
{
    double svgLineFactor = 3.0;//magic number.  should be a setting somewhere.
    balloonLines->setWidth(m_lineWidth / svgLineFactor);
    balloonShape->setWidth(m_lineWidth / svgLineFactor);
    arrow->setWidth(arrow->getWidth() / svgLineFactor);
}

void QGIViewBalloon::setPens(void)
{
    balloonLines->setWidth(m_lineWidth);
    balloonShape->setWidth(m_lineWidth);
    arrow->setWidth(m_lineWidth);
}

void QGIViewBalloon::setNormalColorAll()
{
    QColor qc = prefNormalColor();
    balloonLabel->setColor(qc);
    balloonLines->setNormalColor(qc);
    balloonShape->setNormalColor(qc);
    arrow->setNormalColor(qc);
    arrow->setFillColor(qc);
}

QColor QGIViewBalloon::prefNormalColor()
{
    setNormalColor(PreferencesGui::getAccessibleQColor(PreferencesGui::dimQColor()));

    ViewProviderBalloon* vpBalloon = nullptr;
    Gui::ViewProvider* vp = getViewProvider(getBalloonFeat());
    if (vp) {
        vpBalloon = dynamic_cast<ViewProviderBalloon*>(vp);
        if (vpBalloon) {
            App::Color fcColor = Preferences::getAccessibleColor(vpBalloon->Color.getValue());
            setNormalColor(fcColor.asValue<QColor>());
        }
    }
    return getNormalColor();
}

int QGIViewBalloon::prefDefaultArrow() const { return Preferences::balloonArrow(); }


//should this be an object property or global preference?
//when would you want a crooked pyramid?
bool QGIViewBalloon::prefOrthoPyramid() const
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/Decorations");
    bool ortho = hGrp->GetBool("PyramidOrtho", true);
    return ortho;
}

DrawView* QGIViewBalloon::getSourceView() const
{
    DrawView* balloonParent = nullptr;
    App::DocumentObject* docObj = getViewObject();
    DrawViewBalloon* dvb = dynamic_cast<DrawViewBalloon*>(docObj);
    if (dvb) {
        balloonParent = dynamic_cast<DrawView*>(dvb->SourceView.getValue());
    }
    return balloonParent;
}

#include <Mod/TechDraw/Gui/moc_QGIViewBalloon.cpp>
