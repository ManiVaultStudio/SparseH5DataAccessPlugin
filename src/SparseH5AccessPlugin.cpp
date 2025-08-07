#include "SparseH5AccessPlugin.h"

#include <Project.h>
#include <util/Icon.h>

#include <PointData/InfoAction.h>

#include <QtConcurrent> 
#include <QDebug>
#include <QList>

#include <cassert>
#include <filesystem>

Q_PLUGIN_METADATA(IID "studio.manivault.SparseH5AccessPlugin")

using namespace mv;
namespace fs = std::filesystem;

// =============================================================================
// Utils
// =============================================================================

static QStringList toQStringList(const std::vector<std::string>& str_vec) {

    const std::int64_t n = static_cast<std::int64_t>(str_vec.size());

    QStringList str_list;
    str_list.resizeForOverwrite(n);

#pragma omp parallel for
    for (std::int64_t i = 0; i < n; ++i) {
        str_list[i] = QString::fromStdString(str_vec[i]);
    }

    return str_list;
}

static std::vector<std::string> toStdStringVec(const QStringList& qstr_lst) {

    const std::int64_t n = static_cast<std::int64_t>(qstr_lst.size());

    std::vector<std::string> str_vec(n);

#pragma omp parallel for
    for (std::int64_t i = 0; i < n; ++i) {
        str_vec[i] = qstr_lst[i].toStdString();
    }

    return str_vec;
}

static std::vector<QString> toQStringVec(const QStringList& qstr_lst) {

    const std::int64_t n = static_cast<std::int64_t>(qstr_lst.size());

    std::vector<QString> str_vec(n);

#pragma omp parallel for
    for (std::int64_t i = 0; i < n; ++i) {
        str_vec[i] = qstr_lst[i];
    }

    return str_vec;
}

// =============================================================================
// Plugin
// =============================================================================

SparseH5AccessPlugin::SparseH5AccessPlugin(const PluginFactory* factory) :
    AnalysisPlugin(factory),
    _settingsAction(this),
    _numPoints(),
    _numDims(1),
    _outputPoints(),
    _csrMatrix(),
    _cscMatrix(),
    _sparseMatrix(&_cscMatrix)
{
    connect(&_settingsAction.getFileOnDiskAction(), &gui::FilePickerAction::filePathChanged, this, &SparseH5AccessPlugin::updateFile);
    connect(&_settingsAction.getDataDimsAction(), &gui::OptionsAction::selectedOptionsChanged, this, &SparseH5AccessPlugin::readDataFromDisk);
}

SparseH5AccessPlugin::~SparseH5AccessPlugin()
{
}

void SparseH5AccessPlugin::init()
{
    const auto inputData = getInputDataset<Points>();
    _numPoints = inputData->getNumPoints();

    if (!mv::projects().isOpeningProject() && !mv::projects().isImportingProject()) {
        _outputPoints = Dataset<Points>(mv::data().createDerivedDataset("Sparse data access", inputData, inputData));
        setOutputDataset(_outputPoints);

        std::vector<float> initEmbeddingValues;
        initEmbeddingValues.resize(_numPoints * _numDims);

        _outputPoints->setData(std::move(initEmbeddingValues), _numDims);
        events().notifyDatasetDataChanged(_outputPoints);
    }
    else {
        _outputPoints = getOutputDataset<Points>();
    }
    
    // Add settings to UI
    _outputPoints->addAction(_settingsAction);
    
    // Automatically focus on the data set
    _outputPoints->getDataHierarchyItem().select();
    _outputPoints->_infoAction->collapse();
}

void SparseH5AccessPlugin::updateFile(const QString& filePathQt)
{
    _csrMatrix.reset();
    _cscMatrix.reset();

    const std::string filePath  = filePathQt.toStdString();
    const SparseMatrixType type = SparseMatrixReader::readMatrixType(filePath);
    const std::string typeStr   = sparseMatrixTypeToString(type);
    
    switch (type)
    {
    case SparseMatrixType::CSR:
        _sparseMatrix = &_csrMatrix;
        break;
    case SparseMatrixType::CSC:
        _sparseMatrix = &_cscMatrix;
        break;
    default:
        _sparseMatrix = &_csrMatrix;
    }

    _sparseMatrix->readFile(filePath);

    // if data has at least two dimensions, use 2 dims
    const auto varNames = toQStringList(_sparseMatrix->getVarNames());
    _numDims = (varNames.size() >= 2) ? 2 : 1;

    _settingsAction.getMatrixTypeAction().setString(QString::fromStdString(typeStr));
    _settingsAction.getNumAvailableDimsAction().setString(QString::number(varNames.size()));

    auto& dataDimsAction = _settingsAction.getDataDimsAction();

    dataDimsAction.blockSignals(true);
    dataDimsAction.setSelectedOptions(QSet<std::int32_t>{});
    dataDimsAction.setOptions(varNames);
    dataDimsAction.blockSignals(false);

    dataDimsAction.setSelectedOptions( QSet<std::int32_t>{ 0, 1 });

}

