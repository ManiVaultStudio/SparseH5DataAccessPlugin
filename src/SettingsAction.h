#pragma once

#include "actions/GroupAction.h"
#include "actions/OptionAction.h"
#include "actions/FilePickerAction.h"
#include "actions/StringAction.h"

// TODO: for now, two dimensions, but goal is to have this user-adjustable

class SettingsAction : public mv::gui::GroupAction
{
public:
    SettingsAction(QObject* parent = nullptr);

public: // Action getters

    mv::gui::FilePickerAction& getFileOnDiskAction() { return _fileOnDiskAction; }
    mv::gui::StringAction& getNumAvailableDimsAction() { return _numAvailableDimsAction; }
    mv::gui::OptionAction& getDataDimOneAction() { return _dataDimOneAction; }

public: // Serialization

    void fromVariantMap(const QVariantMap& variantMap) override;
    QVariantMap toVariantMap() const override;

protected:
    mv::gui::FilePickerAction      _fileOnDiskAction;           /** File on disk */
    mv::gui::StringAction          _numAvailableDimsAction;     /** Shows number of available dimension */
    mv::gui::OptionAction          _dataDimOneAction;           /** First dimension */
};
