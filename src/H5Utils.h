#pragma once

#include <string>
#include <vector>

#include <H5Ipublic.h>

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
    std::string _filename    = {};

    hid_t _file_id           = {};
    hid_t _indptr_id         = {};
    hid_t _indices_id        = {};
    hid_t _data_id           = {};

    int _num_rows            = 0;
    int _num_cols            = 0;
    std::vector<int> _indptr = {};
};
