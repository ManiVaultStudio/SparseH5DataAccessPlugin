#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// =============================================================================
// H5 utilities
// =============================================================================

namespace H5 {
    class H5File;
    class DataSet;
}

std::string readAttributeString(H5::H5File& file, const std::string& attr_name);

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

    int _num_rows = 0;
    int _num_cols = 0;
    std::vector<int> _indptr = {};              // Column pointers (size = num_cols + 1)

    std::vector<std::string> _obs_names = {};
    std::vector<std::string> _var_names = {};
};

class SparseMatrixReader {

public:
    SparseMatrixReader(SparseMatrixType type) : _type(type) {}
    virtual ~SparseMatrixReader() {}

public: // Utility
    static SparseMatrixType readMatrixType(const std::string& filename);

public: // Setup

    void readFile(const std::string& filename);
    void reset() { _data.reset(); };

public: // Getter

    virtual std::vector<float> getRow(int row_idx) const = 0;
    virtual std::vector<float> getColumn(int col_idx) const = 0;

    const std::vector<std::string>& getObsNames() const { return _data._obs_names; }
    const std::vector<std::string>& getVarNames() const { return _data._var_names; }

    int getNumRows() const { return _data._num_rows; }
    int getNumCols() const { return _data._num_cols; }

    SparseMatrixType getType() const { return _type; }
    std::string getTypeString() const { return sparseMatrixTypeToString(_type); }

protected:
    SparseMatrixData    _data = {};
    SparseMatrixType    _type = SparseMatrixType::UNKNOWN;
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

    std::vector<float> getRow(int row_idx) const override;
    std::vector<float> getColumn(int col_idx) const override;
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

    std::vector<float> getRow(int row_idx) const override;
    std::vector<float> getColumn(int col_idx) const override;
};
