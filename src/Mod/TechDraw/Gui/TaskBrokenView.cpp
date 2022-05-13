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
#include <cmath>
#endif // #ifndef _PreComp_

#include <QApplication>
#include <QStatusBar>
#include <QMessageBox>

# include <Inventor/SoPickedPoint.h>
# include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoLineSet.h>

#include <Base/Console.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/WaitCursor.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawBrokenView.h>
#include <Mod/TechDraw/App/DrawUtil.h>

#include "Rez.h"
#include "MDIViewPage.h"

#include <Mod/TechDraw/Gui/ui_TaskBrokenView.h>

#include "TaskBrokenView.h"

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

//ctor for create 
TaskBrokenView::TaskBrokenView(TechDraw::DrawBrokenView* brokenView) :
    ui(new Ui_TaskBrokenView),
    m_brokenFeat(brokenView),
    m_editFlag(false),
    m_applyDeferred(0)
{
    m_pickedPoints.clear();
    ui->setupUi(this);

    connect(ui->sbPoint1X, SIGNAL(valueChanged(double)), this, SLOT(onPoint1Changed()));
    connect(ui->sbPoint1Y, SIGNAL(valueChanged(double)), this, SLOT(onPoint1Changed()));
    connect(ui->sbPoint1Z, SIGNAL(valueChanged(double)), this, SLOT(onPoint1Changed()));
    connect(ui->sbPoint2X, SIGNAL(valueChanged(double)), this, SLOT(onPoint2Changed()));
    connect(ui->sbPoint2Y, SIGNAL(valueChanged(double)), this, SLOT(onPoint2Changed()));
    connect(ui->sbPoint2Z, SIGNAL(valueChanged(double)), this, SLOT(onPoint2Changed()));

    connect(ui->pbUseCurrent, SIGNAL(clicked(bool)), this, SLOT(useCurrentClicked()));
    connect(ui->pbFrom3D, SIGNAL(clicked(bool)), this, SLOT(from3dClicked()));

    setUiPrimary();
}


//ctor for edit
TaskBrokenView::TaskBrokenView(TechDraw::DrawBrokenView* brokenView, bool editFlag) :
    ui(new Ui_TaskBrokenView),
    m_brokenFeat(brokenView),
    m_editFlag(true),
    m_applyDeferred(0)
{
    m_pickedPoints.clear();
    ui->setupUi(this);

    connect(ui->sbPoint1X, SIGNAL(valueChanged(double)), this, SLOT(onPoint1Changed()));
    connect(ui->sbPoint1Y, SIGNAL(valueChanged(double)), this, SLOT(onPoint1Changed()));
    connect(ui->sbPoint1Z, SIGNAL(valueChanged(double)), this, SLOT(onPoint1Changed()));
    connect(ui->sbPoint2X, SIGNAL(valueChanged(double)), this, SLOT(onPoint2Changed()));
    connect(ui->sbPoint2Y, SIGNAL(valueChanged(double)), this, SLOT(onPoint2Changed()));
    connect(ui->sbPoint2Z, SIGNAL(valueChanged(double)), this, SLOT(onPoint2Changed()));

    connect(ui->pbUseCurrent, SIGNAL(clicked(bool)), this, SLOT(useCurrentClicked()));
    connect(ui->pbFrom3D, SIGNAL(clicked(bool)), this, SLOT(from3dClicked()));

    setUiEdit();
}

TaskBrokenView::~TaskBrokenView()
{
}

void TaskBrokenView::setUiPrimary()
{
//    Base::Console().Message("TBV::setUiPrimary()\n");
    setWindowTitle(QObject::tr("Create Broken View"));

    std::vector<Base::Vector3d> points = m_brokenFeat->CutPoints.getValues();

    if (points.size() > 1) {
        ui->sbPoint1X->setUnit(Base::Unit::Length);
        ui->sbPoint1X->setValue(points[0].x);
        ui->sbPoint1Y->setUnit(Base::Unit::Length);
        ui->sbPoint1Y->setValue(points[0].y);
        ui->sbPoint1Z->setUnit(Base::Unit::Length);
        ui->sbPoint1Z->setValue(points[0].z);
        ui->sbPoint2X->setUnit(Base::Unit::Length);
        ui->sbPoint2X->setValue(points[1].x);
        ui->sbPoint2Y->setUnit(Base::Unit::Length);
        ui->sbPoint2Y->setValue(points[1].y);
        ui->sbPoint2Z->setUnit(Base::Unit::Length);
        ui->sbPoint2Z->setValue(points[1].z);
    }
}