void SparseH5AccessPlugin::readDataFromDisk(const QStringList& selectedOptions) {

    if (selectedOptions.empty()) {
        qDebug() << "SparseH5AccessPlugin::readDataFromDisk: selectedOptions size is 0";
        return;
    }

    assert(_settingsAction.getDataDimsAction().getSelectedOptionIndices().size() == selectedOptions.size());

    qDebug() << "SparseH5AccessPlugin::readDataFromDisk: " << selectedOptions;

    auto readDataAsync = [this]() -> void {
        const QList<std::int32_t> selectedOptionIdx = _settingsAction.getDataDimsAction().getSelectedOptionIndices();

        _numDims = selectedOptionIdx.size();

        // Read dimensions from disk
        std::vector<std::vector<float>> dimensionValues(_numDims);
        std::vector<QString> dimensionNames(_numDims);
        const std::vector<std::string>& allDimNames = _sparseMatrix->getVarNames();
        for (size_t dim = 0; dim < _numDims; ++dim) {
            dimensionValues[dim] = _sparseMatrix->getColumn(selectedOptionIdx[dim]);
            dimensionNames[dim] = QString::fromStdString(allDimNames[dim]);
        }

        // Interleave data and pass to core
        const size_t total_size = _numPoints * _numDims;
        std::vector<float> sparseVals_combined(total_size);

#pragma omp parallel for
        for (std::int64_t point = 0; point < static_cast<std::int64_t>(_numPoints); ++point) {
            for (size_t dim = 0; dim < _numDims; ++dim) {
                sparseVals_combined[_numDims * point + dim] = dimensionValues[dim][point];
            }
        }

        // update data
        _outputPoints->setData(std::move(sparseVals_combined), _numDims);
        events().notifyDatasetDataChanged(_outputPoints);

        _outputPoints->setDimensionNames(dimensionNames);

        };

    QFuture<void> fvoid = QtConcurrent::run(readDataAsync);
}

void SparseH5AccessPlugin::fromVariantMap(const QVariantMap& variantMap)
{
    AnalysisPlugin::fromVariantMap(variantMap);

    _settingsAction.fromParentVariantMap(variantMap);

    if (_settingsAction.getSaveDataToProjectChecked()) {
        loadFileFromProject(variantMap);
    }

}

QVariantMap SparseH5AccessPlugin::toVariantMap() const
{
    QVariantMap variantMap = AnalysisPlugin::toVariantMap();

    _settingsAction.insertIntoVariantMap(variantMap);

    if (_settingsAction.getSaveDataToProjectChecked()) {
        saveFileToProject(variantMap);
    }

    return variantMap;
}

bool SparseH5AccessPlugin::saveFileToProject(QVariantMap& variantMap) const
{
    const fs::path fileOnDiskPath = _settingsAction.getFileOnDiskPath().toStdString();

    if (fileOnDiskPath.empty()) {
        return false;
    }

    const fs::path mvSaveDir            = mv::projects().getTemporaryDirPath(mv::AbstractProjectManager::TemporaryDirType::Save).toStdString();
    const fs::path fileOnDiskName       = fileOnDiskPath.filename();
    const fs::path savePath             = mvSaveDir / fileOnDiskName;

    const bool success = fs::copy_file(fileOnDiskPath, savePath, fs::copy_options::overwrite_existing);

    if (success) {
        variantMap["FileOnDiskName"]    = QVariant::fromValue(QString::fromStdString(fileOnDiskName.string()));
        qDebug() << "SparseH5AccessPlugin::saveFileToProject: saved file to project: " << fileOnDiskName << ", load path: " << fileOnDiskPath;
    }

    return success;
}

bool SparseH5AccessPlugin::loadFileFromProject(const QVariantMap& variantMap)
{
    const fs::path fileOnDiskName   = variantMap.value("FileOnDiskName", QVariant::fromValue(std::string(""))).toString().toStdString();

    if (fileOnDiskName.empty()) {
        return false;
    }

    const fs::path mvOpenDir    = mv::projects().getTemporaryDirPath(mv::AbstractProjectManager::TemporaryDirType::Open).toStdString();
    const fs::path projectPath  = mv::projects().getCurrentProject()->getFilePath().toStdString();
    const fs::path loadPath     = mvOpenDir / fileOnDiskName;

    if (!fs::exists(loadPath)) {
        qDebug() << "SparseH5AccessPlugin::loadFileFromProject: file does not exist in project: " << fileOnDiskName << ", project path: " << projectPath;
        return false;
    }

    _settingsAction.getFileOnDiskAction().setFilePath(QString::fromStdString(loadPath.string()));

    return true;
}

// =============================================================================
// Factory
// =============================================================================

SparseH5AccessPluginFactory::SparseH5AccessPluginFactory()
{
    setIcon(util::StyledIcon(createPluginIcon("SAH5")));
}

AnalysisPlugin* SparseH5AccessPluginFactory::produce()
{
    return new SparseH5AccessPlugin(this);
}

mv::DataTypes SparseH5AccessPluginFactory::supportedDataTypes() const
{
    return { PointType };
}

mv::gui::PluginTriggerActions SparseH5AccessPluginFactory::getPluginTriggerActions(const mv::Datasets& datasets) const
{
    PluginTriggerActions pluginTriggerActions;

    const auto getPluginInstance = [this](const Dataset<Points>& dataset) -> SparseH5AccessPlugin* {
        return dynamic_cast<SparseH5AccessPlugin*>(plugins().requestPlugin(getKind(), { dataset }));
    };

    const auto numberOfDatasets = datasets.count();

    if (numberOfDatasets >= 1 && PluginFactory::areAllDatasetsOfTheSameType(datasets, PointType)) {
        auto pluginTriggerAction = new PluginTriggerAction(const_cast<SparseH5AccessPluginFactory*>(this), this, "Sparse H5 Access", "Access sparse H5 data on disk", icon(), [this, getPluginInstance, datasets](PluginTriggerAction& pluginTriggerAction) -> void {
            for (const auto& dataset : datasets)
                getPluginInstance(dataset);
            });

        pluginTriggerActions << pluginTriggerAction;
    }

    return pluginTriggerActions;
}
