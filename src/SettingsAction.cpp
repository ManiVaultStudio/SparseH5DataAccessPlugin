#include "SettingsAction.h"

using namespace mv;

SettingsAction::SettingsAction(QObject* parent) :
    gui::GroupAction(parent, "SettingsAction", true),
    _fileOnDiskAction(this, "H5 file on disk"),
    _matrixTypeAction(this, "Matrix storage", "None loaded yet"),
    _numAvailableDimsAction(this, "Variables", "None loaded yet"),
    _dataDimOneAction(this, "Dim 1", {}, ""),
    _saveDataToProjectAction(this, "Save data to project", false)
{
    setText("UMAP Settings");
    setSerializationName("Sparse Matrix Access");

    _fileOnDiskAction.setToolTip("H5 file on disk");
    _matrixTypeAction.setToolTip("Storage type of sparse matrix on disk");
    _numAvailableDimsAction.setToolTip("Number of variables/dimensions/channels in the data");
    _dataDimOneAction.setToolTip("Data dimension 1");
    _saveDataToProjectAction.setToolTip("Saving the data from disk to a project\nmight yield very large project files and loading times!");

    _matrixTypeAction.setDefaultWidgetFlags(gui::StringAction::WidgetFlag::Label);
    _numAvailableDimsAction.setDefaultWidgetFlags(gui::StringAction::WidgetFlag::Label);

    _dataDimOneAction.setDefaultWidgetFlag(gui::OptionAction::WidgetFlag::LineEdit);

    _fileOnDiskAction.setPlaceHolderString("Pick sparse H5 file...");
    _fileOnDiskAction.setFileType("Sparse H5 data");
    _fileOnDiskAction.setNameFilters({ "Images (*.h5)" });

    addAction(&_fileOnDiskAction);
    addAction(&_matrixTypeAction);
    addAction(&_numAvailableDimsAction);
    addAction(&_dataDimOneAction);
    addAction(&_saveDataToProjectAction);
}

void SettingsAction::fromVariantMap(const QVariantMap& variantMap)
{
    gui::GroupAction::fromVariantMap(variantMap);

    _fileOnDiskAction.fromParentVariantMap(variantMap);
    _matrixTypeAction.fromParentVariantMap(variantMap);
    _numAvailableDimsAction.fromParentVariantMap(variantMap);
    _dataDimOneAction.fromParentVariantMap(variantMap);
    _saveDataToProjectAction.fromParentVariantMap(variantMap);
}

QVariantMap SettingsAction::toVariantMap() const
{
    QVariantMap variantMap = gui::GroupAction::toVariantMap();

    _fileOnDiskAction.insertIntoVariantMap(variantMap);
    _matrixTypeAction.insertIntoVariantMap(variantMap);
    _numAvailableDimsAction.insertIntoVariantMap(variantMap);
    _dataDimOneAction.insertIntoVariantMap(variantMap);
    _saveDataToProjectAction.insertIntoVariantMap(variantMap);

    return variantMap;
}