void TaskBrokenView::setUiEdit()
{
//    Base::Console().Message("TBV::setUiEdit()\n");
    setWindowTitle(QObject::tr("Edit Section View"));

    std::vector<Base::Vector3d> points = m_brokenFeat->CutPoints.getValues();

    if (points.size() > 1) {
        ui->sbPoint1X->setUnit(Base::Unit::Length);
        ui->sbPoint1X->setValue(points[0].x);
        ui->sbPoint1Y->setUnit(Base::Unit::Length);
        ui->sbPoint1Y->setValue(points[0].y);
        ui->sbPoint1Z->setUnit(Base::Unit::Length);
        ui->sbPoint1Z->setValue(points[0].z);
        ui->sbPoint2X->setUnit(Base::Unit::Length);
        ui->sbPoint2X->setValue(points[1].x);
        ui->sbPoint2Y->setUnit(Base::Unit::Length);
        ui->sbPoint2Y->setValue(points[1].y);
        ui->sbPoint2Z->setUnit(Base::Unit::Length);
        ui->sbPoint2Z->setValue(points[1].z);
    }
}

//save the start conditions
void TaskBrokenView::saveBrokenState()
{
//    Base::Console().Message("TBV::saveBrokenState()\n");

    if (m_brokenFeat != nullptr) {
        std::vector<Base::Vector3d> points = m_brokenFeat->CutPoints.getValues();
        if (points.size() > 1) {
            m_savePoint1 = points[0];
            m_savePoint2 = points[1];
        }
    }
}

//restore the start conditions
void TaskBrokenView::restoreBrokenState()
{
//    Base::Console().Message("TBV::restoreBrokenState()\n");
    if (m_brokenFeat != nullptr) {
        std::vector<Base::Vector3d> points;
        points.push_back(m_savePoint1);
        points.push_back(m_savePoint2);
        m_brokenFeat->CutPoints.setValues(points);
    }
}


void TaskBrokenView::onPoint1Changed()
{
    apply();
}

void TaskBrokenView::onPoint2Changed()
{
    apply();
}

void TaskBrokenView::useCurrentClicked()
{
    apply();
}

void TaskBrokenView::from3dClicked()
{
    m_pickedPoints.clear();
//    //try to use the ActiveWindow
//    MDIView* active = Gui::getMainWindow()->activeWindow();
//    if (active && active->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
//        m_view = qobject_cast<Gui::View3DInventor*>(active);
//    } else {
//        //use the first MDIView that is a View3DInventor
//        QList<QWidget*> windows = Gui::getMainWindow()->windows();
//        for (auto it : windows) {
//            MDIView* view = qobject_cast<MDIView*>(it);
//            if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
//                m_view = qobject_cast<Gui::View3DInventor*>(view);
//                Gui::getMainWindow()->setActiveWindow(m_view);
//            }
//        }
//    }
///    m_view = qobject_cast<Gui::View3DInventor*>(Gui::getMainWindow()->activeWindow());
//    if (m_view) {
        Gui::View3DInventorViewer* viewer = getViewer();
        if (viewer) {
            viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(), pickCallback, this);
            Base::Console().Message("TBV::from3dClicked - pick 2 points\n");
        }
//    }
    setupDragger();
}

