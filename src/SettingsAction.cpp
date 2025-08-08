#include "SettingsAction.h"

#include <cassert>

using namespace mv;

SettingsAction::SettingsAction(QObject* parent) :
    gui::GroupAction(parent, "SettingsAction", true),
    _fileOnDiskAction(this, "H5 file on disk"),
    _matrixTypeAction(this, "Matrix storage", "None loaded yet"),
    _numAvailableDimsAction(this, "Variables", "None loaded yet"),
    _addRemoveDimsAction(this),
    _dataDimActions(),
    _dataDimsAction(this, "Data dimensions"),
    _saveDataToProjectAction(this, "Save data to project", false)
{
    setText("UMAP Settings");
    setSerializationName("Sparse Matrix Access");

    _fileOnDiskAction.setToolTip("H5 file on disk");
    _matrixTypeAction.setToolTip("Storage type of sparse matrix on disk");
    _numAvailableDimsAction.setToolTip("Number of variables/dimensions/channels in the data");
    _saveDataToProjectAction.setToolTip("Saving the data from disk to a project\nmight yield very large project files and loading times!");

    _matrixTypeAction.setDefaultWidgetFlags(gui::StringAction::WidgetFlag::Label);
    _numAvailableDimsAction.setDefaultWidgetFlags(gui::StringAction::WidgetFlag::Label);

    appendSingleDataDimAction(1);

    _fileOnDiskAction.setPlaceHolderString("Pick sparse H5 file...");
    _fileOnDiskAction.setFileType("Sparse H5 data");
    _fileOnDiskAction.setNameFilters({ "Images (*.h5)" });

    addAction(&_fileOnDiskAction);
    addAction(&_matrixTypeAction);
    addAction(&_numAvailableDimsAction);
    addAction(&_addRemoveDimsAction);
    addAction(&_dataDimsAction);
    addAction(&_saveDataToProjectAction);
}

SettingsAction::~SettingsAction() {
}

void SettingsAction::appendSingleDataDimAction(const int id)
{
    auto& action = _dataDimActions.emplace_back(std::make_unique<gui::OptionAction>(this, QString("Dim %1").arg(id), QStringList{}, QString{}));
    action->setToolTip(QString("Data dimension %1").arg(id));
    action->setDefaultWidgetFlag(gui::OptionAction::WidgetFlag::LineEdit);
    _dataDimsAction.addAction(action.get());
}

bool SettingsAction::addDataDimAction()
{
    appendSingleDataDimAction(_dataDimActions.size() + 1);

    assert(_dataDimActions.size() == _dataDimsAction.getActions().size());
    assert(_dataDimActions.size() >= 1);

    return true;
}

bool SettingsAction::removeDataDimAction()
{
    if (_dataDimActions.size() <= 1)
        return false;

    disconnect(_dataDimActions.back().get(), nullptr, nullptr, nullptr);

    _dataDimsAction.removeAction(_dataDimActions.back().get());
    _dataDimActions.pop_back();

    assert(_dataDimActions.size() >= 1);
    assert(_dataDimActions.size() == _dataDimsAction.getActions().size());

    return true;
}

void SettingsAction::resetDataDimActions()
{
    while (_dataDimActions.size() < 1) {
        removeDataDimAction();
    }

    assert(_dataDimActions.size() == 1);

    _dataDimActions.back()->initialize(QStringList{}, QString{});
}

std::vector<std::int32_t> SettingsAction::getSelectedOptionIndices() const
{
    const size_t numOptions = _dataDimActions.size();
    std::vector<std::int32_t> selectedOptionIndices(numOptions);

    for (int numOption = 0; numOption < numOptions; numOption++) {
        assert(_dataDimActions[numOption]);
        selectedOptionIndices[numOption] = _dataDimActions[numOption]->getCurrentIndex();
    }

    return selectedOptionIndices;
}

void SettingsAction::fromVariantMap(const QVariantMap& variantMap)
{
    gui::GroupAction::fromVariantMap(variantMap);

    _fileOnDiskAction.fromParentVariantMap(variantMap);
    _matrixTypeAction.fromParentVariantMap(variantMap);
    _numAvailableDimsAction.fromParentVariantMap(variantMap);
    _dataDimsAction.fromParentVariantMap(variantMap);
    _saveDataToProjectAction.fromParentVariantMap(variantMap);
}

QVariantMap SettingsAction::toVariantMap() const
{
    QVariantMap variantMap = gui::GroupAction::toVariantMap();

    _fileOnDiskAction.insertIntoVariantMap(variantMap);
    _matrixTypeAction.insertIntoVariantMap(variantMap);
    _numAvailableDimsAction.insertIntoVariantMap(variantMap);
    _dataDimsAction.insertIntoVariantMap(variantMap);
    _saveDataToProjectAction.insertIntoVariantMap(variantMap);

    return variantMap;
}

