#include "SparseH5AccessPlugin.h"

#include <CoreInterface.h>
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
    _selectedDimensionIndices(),
    _csrMatrix(),
    _cscMatrix(),
    _sparseMatrix(&_cscMatrix),
    _blockReadingFromFile(false)
{
    connect(&_settingsAction.getFileOnDiskAction(), &gui::FilePickerAction::filePathChanged, this, &SparseH5AccessPlugin::updateFile);
    connect(_settingsAction.getDataDimActions()[0].get(), &gui::OptionAction::currentIndexChanged, this, &SparseH5AccessPlugin::readDataFromDisk);
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
        mv::events().notifyDatasetDataChanged(_outputPoints);
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

    auto& dataDimActions = _settingsAction.getDataDimActions();

    _blockReadingFromFile = true;

    for (int numDimAction = 0; numDimAction < _numDims; numDimAction++) {
        auto& action = dataDimActions[numDimAction];
        action->setCurrentIndex(0);
        action->setOptions(varNames);
        action->setCurrentIndex(numDimAction);
    }

    _blockReadingFromFile = false;

    readDataFromDisk();
}

void SparseH5AccessPlugin::readDataFromDisk() {

    if (_blockReadingFromFile) {
        return;
    }

    std::vector<std::int32_t> selectedDimensionIndices = _settingsAction.getSelectedOptionIndices();

    if (_selectedDimensionIndices == selectedDimensionIndices) {
        return;
    }

    std::swap(_selectedDimensionIndices, selectedDimensionIndices);

    using ResultType = std::pair<std::vector<float>, std::vector<QString>>;

    auto readDataAsync = [this]() -> ResultType {

        assert(_numDims = _selectedDimensionIndices.size());

        // Read dimensions from disk
        std::vector<std::vector<float>> dimensionValues(_numDims);
        std::vector<QString> dimensionNames(_numDims);
        const std::vector<std::string>& allDimNames = _sparseMatrix->getVarNames();
        for (size_t dim = 0; dim < _numDims; ++dim) {
            dimensionValues[dim] = _sparseMatrix->getColumn(_selectedDimensionIndices[dim]);
            dimensionNames[dim] = QString::fromStdString(allDimNames[dim]);
        }

        // Interleave data and pass to core
        const size_t total_size = _numPoints * _numDims;
        std::vector<float> dimensionValuesInterleaved(total_size);

#pragma omp parallel for
        for (std::int64_t point = 0; point < static_cast<std::int64_t>(_numPoints); ++point) {
            for (size_t dim = 0; dim < _numDims; ++dim) {
                dimensionValuesInterleaved[_numDims * point + dim] = dimensionValues[dim][point];
            }
        }

        return std::make_pair(std::move(dimensionValuesInterleaved), std::move(dimensionNames));
        };

    auto passDataToCore = [this](ResultType result) -> void {
        const auto& data = result.first;
        const auto& names = result.second;

        _outputPoints->setData(std::move(data), _numDims);
        _outputPoints->setDimensionNames(names);
        mv::events().notifyDatasetDataChanged(_outputPoints);
        };

    // Read data asynchronously, then update core data in main thread
    auto future = QtConcurrent::run(readDataAsync).then(this, passDataToCore);
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
