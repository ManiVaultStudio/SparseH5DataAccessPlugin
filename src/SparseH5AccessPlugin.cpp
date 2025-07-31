#include "SparseH5AccessPlugin.h"

#include <util/Icon.h>

#include <PointData/InfoAction.h>

#include <QDebug>

#include <cassert>

Q_PLUGIN_METADATA(IID "studio.manivault.SparseH5AccessPlugin")

using namespace mv;

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
    connect(&_settingsAction.getDataDimOneAction(), &gui::OptionAction::currentIndexChanged, this, [this](const int32_t varIndex) { updateVariable(0, varIndex); });
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

static QStringList toQStringList(const std::vector<std::string>& str_vec) {

    std::int64_t n = static_cast<std::int64_t>(str_vec.size());

    QStringList str_list;
    str_list.resizeForOverwrite(n);

#pragma omp parallel for
    for (std::int64_t i = 0; i < n; ++i) {
        str_list[i] = QString::fromStdString(str_vec[i]);
    }

    return str_list;
}

void SparseH5AccessPlugin::updateFile(const QString& filePathQt)
{
    _csrMatrix.reset();
    _cscMatrix.reset();

    const std::string filePath  = filePathQt.toStdString();
    const SparseMatrixType type = SparseMatrixReader::readMatrixType(filePath);
    
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

    const auto varNames = toQStringList(_sparseMatrix->getVarNames());

    _settingsAction.getNumAvailableDimsAction().setString(QString::number(varNames.size()));

    _settingsAction.getDataDimOneAction().blockSignals(true);

    _settingsAction.getDataDimOneAction().setOptions(varNames);
    _settingsAction.getDataDimOneAction().setCurrentIndex(0);

    _settingsAction.getDataDimOneAction().blockSignals(false);

    updateVariable(0, 0);
}

// TODO: handle this update differently for variable number of dimensions
void SparseH5AccessPlugin::updateVariable(size_t dim, size_t varIndex) {
    const auto varIndex_1 = _settingsAction.getDataDimOneAction().getCurrentIndex();

    auto sparseVals_1 = _sparseMatrix->getColumn(varIndex_1);

    assert(sparseVals_1.size() == _numPoints);

    // update data
    _outputPoints->setData(std::move(sparseVals_1), _numDims);
    events().notifyDatasetDataChanged(_outputPoints);

    // update dimension names
    const std::vector<std::string>& varNames = _sparseMatrix->getVarNames();

    _outputPoints->setDimensionNames({ QString::fromStdString(varNames[varIndex_1]) });
}

void SparseH5AccessPlugin::fromVariantMap(const QVariantMap& variantMap)
{
    AnalysisPlugin::fromVariantMap(variantMap);

    mv::util::variantMapMustContain(variantMap, "Settings");

    _settingsAction.fromParentVariantMap(variantMap);
}

QVariantMap SparseH5AccessPlugin::toVariantMap() const
{
    QVariantMap variantMap = AnalysisPlugin::toVariantMap();

    _settingsAction.insertIntoVariantMap(variantMap);

    return variantMap;
}
 
// =============================================================================
// Factory
// =============================================================================
SparseH5AccessPluginFactory::SparseH5AccessPluginFactory()
{
    setIcon(StyledIcon(createPluginIcon("SAH5")));
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
