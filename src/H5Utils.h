#pragma once

#include <cstdint>
#include <memory>
#include <list>
#include <string>
#include <unordered_map>
#include <optional>
#include <utility>
#include <vector>

// =============================================================================
// H5 utilities
// =============================================================================

namespace H5 {
    class H5File;
    class DataSet;
    class H5Object;
}

std::string readAttributeString(H5::H5Object& file, const std::string& attr_name);

bool groupExists(const H5::H5File& file, const std::string& path);
bool attributeExists(const H5::H5Object& loc, const std::string& attr_name);

// =============================================================================
// Sparse matrix common utilities
// =============================================================================

using H5File_p = std::unique_ptr<H5::H5File>;
using DataSet_p = std::unique_ptr<H5::DataSet>;

enum class SparseMatrixType : std::int32_t {
    CSR,
    CSC,
    UNKNOWN,
};

std::string sparseMatrixTypeToString(const SparseMatrixType& type);
SparseMatrixType sparseMatrixStringToType(const std::string& type);

struct SparseMatrixData {
    SparseMatrixData();
    ~SparseMatrixData();
    void reset();

    std::string _filename = "";

    H5File_p _file;
    DataSet_p _indptr_ds;
    DataSet_p _indices_ds;
    DataSet_p _data_ds;

    std::int64_t _num_rows = 0;
    std::int64_t _num_cols = 0;
    std::vector<std::int64_t> _indptr = {};              // Column pointers (size = num_cols + 1)

    std::vector<std::string> _obs_names = {};
    std::vector<std::string> _var_names = {};
};

class SparseMatrixReader {
    using Cache = std::unordered_map<int, std::pair<std::vector<float>, std::list<int>::iterator>>;

public:
    SparseMatrixReader(SparseMatrixType type) : _type(type) {}
    virtual ~SparseMatrixReader() {}

public: // Utility
    static SparseMatrixType readMatrixType(const std::string& filename);

public: // Setup

    void setUseCache(const bool useCache) { _useCache = useCache; }
    void setMaxCacheSize(const size_t newSize);
    bool readFile(const std::string& filename);
    void reset(const bool keepType = true);

public: // Getter

    std::vector<float> getRow(int row_idx);
    std::vector<float> getColumn(int col_idx);

    virtual std::vector<float> getRowImpl(int row_idx) const = 0;
    virtual std::vector<float> getColumnImpl(int col_idx) const = 0;

    bool hasObsNames() const { return !_data._obs_names.empty(); }
    bool hasVarNames() const { return !_data._var_names.empty(); }

    const std::vector<std::string>& getObsNames() const { return _data._obs_names; }
    const std::vector<std::string>& getVarNames() const { return _data._var_names; }

    int getNumRows() const { return _data._num_rows; }
    int getNumCols() const { return _data._num_cols; }

    SparseMatrixType getType() const { return _type; }
    std::string getTypeString() const { return sparseMatrixTypeToString(_type); }

    const SparseMatrixData& getRawData() const { return _data; }

    bool getUseCache() const { return _useCache; }
    size_t getMaxCacheSize() const { return _maxCacheSize; }

private:
    std::optional<std::vector<float>*> lookupCache(Cache& cache, std::list<int>& order, int id) const;
    void saveToCache(Cache& cache, std::list<int>& order, int id, const std::vector<float>& data) const;
    void removeLeastRecentlyUsed(Cache& cache, std::list<int>& order) const;

protected:
    SparseMatrixData    _data = {};
    SparseMatrixType    _type = SparseMatrixType::UNKNOWN;

    size_t              _maxCacheSize = 10;

    bool                _useCache = true;
    std::list<int>      _lookupOrderRows = {}; // Most recently used at front
    Cache               _cacheRows = {};
    std::list<int>      _lookupOrderColumns = {}; // Most recently used at front
    Cache               _cacheColumns = {};
};

bool readMatrixFromFile(const std::string& filename, SparseMatrixData& data);

// =============================================================================
// CSRReader
// =============================================================================

/*
Reads CSR matrices from .h5 files created like this:
```python
def save_h5(data: ad.AnnData, filename: str | Path):
    data_csr = data.X.to_memory()
    data_string_dt = h5py.string_dtype(encoding='utf-8')
    with h5py.File(filename, 'w') as f:
        f.attrs['format'] = 'CSR'
        f.create_dataset('data', data=data_csr.data)
        f.create_dataset('indices', data=data_csr.indices)
        f.create_dataset('indptr', data=data_csr.indptr)
        f.create_dataset('shape', data=data_csr.shape)
        f.create_dataset('obs_names', data=data.obs_names.to_numpy(), dtype=data_string_dt)
        f.create_dataset('var_names', data=data.var_names.to_numpy(), dtype=data_string_dt)
    del data_csr
```
obs_names and var_names are optional fields
*/
class CSRReader : public SparseMatrixReader
{

public:
    CSRReader();
    CSRReader(const std::string& filename);

    ~CSRReader();

public: // Getter

    std::vector<float> getRowImpl(int row_idx) const override;
    std::vector<float> getColumnImpl(int col_idx) const override;
};

// =============================================================================
// CSCReader
// =============================================================================

/*
Reads CSC matrices from .h5 files created like this:
```python
def save_h5(data: ad.AnnData, filename: str | Path):
    from scipy.sparse import csc_matrix, csr_matrix
    data_csr = data.X.to_memory()
    data_csc = data_csr.tocsc()
    del data_csr
    data_string_dt = h5py.string_dtype(encoding='utf-8')
    with h5py.File(filename, 'w') as f:
        f.attrs['format'] = 'CSC'
        f.create_dataset('data', data=data_csc.data)
        f.create_dataset('indices', data=data_csc.indices)
        f.create_dataset('indptr', data=data_csc.indptr)
        f.create_dataset('shape', data=data_csc.shape)
        f.create_dataset('obs_names', data=data.obs_names.to_numpy(), dtype=data_string_dt)
        f.create_dataset('var_names', data=data.var_names.to_numpy(), dtype=data_string_dt)
    del data_csc
```
obs_names and var_names are optional fields
*/
class CSCReader : public SparseMatrixReader
{

public:
    CSCReader();
    CSCReader(const std::string& filename);

    ~CSCReader();

public: // Getter

    std::vector<float> getRowImpl(int row_idx) const override;
    std::vector<float> getColumnImpl(int col_idx) const override;
};
