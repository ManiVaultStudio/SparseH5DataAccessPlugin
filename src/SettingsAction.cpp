#include "SettingsAction.h"

using namespace mv;

SettingsAction::SettingsAction(QObject* parent) :
    gui::GroupAction(parent, "SettingsAction", true),
    _fileOnDiskAction(this, "H5 on disk"),
    _numAvailableDimsAction(this, "Variables"),
    _dataDimOneAction(this, "Dim 1", {}, "")
{
    setText("UMAP Settings");
    setSerializationName("UMAP Settings");

    _fileOnDiskAction.setToolTip("H5 file on disk");
    _numAvailableDimsAction.setToolTip("Number of variables/dimensions/channels in the data");
    _dataDimOneAction.setToolTip("Data dimension 1");

    _numAvailableDimsAction.setDefaultWidgetFlag(gui::StringAction::WidgetFlag::Label);

    _dataDimOneAction.setDefaultWidgetFlag(gui::OptionAction::WidgetFlag::LineEdit);

    _fileOnDiskAction.setPlaceHolderString("Pick sparse H5 file...");
    _fileOnDiskAction.setFileType("Sparse H5 data");
    _fileOnDiskAction.setNameFilters({ "Images (*.h5)" });

    addAction(&_fileOnDiskAction);
    addAction(&_numAvailableDimsAction);
    addAction(&_dataDimOneAction);
}

void SettingsAction::fromVariantMap(const QVariantMap& variantMap)
{
    gui::GroupAction::fromVariantMap(variantMap);

    _fileOnDiskAction.fromParentVariantMap(variantMap);
    _numAvailableDimsAction.fromParentVariantMap(variantMap);
    _dataDimOneAction.fromParentVariantMap(variantMap);
}

QVariantMap SettingsAction::toVariantMap() const
{
    QVariantMap variantMap = gui::GroupAction::toVariantMap();

    _fileOnDiskAction.insertIntoVariantMap(variantMap);
    _numAvailableDimsAction.insertIntoVariantMap(variantMap);
    _dataDimOneAction.insertIntoVariantMap(variantMap);

    return variantMap;
}

