#pragma once

#include <string>
#include <vector>

#include <H5Cpp.h>

/*
Reads .h5 files creates like this:
```python
def save_h5(data: ad.AnnData, filename: str | Path):
    data_sparse_in_mem = data.X.to_memory()
    data_string_dt = h5py.string_dtype(encoding='utf-8')
    with h5py.File(filename, 'w') as f:
        f.create_dataset('data', data=data_sparse_in_mem.data)
        f.create_dataset('indices', data=data_sparse_in_mem.indices)
        f.create_dataset('indptr', data=data_sparse_in_mem.indptr)
        f.create_dataset('shape', data=data_sparse_in_mem.shape)
        f.create_dataset('obs_names', data=data.obs_names.to_numpy(), dtype=data_string_dt)
        f.create_dataset('var_names', data=data.var_names.to_numpy(), dtype=data_string_dt)
    del data_sparse_in_mem
```
obs_names and var_names are optional fields
*/
class SparseMatrixReader {

public:
    SparseMatrixReader() {};
    SparseMatrixReader(const std::string& filename);

    ~SparseMatrixReader();

    void readFile(const std::string& filename);
    void closeCurrentFile() const;
    void reset();

public: // Getter

    std::vector<float> getRow(int row_idx) const;

    int getNumRows() const { return _num_rows; }
    int getNumCols() const { return _num_cols; }

private:
    std::string _filename               = {};

    H5::H5File _file                    = {};
    H5::DataSet _indptr_ds              = {};
    H5::DataSet _indices_ds             = {};
    H5::DataSet _data_ds                = {};

    int _num_rows                       = 0;
    int _num_cols                       = 0;
    std::vector<int> _indptr            = {};

    std::vector<std::string> _obs_names = {};
    std::vector<std::string> _var_names = {};

};
