#include "H5Utils.h"

#include <H5Cpp.h>

#include <array>
#include <iostream>

SparseMatrixReader::SparseMatrixReader() :
    _file(std::make_unique<H5::H5File>()),
    _indptr_ds(std::make_unique<H5::DataSet>()),
    _indices_ds(std::make_unique<H5::DataSet>()),
    _data_ds(std::make_unique<H5::DataSet>())
{
}

SparseMatrixReader::SparseMatrixReader(const std::string& filename) : 
    SparseMatrixReader()
{
    readFile(filename);
}

SparseMatrixReader::~SparseMatrixReader() = default;

void SparseMatrixReader::reset()
{
    _filename   = "";
    _file       = {};
    _data_ds    = {};
    _indices_ds = {};
    _indptr_ds  = {};
    _num_rows   = 0;
    _num_cols   = 0;
    _indptr     = {};
    _obs_names  = {};
    _var_names  = {};
}

void SparseMatrixReader::readFile(const std::string& filename)
{
    if (_file || _filename.empty()) {
        reset();
    }

    try {
        _filename = filename;
        _file = std::make_unique<H5::H5File>(_filename, H5F_ACC_RDONLY);

        // Read shape
        H5::DataSet shape_ds = _file->openDataSet("/shape");
        std::array<int, 2> shape{};
        shape_ds.read(shape.data(), H5::PredType::NATIVE_INT);
        _num_rows = shape[0];
        _num_cols = shape[1];

        // Open datasets (but don't read data yet)
        _data_ds    = std::make_unique<H5::DataSet>(_file->openDataSet("/data"));
        _indices_ds = std::make_unique<H5::DataSet>(_file->openDataSet("/indices"));
        _indptr_ds  = std::make_unique<H5::DataSet>(_file->openDataSet("/indptr"));

        // Read indptr array (small, need for row access)
        H5::DataSpace indptr_space = _indptr_ds->getSpace();
        hsize_t indptr_size = {};
        indptr_space.getSimpleExtentDims(&indptr_size);
        _indptr.resize(indptr_size);
        _indptr_ds->read(_indptr.data(), H5::PredType::NATIVE_INT);

        // Read variable and observation names (if available)
        auto readStringArray = [this](const std::string& datasetName, std::vector<std::string>& dest) -> void {

            if (!_file->nameExists(datasetName)) {
                dest.clear();
                return;
            }

            H5::DataSet obs_ds = _file->openDataSet(datasetName);
            H5::DataSpace obs_space = obs_ds.getSpace();
            hsize_t n = {};
            obs_space.getSimpleExtentDims(&n);

            // Variable-length UTF-8 string type
            H5::StrType str_type(H5::PredType::C_S1, H5T_VARIABLE);
            str_type.setCset(H5T_CSET_UTF8);

            std::vector<char*> obs_raw(n);
            obs_ds.read(obs_raw.data(), str_type);

            dest.clear();
            dest.reserve(n);
            for (size_t i = 0; i < n; ++i) {
                dest.emplace_back(obs_raw[i]);
                free(obs_raw[i]); // Free HDF5-allocated memory
            }

            };

        readStringArray("/obs_names", _obs_names);
        readStringArray("/var_names", _var_names);
    }
    catch (H5::FileIException& e) {
        std::cerr << "HDF5 file error: " << e.getDetailMsg() << std::endl;
    }
    catch (H5::DataSetIException& e) {
        std::cerr << "HDF5 dataset error: " << e.getDetailMsg() << std::endl;
    }
    catch (H5::DataSpaceIException& e) {
        std::cerr << "HDF5 dataspace error: " << e.getDetailMsg() << std::endl;
    }
    catch (H5::DataTypeIException& e) {
        std::cerr << "HDF5 datatype error: " << e.getDetailMsg() << std::endl;
    }
    catch (const H5::Exception& e) {
        std::cerr << "HDF5 error: " << e.getDetailMsg() << std::endl;
    }
}

std::vector<float>SparseMatrixReader::getRow(int row_idx) const {
    std::vector<float> dense_row(_num_cols, 0.0f);

    if (!_data_ds || !_indices_ds) {
        return dense_row;  // invalid data sets
    }

    const int start   = _indptr[row_idx];
    const int end     = _indptr[row_idx + 1];
    const int row_nnz = end - start;

    if (row_nnz == 0) {
        return dense_row;  // Empty row
    }

    try {
        // Define hyperslab parameters
        hsize_t offset = start;
        hsize_t count = row_nnz;

        // Read data slice
        H5::DataSpace data_space = _data_ds->getSpace();
        data_space.selectHyperslab(H5S_SELECT_SET, &count, &offset);

        H5::DataSpace mem_space(1, &count);
        std::vector<float> row_data(row_nnz);
        _data_ds->read(row_data.data(), H5::PredType::NATIVE_FLOAT, mem_space, data_space);

        // Read indices slice
        H5::DataSpace indices_space = _indices_ds->getSpace();
        indices_space.selectHyperslab(H5S_SELECT_SET, &count, &offset);

        std::vector<int> row_indices(row_nnz);
        _indices_ds->read(row_indices.data(), H5::PredType::NATIVE_INT, mem_space, indices_space);

        // Populate dense row
#pragma omp parallel for
        for (std::int64_t i = 0; i < static_cast<std::int64_t>(row_nnz); ++i) {
            dense_row[row_indices[i]] = row_data[i];
        }
    }
    catch (const H5::Exception& e) {
        std::cerr << "Error reading row " << row_idx << ": " << e.getDetailMsg() << std::endl;
    }

    return dense_row;
}

std::vector<float> SparseMatrixReader::getColumn(int col_idx) const {
    std::vector<float> dense_col(_num_rows, 0.0f);

    if (!_data_ds || !_indices_ds || col_idx < 0 || col_idx >= _num_cols) {
        return dense_col;  // invalid datasets or column index
    }

    try {
        // We need to scan through all rows to find entries in the requested column
        for (int row = 0; row < _num_rows; ++row) {
            const int start = _indptr[row];
            const int end = _indptr[row + 1];
            const int row_nnz = end - start;

            if (row_nnz == 0) {
                continue;  // Empty row, skip
            }

            // Define hyperslab parameters for this row
            hsize_t offset = start;
            hsize_t count = row_nnz;

            // Read indices slice for this row
            H5::DataSpace indices_space = _indices_ds->getSpace();
            indices_space.selectHyperslab(H5S_SELECT_SET, &count, &offset);

            H5::DataSpace mem_space(1, &count);
            std::vector<int> row_indices(row_nnz);
            _indices_ds->read(row_indices.data(), H5::PredType::NATIVE_INT, mem_space, indices_space);

            // Check if this row has an entry at the requested column
            for (int i = 0; i < row_nnz; ++i) {
                if (row_indices[i] == col_idx) {
                    // Found the column! Now read the corresponding data value
                    hsize_t data_offset = start + i;
                    hsize_t data_count = 1;

                    H5::DataSpace data_space = _data_ds->getSpace();
                    data_space.selectHyperslab(H5S_SELECT_SET, &data_count, &data_offset);

                    H5::DataSpace scalar_space(1, &data_count);
                    float value;
                    _data_ds->read(&value, H5::PredType::NATIVE_FLOAT, scalar_space, data_space);

                    dense_col[row] = value;
                    break;  // Found the entry for this row, move to next row
                }
            }
        }
    }
    catch (const H5::Exception& e) {
        std::cerr << "Error reading column " << col_idx << ": " << e.getDetailMsg() << std::endl;
    }

    return dense_col;
}