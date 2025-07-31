#pragma once

#include <string>
#include <vector>

#include <memory>

namespace H5 {
    class H5File;
    class DataSet;
}

// =============================================================================
// Sparse matrix common utilities
// =============================================================================

using H5File_p = std::unique_ptr<H5::H5File>;
using DataSet_p = std::unique_ptr<H5::DataSet>;

// Sparse matrix reader interface
class SparseMatrixReader {

public:
    virtual ~SparseMatrixReader() {}

public: // Setup

    virtual void readFile(const std::string& filename) = 0;
    virtual void reset() = 0;

public: // Getter

    virtual std::vector<float> getRow(int row_idx) const = 0;
    virtual std::vector<float> getColumn(int col_idx) const = 0;

    virtual const std::vector<std::string>& getObsNames() const = 0;
    virtual const std::vector<std::string>& getVarNames() const = 0;

    virtual int getNumRows() const = 0;
    virtual int getNumCols() const = 0;
};

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

public: // Setup

    void readFile(const std::string& filename) override;
    void reset() override;

public: // Getter

    std::vector<float> getRow(int row_idx) const override;
    std::vector<float> getColumn(int col_idx) const override;

    const std::vector<std::string>& getObsNames() const override { return _data._obs_names; }
    const std::vector<std::string>& getVarNames() const override { return _data._var_names; }

    int getNumRows() const override { return _data._num_rows; }
    int getNumCols() const override { return _data._num_cols; }

private:
    SparseMatrixData    _data = {};

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
    using H5File_p = std::unique_ptr<H5::H5File>;
    using DataSet_p = std::unique_ptr<H5::DataSet>;

public:
    CSCReader();
    CSCReader(const std::string& filename);

    ~CSCReader();

public: // Setup

    void readFile(const std::string& filename) override;
    void reset() override;

public: // Getter

    std::vector<float> getRow(int row_idx) const override;
    std::vector<float> getColumn(int col_idx) const override;

    const std::vector<std::string>& getObsNames() const override { return _data._obs_names; }
    const std::vector<std::string>& getVarNames() const override { return _data._var_names; }

    int getNumRows() const override { return _data._num_rows; }
    int getNumCols() const override { return _data._num_cols; }

private:
    SparseMatrixData    _data = {};

};
