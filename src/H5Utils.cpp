#include "H5Utils.h"

#include <hdf5.h>

#include <array>
#include <iostream>

SparseMatrixReader::SparseMatrixReader(const std::string& filename) {
    readFile(filename);
}

SparseMatrixReader::~SparseMatrixReader() {
    closeCurrentFile();
}

void SparseMatrixReader::closeCurrentFile() const
{
    H5Dclose(_data_id);
    H5Dclose(_indices_id);
    H5Dclose(_indptr_id);
    H5Fclose(_file_id);
}

void SparseMatrixReader::reset()
{
    closeCurrentFile();

    _filename   = {};
    _data_id    = {};
    _indices_id = {};
    _indptr_id  = {};
    _file_id    = {};
    _num_rows   = 0;
    _num_cols   = 0;
    _indptr     = {};
}

void SparseMatrixReader::readFile(const std::string& filename)
{
    if (H5Iis_valid(_file_id) > 0) {
        reset();
    }

    _file_id = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);

    if (auto file_id_valid = H5Iis_valid(_file_id) > 0) {
        std::cout << "SparseMatrixReader::readFile: file open" << std::endl;
    }
    else if (!file_id_valid) {
        std::cout << "SparseMatrixReader::readFile: file NOT open" << std::endl;
    }
    else {
        std::cout << "SparseMatrixReader::readFile: unexptected error" << std::endl;
    }

    // Read shape
    hid_t shape_dataset = H5Dopen2(_file_id, "/shape", H5P_DEFAULT);
    std::array<int, 2> shape = {};
    H5Dread(shape_dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, shape.data());
    _num_rows = shape[0];
    _num_cols = shape[1];
    H5Dclose(shape_dataset);

    // Open datasets (but don't read data yet)
    _data_id    = H5Dopen2(_file_id, "/data", H5P_DEFAULT);
    _indices_id = H5Dopen2(_file_id, "/indices", H5P_DEFAULT);
    _indptr_id  = H5Dopen2(_file_id, "/indptr", H5P_DEFAULT);

    // Read indptr array (small, need for row access)
    hsize_t indptr_size = {};
    hid_t indptr_space = H5Dget_space(_indptr_id);
    H5Sget_simple_extent_dims(indptr_space, &indptr_size, NULL);
    _indptr.resize(indptr_size);
    H5Dread(_indptr_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, _indptr.data());
    H5Sclose(indptr_space);
}

std::vector<float>SparseMatrixReader::getRow(int row_idx) const {
    std::vector<float> dense_row(_num_cols, 0.0f);

    int start = _indptr[row_idx];
    int end = _indptr[row_idx + 1];
    int row_nnz = end - start;

    if (row_nnz == 0) {
        return dense_row;  // Empty row
    }

    // Read only the data for this row
    std::vector<float> row_data(row_nnz);
    std::vector<int> row_indices(row_nnz);

    // Create hyperslab for data array
    hid_t data_space = H5Dget_space(_data_id);
    hsize_t data_start = start;
    hsize_t data_count = row_nnz;
    H5Sselect_hyperslab(data_space, H5S_SELECT_SET, &data_start, NULL, &data_count, NULL);

    // Create memory space
    hid_t mem_space = H5Screate_simple(1, &data_count, NULL);

    // Read data slice
    H5Dread(_data_id, H5T_NATIVE_FLOAT, mem_space, data_space, H5P_DEFAULT, row_data.data());

    // Create hyperslab for indices array
    hid_t indices_space = H5Dget_space(_indices_id);
    H5Sselect_hyperslab(indices_space, H5S_SELECT_SET, &data_start, NULL, &data_count, NULL);

    // Read indices slice
    H5Dread(_indices_id, H5T_NATIVE_INT, mem_space, indices_space, H5P_DEFAULT, row_indices.data());

    // Clean up
    H5Sclose(data_space);
    H5Sclose(indices_space);
    H5Sclose(mem_space);

    // Populate dense row
#pragma omp parallel for
    for (int i = 0; i < row_nnz; ++i) {
        dense_row[row_indices[i]] = row_data[i];
    }

    return dense_row;
}
