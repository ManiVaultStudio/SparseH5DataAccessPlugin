#include "H5Utils.h"

#include <H5Cpp.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <stdlib.h> // free

// =============================================================================
// H5 utilities
// =============================================================================

std::string readAttributeString(H5::H5File& file, const std::string& attr_name) {
    std::string value = "";

    try {
        const H5::Attribute attr   = file.openAttribute(attr_name);
        const H5::StrType str_type = attr.getStrType();
                
        attr.read(str_type, value);
    }
    catch (const H5::Exception& e) {
        std::cerr << "Error reading attribute '" << attr_name << "': " << e.getDetailMsg() << std::endl;
    }

    return value;
}

// =============================================================================
// Sparse matrix common utilities
// =============================================================================

std::string sparseMatrixTypeToString(const SparseMatrixType& type)
{
    std::string typeStr = "";

    switch (type)
    {
    case SparseMatrixType::CSR:
        typeStr = "CSR";
        break;
    case SparseMatrixType::CSC:
        typeStr = "CSC";
        break;
    case SparseMatrixType::UNKNOWN:
        typeStr = "UNKNOWN";
        break;
    }

    return typeStr;
}

SparseMatrixType sparseMatrixStringToType(const std::string& typeStr)
{
    SparseMatrixType type = SparseMatrixType::UNKNOWN;

    auto str_toupper = [](std::string str) -> std::string {
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::toupper(c);});
        return str;
        };

    const std::string typeStrUpperCase = str_toupper(typeStr);

    if (typeStrUpperCase == "CSR")
        type = SparseMatrixType::CSR;

    if (typeStrUpperCase == "CSC")
        type = SparseMatrixType::CSC;

    return type;
}

