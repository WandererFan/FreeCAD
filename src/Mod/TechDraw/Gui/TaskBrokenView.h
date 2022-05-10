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

#ifndef _TASKBROKENVIEW_H
#define _TASKBROKENVIEW_H

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
//#include <Inventor/nodes/SoEventCallback.h>


class Ui_TaskBrokenView;
class SoPickedPoint;
class SoEventCallback;

namespace Gui {
    class View3DInventor;
    class View3DInventorViewer;
}

namespace App {
    class DocumentObject;
}

namespace TechDraw {
    class DrawViewPart;
    class DrawBrokenView;
}

namespace TechDrawGui
{

class TechDrawGuiExport TaskBrokenView : public QWidget
{
    Q_OBJECT

public:
    TaskBrokenView(TechDraw::DrawBrokenView* brokenView);
    TaskBrokenView(TechDraw::DrawBrokenView* brokenView, bool editFlag);
    ~TaskBrokenView();

public:
    virtual bool accept();
    virtual bool reject();

    virtual void addPickedPoint(Base::Vector3d p);
    static void pickCallback(void * ud, SoEventCallback * n);

    Gui::View3DInventorViewer* getViewer();

protected Q_SLOTS:
    void onPoint1Changed();
    void onPoint2Changed();
    void useCurrentClicked();
    void from3dClicked();

protected:
    void changeEvent(QEvent *e);
    void saveBrokenState();
    void restoreBrokenState();

    bool apply(bool force = false);

    void updateBrokenView(void);

    void setUiPrimary();
    void setUiEdit();

    void checkAll(bool b);
    void enableAll(bool b);

private:
    std::unique_ptr<Ui_TaskBrokenView> ui;
    TechDraw::DrawBrokenView* m_brokenFeat;
    Base::Vector3d m_savePoint1;
    Base::Vector3d m_savePoint2;
    bool m_editFlag;

    int m_applyDeferred;
    std::vector<Base::Vector3d> m_pickedPoints;
//    Gui::View3DInventor* m_view;
};

class TaskDlgBrokenView : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgBrokenView(TechDraw::DrawBrokenView* brokenView);
    TaskDlgBrokenView(TechDraw::DrawBrokenView* brokenView, bool editFlag);
    ~TaskDlgBrokenView();

public:
    /// is called the TaskView when the dialog is opened
    virtual void open();
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
    /// is called by the framework if the dialog is rejected (Cancel)
    virtual bool reject();
    /// is called by the framework if the user presses the help button
    virtual void helpRequested() { return;}

    virtual QDialogButtonBox::StandardButtons getStandardButtons() const
    { return QDialogButtonBox::Ok | QDialogButtonBox::Cancel; }

    virtual bool isAllowedAlterSelection(void) const
    { return false; }
    virtual bool isAllowedAlterDocument(void) const
    { return false; }

protected:

private:
    TaskBrokenView * widget;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace TechDrawGui

#endif // #ifndef _TASKBROKENVIEW_H
