#pragma once

#include "actions/GroupAction.h"
#include "actions/OptionsAction.h"
#include "actions/FilePickerAction.h"
#include "actions/StringAction.h"
#include "actions/ToggleAction.h"

#include <QString>

// TODO: for now, two dimensions, but goal is to have this user-adjustable

class SettingsAction : public mv::gui::GroupAction
{
public:
    SettingsAction(QObject* parent = nullptr);

public: // Getters

    bool getSaveDataToProjectChecked() const { return _saveDataToProjectAction.isChecked(); }
    QString getFileOnDiskPath() const { return _fileOnDiskAction.getFilePath(); }

public: // Action getters

    mv::gui::FilePickerAction& getFileOnDiskAction() { return _fileOnDiskAction; }
    mv::gui::StringAction& getMatrixTypeAction() { return _matrixTypeAction; }
    mv::gui::StringAction& getNumAvailableDimsAction() { return _numAvailableDimsAction; }
    mv::gui::OptionsAction& getDataDimsAction() { return _dataDimsAction; }
    mv::gui::ToggleAction& getSaveDataToProjectAction() { return _saveDataToProjectAction; }

public: // Serialization

    void fromVariantMap(const QVariantMap& variantMap) override;
    QVariantMap toVariantMap() const override;

protected:
    mv::gui::FilePickerAction      _fileOnDiskAction;           /** File on disk */
    mv::gui::StringAction          _matrixTypeAction;           /** Type of sparse matrix */
    mv::gui::StringAction          _numAvailableDimsAction;     /** Shows number of available dimension */
    mv::gui::OptionsAction         _dataDimsAction;             /** Data dimension */
    mv::gui::ToggleAction          _saveDataToProjectAction;    /** Whether to save the data form disk to the project */
};
