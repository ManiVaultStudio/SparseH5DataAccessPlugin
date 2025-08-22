/**
 * Plugin to access one or multiple rows of sparse matrices stored with H5 on disk.
 *
 * @authors A.Vieth
 */
#pragma once

#include <AnalysisPlugin.h>

#include <Dataset.h>
#include <PointData/PointData.h>

#include "H5Utils.h"
#include "SettingsAction.h"

#include <QString>
#include <QStringList>
#include <QVariantMap>

#include <cstdint>
#include <vector>

// =============================================================================
// Analysis
// =============================================================================

class SparseH5AccessPlugin : public mv::plugin::AnalysisPlugin
{
    Q_OBJECT

public:

    SparseH5AccessPlugin(const PluginFactory* factory);
    ~SparseH5AccessPlugin() override;

    void init() override;

private:
    // Default: select the first two dimensions of the data
    void updateFile(const QString& filePathQt);

    void readDataFromDisk();
    void updateOptionsForDim(const std::int32_t numDim, const QStringList& dimNames);

    bool saveFileToProject(QVariantMap& variantMap) const;
    bool loadFileFromProject(const QVariantMap& variantMap);

public: // Serialization

    Q_INVOKABLE void fromVariantMap(const QVariantMap& variantMap) override;
    Q_INVOKABLE QVariantMap toVariantMap() const override;

private:
    SettingsAction              _settingsAction;    /** General settings */

    size_t                      _numPoints;         /** Numer of data points */
    size_t                      _numDims;           /** The number of dimensions */
    mv::Dataset<Points>         _outputPoints;
    std::vector<std::int32_t>   _selectedDimensionIndices;
    QStringList                 _dimensionNames;

    CSRReader                   _csrMatrix;
    CSCReader                   _cscMatrix;
    SparseMatrixReader*         _sparseMatrix;

    bool                        _blockReadingFromFile;
};

// =============================================================================
// Factory
// =============================================================================

class SparseH5AccessPluginFactory : public mv::plugin::AnalysisPluginFactory
{
    Q_OBJECT
    Q_INTERFACES(mv::plugin::AnalysisPluginFactory mv::plugin::PluginFactory)
    Q_PLUGIN_METADATA(IID   "studio.manivault.SparseH5AccessPlugin"
                      FILE  "PluginInfo.json")

public:
    SparseH5AccessPluginFactory();
    ~SparseH5AccessPluginFactory() override {}

    /** Creates an instance of the UMAP analysis plugin */
    AnalysisPlugin* produce() override;

    /** Returns the data types that are supported by the UMAP analysis plugin */
    mv::DataTypes supportedDataTypes() const override;

    /** Enable right-click on data set to open analysis */
    mv::gui::PluginTriggerActions getPluginTriggerActions(const mv::Datasets& datasets) const override;
};