bool readMatrixFromFile(const std::string& filename, SparseMatrixData& data)
{
    if (!std::filesystem::exists(filename)) {
        return false;
    }

    try {
        data._filename = filename;
        data._file = std::make_unique<H5::H5File>(data._filename, H5F_ACC_RDONLY);

        // Read shape
        H5::DataSet shape_ds = data._file->openDataSet("/shape");
        std::array<int, 2> shape{};
        shape_ds.read(shape.data(), H5::PredType::NATIVE_INT);
        data._num_rows = shape[0];
        data._num_cols = shape[1];

        // Open datasets (but don't read data yet)
        data._data_ds = std::make_unique<H5::DataSet>(data._file->openDataSet("/data"));
        data._indices_ds = std::make_unique<H5::DataSet>(data._file->openDataSet("/indices"));
        data._indptr_ds = std::make_unique<H5::DataSet>(data._file->openDataSet("/indptr"));

        // Read indptr array (small, need for row access)
        H5::DataSpace indptr_space = data._indptr_ds->getSpace();
        hsize_t indptr_size = {};
        indptr_space.getSimpleExtentDims(&indptr_size);
        data._indptr.resize(indptr_size);
        data._indptr_ds->read(data._indptr.data(), H5::PredType::NATIVE_INT);

        // Read variable and observation names (if available)
        auto readStringArray = [&data](const std::string& datasetName, std::vector<std::string>& dest) -> void {

            if (!data._file->nameExists(datasetName)) {
                dest.clear();
                return;
            }

            H5::DataSet obs_ds = data._file->openDataSet(datasetName);
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

        readStringArray("/obs_names", data._obs_names);
        readStringArray("/var_names", data._var_names);
    }
    catch (H5::FileIException& e) {
        std::cerr << "HDF5 file error: " << e.getDetailMsg() << std::endl;
        return false;
    }
    catch (H5::DataSetIException& e) {
        std::cerr << "HDF5 dataset error: " << e.getDetailMsg() << std::endl;
        return false;
    }
    catch (H5::DataSpaceIException& e) {
        std::cerr << "HDF5 dataspace error: " << e.getDetailMsg() << std::endl;
        return false;
    }
    catch (H5::DataTypeIException& e) {
        std::cerr << "HDF5 datatype error: " << e.getDetailMsg() << std::endl;
        return false;
    }
    catch (const H5::Exception& e) {
        std::cerr << "HDF5 error: " << e.getDetailMsg() << std::endl;
        return false;
    }

    return true;
}

SparseMatrixData::SparseMatrixData() :
    _file(std::make_unique<H5::H5File>()),
    _indptr_ds(std::make_unique<H5::DataSet>()),
    _indices_ds(std::make_unique<H5::DataSet>()),
    _data_ds(std::make_unique<H5::DataSet>())
{
}

SparseMatrixData::~SparseMatrixData() = default;

void SparseMatrixData::reset()
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

SparseMatrixType SparseMatrixReader::readMatrixType(const std::string& filename) {
    H5::H5File file = H5::H5File(filename, H5F_ACC_RDONLY);
    std::string format = readAttributeString(file, "format");

    return sparseMatrixStringToType(format);
}

bool SparseMatrixReader::readFile(const std::string& filename)
{
    if (_data._file || !_data._filename.empty()) {
        reset();
    }

    return readMatrixFromFile(filename, _data);
}

void SparseMatrixReader::reset(const bool keepType) {
    _data.reset(); 
    _lookupOrderRows.clear();
    _cacheRows.clear();
    _lookupOrderColumns.clear();
    _cacheColumns.clear();
    _maxCacheSize = 10;
    _useCache = true;

    if (!keepType) {
        _type = SparseMatrixType::UNKNOWN;
    }
};

void SparseMatrixReader::setMaxCacheSize(const size_t newSize) {
    if (newSize == _maxCacheSize)
        return;

    _maxCacheSize = newSize;
    removeLeastRecentlyUsed(_cacheRows, _lookupOrderRows);
    removeLeastRecentlyUsed(_cacheColumns, _lookupOrderColumns);

    assert(_cacheRows.size() <= _maxCacheSize);
    assert(_cacheColumns.size() <= _maxCacheSize);
}

void SparseMatrixReader::removeLeastRecentlyUsed(Cache& cache, std::list<int>& order) const {
    assert(order.size() == cache.size());

    if (cache.empty())
        return;

    const int leastRecentID = order.back();
    order.pop_back();
    cache.erase(leastRecentID);

    assert(order.size() == cache.size());
}

std::optional<std::vector<float>*> SparseMatrixReader::lookupCache(Cache& cache, std::list<int>& order, int id) const {
    if (!_useCache)
        return std::nullopt;

    auto it = cache.find(id);
    if (it != cache.cend()) {
        // Move row_idx to front to mark as most recently used
        order.erase(it->second.second);
        order.push_front(id);
        it->second.second = order.begin();
        return &(it->second.first);
    }

    return std::nullopt;
}

void SparseMatrixReader::saveToCache(Cache& cache, std::list<int>& order, int id, const std::vector<float>& data) const {
    if (!_useCache)
        return;

    if (cache.size() >= _maxCacheSize) {
        removeLeastRecentlyUsed(cache, order);
    }

    // Insert new item
    order.push_front(id);
    cache[id] = { data, order.begin() };
}

std::vector<float> SparseMatrixReader::getRow(int row_idx) {
    // Check cache
    const auto cacheResult = lookupCache(_cacheRows, _lookupOrderRows, row_idx);

    if (cacheResult.has_value() && cacheResult.value() != nullptr) {
        return *(cacheResult.value());
    }

    // Otherwise, fetch data
    std::vector<float> data = getRowImpl(row_idx);

    // Add to cache
    saveToCache(_cacheRows, _lookupOrderRows, row_idx, data);

    return data;
}

std::vector<float> SparseMatrixReader::getColumn(int col_idx) {
    // Check cache
    const auto cacheResult = lookupCache(_cacheColumns, _lookupOrderColumns, col_idx);

    if (cacheResult.has_value() && cacheResult.value() != nullptr) {
        return *(cacheResult.value());
    }

    // Otherwise, fetch data
    std::vector<float> data = getColumnImpl(col_idx);

    // Add to cache
    saveToCache(_cacheColumns, _lookupOrderColumns, col_idx, data);

    return data;
}

static std::vector<float> getArrayPrimary(const SparseMatrixData& data, const int size_primary, const int size_second, const int idx) {
    std::vector<float> dense_array(size_second, 0.0f);

    if (!data._data_ds || !data._indices_ds || idx < 0 || idx >= size_primary) {
        return dense_array;  // invalid data sets
    }

    const int start = data._indptr[idx];
    const int end = data._indptr[idx + 1];
    const int arr_nnz = end - start;

    if (arr_nnz == 0) {
        return dense_array;  // Empty array
    }

    try {
        // Define hyperslab parameters
        hsize_t offset = start;
        hsize_t count = arr_nnz;

        H5::DataSpace mem_space(1, &count);

        // Read data
        H5::DataSpace data_space = data._data_ds->getSpace();
        data_space.selectHyperslab(H5S_SELECT_SET, &count, &offset);
        std::vector<float> arr_data(arr_nnz);
        data._data_ds->read(arr_data.data(), H5::PredType::NATIVE_FLOAT, mem_space, data_space);

        // Read indices
        H5::DataSpace indices_space = data._indices_ds->getSpace();
        indices_space.selectHyperslab(H5S_SELECT_SET, &count, &offset);
        std::vector<int> arr_indices(arr_nnz);
        data._indices_ds->read(arr_indices.data(), H5::PredType::NATIVE_INT, mem_space, indices_space);

        // Populate dense arr
#pragma omp parallel for
        for (std::int64_t i = 0; i < static_cast<std::int64_t>(arr_nnz); ++i) {
            assert(arr_indices[i] >= 0);
            assert(arr_indices[i] < size_second);
            dense_array[arr_indices[i]] = arr_data[i];
        }
    }
    catch (const H5::Exception& e) {
        std::cerr << "Error reading primary array " << idx << ": " << e.getDetailMsg() << std::endl;
    }

    return dense_array;
}

static std::vector<float> getArraySecondary(const SparseMatrixData& data, const int size_primary, const int size_second, const int idx) {
    std::vector<float> dense_array(size_primary, 0.0f);

    if (!data._data_ds || !data._indices_ds || idx < 0 || idx >= size_second) {
        return dense_array;  // invalid datasets or column index
    }

    try {
        // We need to scan through all arrays to find entries in the requested column
        for (int arr = 0; arr < size_primary; ++arr) {
            const int start = data._indptr[arr];
            const int end = data._indptr[arr + 1];
            const int arr_nnz = end - start;

            if (arr_nnz == 0) {
                continue;  // Empty arr, skip
            }

            // Define hyperslab parameters for this arr
            hsize_t offset = start;
            hsize_t count = arr_nnz;

            // Read indices slice for this arr
            H5::DataSpace indices_space = data._indices_ds->getSpace();
            indices_space.selectHyperslab(H5S_SELECT_SET, &count, &offset);

            H5::DataSpace mem_space(1, &count);
            std::vector<int> arr_indices(arr_nnz);
            data._indices_ds->read(arr_indices.data(), H5::PredType::NATIVE_INT, mem_space, indices_space);

            // Check if this arr has an entry at the requested column
            for (int i = 0; i < arr_nnz; ++i) {
                if (arr_indices[i] == idx) {
                    // Found the column! Now read the corresponding data value
                    hsize_t data_offset = start + i;
                    hsize_t data_count = 1;

                    H5::DataSpace data_space = data._data_ds->getSpace();
                    data_space.selectHyperslab(H5S_SELECT_SET, &data_count, &data_offset);

                    H5::DataSpace scalar_space(1, &data_count);
                    float value;
                    data._data_ds->read(&value, H5::PredType::NATIVE_FLOAT, scalar_space, data_space);

                    dense_array[arr] = value;
                    break;  // Found the entry for this arr, move to next arr
                }
            }
        }
    }
    catch (const H5::Exception& e) {
        std::cerr << "Error reading secondary array " << idx << ": " << e.getDetailMsg() << std::endl;
    }

    return dense_array;
}

// =============================================================================
// CSRReader
// =============================================================================

CSRReader::CSRReader() :
    SparseMatrixReader(SparseMatrixType::CSR)
{
}

CSRReader::CSRReader(const std::string& filename) : 
    CSRReader()
{
    readFile(filename);
}

CSRReader::~CSRReader() = default;

std::vector<float>CSRReader::getRowImpl(int row_idx) const
{
    return getArrayPrimary(_data, _data._num_rows, _data._num_cols, row_idx);
}

std::vector<float> CSRReader::getColumnImpl(int col_idx) const
{
    return getArraySecondary(_data, _data._num_rows, _data._num_cols, col_idx);
}

// =============================================================================
// CSCReader
// =============================================================================

CSCReader::CSCReader() :
    SparseMatrixReader(SparseMatrixType::CSC)
{
}

CSCReader::CSCReader(const std::string& filename) :
    CSCReader()
{
    readFile(filename);
}

CSCReader::~CSCReader() = default;

std::vector<float> CSCReader::getColumnImpl(int col_idx) const
{
    return getArrayPrimary(_data, _data._num_cols, _data._num_rows, col_idx);
}

std::vector<float> CSCReader::getRowImpl(int row_idx) const 
{
    return getArraySecondary(_data, _data._num_cols, _data._num_rows, row_idx);
}
