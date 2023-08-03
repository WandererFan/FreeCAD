/***************************************************************************
 *   Copyright (c) 2022 Benjamin Bræstrup Sayoc <benj5378@outlook.com>     *
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
# include <QDialog>
#endif

#include <Mod/TechDraw/App/Preferences.h>

#include "DlgPrefsTechDrawTemplate.h"
#include "ui_DlgPrefsTechDrawTemplate.h"


using namespace TechDrawGui;

DlgPrefsTechDrawTemplate::DlgPrefsTechDrawTemplate(QWidget* parent) :
    PreferencePage(parent),
    ui(new Ui_DlgPrefsTechDrawTemplate)
{
    ui->setupUi(this);

    connect(ui->button_add, &QPushButton::released, this, &DlgPrefsTechDrawTemplate::addRow);
    connect(ui->button_remove, &QPushButton::released, this, &DlgPrefsTechDrawTemplate::removeRow);
}

DlgPrefsTechDrawTemplate::~DlgPrefsTechDrawTemplate()
{

}

void DlgPrefsTechDrawTemplate::addRow()
{
    int location = ui->tabDefaults->rowCount();
    ui->tabDefaults->insertRow(location);
}

void DlgPrefsTechDrawTemplate::removeRow()
{
    if(ui->tabDefaults->selectedRanges().isEmpty()) {
        return;  // No selection
    }

    QTableWidgetSelectionRange selection = ui->tabDefaults->selectedRanges().at(0);
    for (int row = selection.bottomRow(); row >= selection.topRow(); row--) {
        ui->tabDefaults->removeRow(row);
    }
}

void DlgPrefsTechDrawTemplate::saveSettings()
{
    // get all the string entries for this branch of the preference tree
    std::vector<std::pair<std::string,std::string> > entryMap =
        TechDraw::Preferences::getPreferenceGroup("Template")->GetASCIIMap();
    // delete the old entries
    for (auto& item : entryMap) {
        TechDraw::Preferences::getPreferenceGroup("Template")->RemoveASCII(item.first.c_str());
    }
    // save the new entries from the table widget
//    ui->tabDefaults->onSave();
   for (int iRow = 0; iRow < ui->tabDefaults->rowCount(); iRow++) {
        auto qKey = ui->tabDefaults->item(iRow, 0)->text();
        auto qValue = ui->tabDefaults->item(iRow, 1)->text();
        TechDraw::Preferences::getPreferenceGroup("Template")->SetASCII(qKey.toStdString().c_str(), qValue.toStdString().c_str());
    }
}

void DlgPrefsTechDrawTemplate::loadSettings()
{
    // get all the string entries for this branch of the preference tree
    std::vector<std::pair<std::string,std::string> > entryMap =
        TechDraw::Preferences::getPreferenceGroup("Template")->GetASCIIMap();
    ui->tabDefaults->setColumnCount(2);
    ui->tabDefaults->setRowCount(entryMap.size());
    int iRow  = 0;
    for (auto& item : entryMap) {
        auto key = item.first;
        ui->tabDefaults->setItem(
            iRow,
            0,
            new QTableWidgetItem(QString::fromStdString(key)));
        auto value = item.second;
        ui->tabDefaults->setItem(
            iRow,
            1,
            new QTableWidgetItem(QString::fromStdString(value)));
        iRow++;
    }
//    ui->tabDefaults->onRestore();
}

void DlgPrefsTechDrawTemplate::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
}
