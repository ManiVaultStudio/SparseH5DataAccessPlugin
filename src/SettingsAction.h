#pragma once

#include "AddRemoveButtonAction.h"

#include <actions/GroupAction.h>
#include <actions/OptionAction.h>
#include <actions/FilePickerAction.h>
#include <actions/StringAction.h>
#include <actions/ToggleAction.h>

#include <cstdint>
#include <memory>
#include <vector>

#include <QString>

class SettingsAction : public mv::gui::GroupAction
{
public:
    using OptionActions = std::vector<std::unique_ptr<mv::gui::OptionAction>>;

    SettingsAction(QObject* parent = nullptr);
    ~SettingsAction();

public:
    size_t addDataDimAction();      // returns whether new number of data dim options
    bool removeDataDimAction();     // returns whether removing was successful
    void resetDataDimActions();

public: // Getters

    bool getSaveDataToProjectChecked() const { return _saveDataToProjectAction.isChecked(); }
    QString getFileOnDiskPath() const { return _fileOnDiskAction.getFilePath(); }
    std::vector<std::int32_t> getSelectedOptionIndices() const;

public: // Action getters

    mv::gui::FilePickerAction& getFileOnDiskAction() { return _fileOnDiskAction; }
    mv::gui::StringAction& getMatrixTypeAction() { return _matrixTypeAction; }
    mv::gui::StringAction& getNumAvailableDimsAction() { return _numAvailableDimsAction; }
    AddRemoveButtonAction& getAddRemoveButtonAction() { return _addRemoveDimsAction; }
    OptionActions& getDataDimActions() { return _dataDimActions; }
    mv::gui::ToggleAction& getSaveDataToProjectAction() { return _saveDataToProjectAction; }

public: // Serialization

    void fromVariantMap(const QVariantMap& variantMap) override;
    QVariantMap toVariantMap() const override;

private:
    void appendSingleDataDimAction(const int id);

protected:

    mv::gui::FilePickerAction   _fileOnDiskAction;           /** File on disk */
    mv::gui::StringAction       _matrixTypeAction;           /** Type of sparse matrix */
    mv::gui::StringAction       _numAvailableDimsAction;     /** Shows number of available dimension */
    AddRemoveButtonAction       _addRemoveDimsAction;        /** Buttons to add/remove dimension option */
    OptionActions               _dataDimActions;             /** Data dimension actions */
    mv::gui::GroupAction        _dataDimsAction;             /** Group of data dimension actions */
    mv::gui::ToggleAction       _saveDataToProjectAction;    /** Whether to save the data form disk to the project */
};