void TaskBrokenView::addPickedPoint(Base::Vector3d p)
{
    Base::Console().Message("TBV::addPickedPoint(%s)\n", DrawUtil::formatVector(p).c_str());
    m_pickedPoints.push_back(p);
    if (m_pickedPoints.size() == 1) {
        ui->sbPoint1X->setValue(m_pickedPoints[0].x);
        ui->sbPoint1Y->setValue(m_pickedPoints[0].y);
        ui->sbPoint1Z->setValue(m_pickedPoints[0].z);
        SbVec3f start(m_pickedPoints[0].x,
                      m_pickedPoints[0].y,
                      m_pickedPoints[0].z);
        m_linePoints->point.set1Value(0, start);
    } else if (m_pickedPoints.size() > 1) {
        ui->sbPoint2X->setValue(m_pickedPoints[1].x);
        ui->sbPoint2Y->setValue(m_pickedPoints[1].y);
        ui->sbPoint2Z->setValue(m_pickedPoints[1].z);
        SbVec3f end(m_pickedPoints[1].x,
                      m_pickedPoints[1].y,
                      m_pickedPoints[1].z);
        m_linePoints->point.set1Value(1, end);
        //finished, time to clean up
        Gui::View3DInventorViewer* viewer = getViewer();
        viewer->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), pickCallback, this);
//        m_view = nullptr;
        apply();
    }
}

void TaskBrokenView::pickCallback(void* userData, SoEventCallback* cbNode)
{
    const SoMouseButtonEvent * mbe = static_cast<const SoMouseButtonEvent*>(cbNode->getEvent());
    TaskBrokenView* dlg = reinterpret_cast<TaskBrokenView*>(userData);
    // Mark all incoming mouse button events as handled, especially, to deactivate the selection node
    cbNode->getAction()->setHandled();
    if (mbe->getButton() == SoMouseButtonEvent::BUTTON1) {
        if (mbe->getState() == SoButtonEvent::DOWN) {
//            const SoPickedPoint * point = cbNode->getPickedPoint();
            SbVec2s screenPos = mbe->getPosition();
            SbVec3f point = dlg->getViewer()->getPointOnFocalPlane(screenPos);
//            if (point) {
//                SbVec3f pnt = point->getPoint();
                Base::Vector3d pointToAdd(point[0], point[1], point[2]);
                dlg->addPickedPoint(pointToAdd);
                cbNode->setHandled();
//            }
        }
    }
}

Gui::View3DInventorViewer* TaskBrokenView::getViewer()
{
    Gui::View3DInventor* view = nullptr;
    Gui::View3DInventorViewer* viewer = nullptr;
    //try to use the ActiveWindow
    MDIView* active = Gui::getMainWindow()->activeWindow();
    if (active && active->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        view = qobject_cast<Gui::View3DInventor*>(active);
    } else {
        //use the first MDIView that is a View3DInventor
        QList<QWidget*> windows = Gui::getMainWindow()->windows();
        for (auto it : windows) {
            MDIView* mdiView = qobject_cast<MDIView*>(it);
            if (mdiView && mdiView->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
                view = qobject_cast<Gui::View3DInventor*>(mdiView);
            }
        }
    }
    if (view) {
        viewer = view->getViewer();
        Gui::getMainWindow()->setActiveWindow(view);
    }
    return viewer;
}

void TaskBrokenView::setupDragger(void)
{
    SoNode* scene = getViewer()->getSceneGraph();
    SoSeparator* sepScene = static_cast<SoSeparator*>(scene);

    SoSeparator* dragger = new SoSeparator();
    SoBaseColor* color = new SoBaseColor();
    SoDrawStyle* style = new SoDrawStyle();
    style->lineWidth = 2.0f;
//    SoCoordinate3* linePoints = new SoCoordinate3();
    m_linePoints = new SoCoordinate3();
//    SbVec3f start(0.0, 0.0, 0.0);
//    SbVec3f end(100.0, 100.0, 100.0);
//    m_linePoints->point.set1Value(0, start);
//    m_linePoints->point.set1Value(1, end);
    SoLineSet* line = new SoLineSet();
    line->numVertices.setValue(2);
    dragger->addChild(color);
    dragger->addChild(style);
    dragger->addChild(m_linePoints);
    dragger->addChild(line);
    sepScene->addChild(dragger);
}

