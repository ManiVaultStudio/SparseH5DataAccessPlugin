#include "SettingsAction.h"

using namespace mv;

SettingsAction::SettingsAction(QObject* parent) :
    gui::GroupAction(parent, "SettingsAction", true),
    _fileOnDiskAction(this, "H5 on disk"),
    _dataDimOneAction(this, "Dim 1", {}, ""),
    _dataDimTwoAction(this, "Dim 2", {}, "")
{
    setText("UMAP Settings");
    setSerializationName("UMAP Settings");

    _fileOnDiskAction.setToolTip("H5 file on disk");
    _dataDimOneAction.setToolTip("Data dimension 1");
    _dataDimTwoAction.setToolTip("Data dimension 2");

    _fileOnDiskAction.setPlaceHolderString("Pick sparse H5 file...");
    _fileOnDiskAction.setFileType("Sparse H5 data");
    _fileOnDiskAction.setNameFilters({ "Images (*.h5)" });

    addAction(&_fileOnDiskAction);
    addAction(&_dataDimOneAction);
    addAction(&_dataDimTwoAction);
}

void SettingsAction::fromVariantMap(const QVariantMap& variantMap)
{
    gui::GroupAction::fromVariantMap(variantMap);

    _fileOnDiskAction.fromParentVariantMap(variantMap);
    _dataDimOneAction.fromParentVariantMap(variantMap);
    _dataDimTwoAction.fromParentVariantMap(variantMap);
}

QVariantMap SettingsAction::toVariantMap() const
{
    QVariantMap variantMap = gui::GroupAction::toVariantMap();

    _fileOnDiskAction.insertIntoVariantMap(variantMap);
    _dataDimOneAction.insertIntoVariantMap(variantMap);
    _dataDimTwoAction.insertIntoVariantMap(variantMap);

    return variantMap;
}

