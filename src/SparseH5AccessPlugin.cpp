#include "SparseH5AccessPlugin.h"

#include <util/Icon.h>

#include <PointData/InfoAction.h>

#include <QDebug>

Q_PLUGIN_METADATA(IID "studio.manivault.SparseH5AccessPlugin")

using namespace mv;

SparseH5AccessPlugin::SparseH5AccessPlugin(const PluginFactory* factory) :
    AnalysisPlugin(factory),
    _settingsAction(this),
    _numPoints(),
    _outDimensions(2),
    _outputPoints()
{
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
        initEmbeddingValues.resize(_numPoints * _outDimensions);

        _outputPoints->setData(std::move(initEmbeddingValues), _outDimensions);
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