//******************************************************************************
bool TaskBrokenView::apply(bool forceUpdate)
{
//    if(!ui->cbLiveUpdate->isChecked() &&
//       !forceUpdate) {
//        //nothing to do
//        m_applyDeferred++;
//        QString msgLiteral = QString::fromUtf8(QT_TRANSLATE_NOOP("TaskPojGroup", " updates pending"));
//        QString msgNumber = QString::number(m_applyDeferred);
//        ui->lPendingUpdates->setText(msgNumber + msgLiteral);
//        return false;
//    }

//    Base::Console().Message("TBV::apply()\n");
    Gui::WaitCursor wc;
    double x1 = ui->sbPoint1X->value().getValue();
    double y1 = ui->sbPoint1Y->value().getValue();
    double z1 = ui->sbPoint1Z->value().getValue();
    double x2 = ui->sbPoint2X->value().getValue();
    double y2 = ui->sbPoint2Y->value().getValue();
    double z2 = ui->sbPoint2Z->value().getValue();
    Base::Vector3d p1(x1, y1, z1);
    Base::Vector3d p2(x2, y2, z2);
    std::vector<Base::Vector3d> points;
    points.push_back(p1);
    points.push_back(p2);
    m_brokenFeat->CutPoints.setValues(points);
    m_brokenFeat->recomputeFeature();

    wc.restoreCursor();
//    m_applyDeferred = 0;
//    ui->lPendingUpdates->setText(QString());
    return true;
}

void TaskBrokenView::updateBrokenView(void)
{
//    Base::Console().Message("TBV::updateBrokenView()\n");
//    if (!isSectionValid()) {
//        failNoObject();
//        return;
//    }

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Update Broken View"));
    if (m_brokenFeat != nullptr) {
//        Command::doCommand(Command::Doc,"App.ActiveDocument.%s.SectionDirection = '%s'",
//                           m_sectionName.c_str(),m_dirName.c_str());
//        Command::doCommand(Command::Doc,
//                           "App.ActiveDocument.%s.SectionOrigin = FreeCAD.Vector(%.3f,%.3f,%.3f)",
//                           m_sectionName.c_str(), 
//                           ui->sbPoint1X->value().getValue(),
//                           ui->sbPoint1Y->value().getValue(),
//                           ui->sbPoint1Z->value().getValue());
    }
    Gui::Command::commitCommand();
}

//void TaskBrokenView::failNoObject(void)
//{
//    QString qsectionName = Base::Tools::fromStdString(m_sectionName);
//    QString qbaseName = Base::Tools::fromStdString(m_baseName);
//    QString msg = tr("Can not continue. Object * %1 or %2 not found.").arg(qsectionName).arg(qbaseName);
//    QMessageBox::critical(Gui::getMainWindow(), QObject::tr("Operation Failed"), msg);
//}

//******************************************************************************

bool TaskBrokenView::accept()
{
//    Base::Console().Message("TBV::accept()\n");
    if (m_brokenFeat != nullptr) {
        apply(true);
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");
        return true;
    }
//    failNoObject();
    return true;
}

bool TaskBrokenView::reject()
{
//    Base::Console().Message("TBV::reject()\n");
    if (m_brokenFeat != nullptr) {
        restoreBrokenState();
    }

    Gui::Command::updateActive();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");

    return false;
}

void TaskBrokenView::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgBrokenView::TaskDlgBrokenView(TechDraw::DrawBrokenView* brokenView) :
    TaskDialog()
{
    widget  = new TaskBrokenView(brokenView);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_BrokenView"),
                                         widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgBrokenView::TaskDlgBrokenView(TechDraw::DrawBrokenView* brokenView, bool editFlag) :
    TaskDialog()
{
    widget  = new TaskBrokenView(brokenView, editFlag);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_BrokenView"),
                                         widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}
TaskDlgBrokenView::~TaskDlgBrokenView()
{
}

//==== calls from the TaskView ===============================================================
void TaskDlgBrokenView::open()
{
}

bool TaskDlgBrokenView::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgBrokenView::reject()
{
    widget->reject();
    return true;
}

#include <Mod/TechDraw/Gui/moc_TaskBrokenView.cpp>
