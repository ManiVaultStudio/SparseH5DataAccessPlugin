# Sparse H5 data access plugin

Analysis plugin for [ManiVault](https://github.com/ManiVaultStudio/core) which populates a point data selectively from sparse matrices stored on disk using [HDF5](https://github.com/HDFGroup/hdf5/).

Clone the repo:
```
git clone https://github.com/ManiVaultStudio/SparseH5DataAccessPlugin.git
```

## Usage
This plugin can consume sparse matrices stored in CRS and CSC layouts as `.h5` files.
Instead of loading the entire sparse matrix, only a single dimension/variable is exposed. The dimension can be changed at runtime.

Here is example code to create such H5 files with Python using [anndata](https://anndata.readthedocs.io/en/stable/) files:
```python
def save_h5(data: ad.AnnData, filename: str | Path, storage_type: str):
    """
    Save AnnData sparse matrix in CSC (Compressed Sparse Column) or CSR (Compressed Sparse Row) format to HDF5.

    CSC format stores:
    - data: non-zero values
    - indices: row/column indices for each non-zero value
    - indptr: pointers to start of each column/row in data/indices arrays
    - storage_type: sparse matrix storage type
    """
    data_in_memory = data.X.to_memory()

    if storage_type == 'CSR':
        if not isinstance(data_in_memory, csr_matrix):
            data_in_memory = data_in_memory.tocsr()
    elif storage_type == 'CSC':
        if not isinstance(data_in_memory, csc_matrix):
            data_in_memory = data_in_memory.tocsc()
    else:
        raise TypeError('Unsupported data type.')

    data_string_dt = h5py.string_dtype() # encoding='utf-8'

    with h5py.File(filename, 'w') as f:
        f.attrs['format'] = storage_type
        f.create_dataset('data', data=data_in_memory.data)
        f.create_dataset('indices', data=data_in_memory.indices)  # row indices
        f.create_dataset('indptr', data=data_in_memory.indptr)  # column pointers
        f.create_dataset('shape', data=data_in_memory.shape)

        # Optional metadata
        if hasattr(data, 'obs_names') and len(data.obs_names) == data.n_obs:
            f.create_dataset('obs_names', data=data.obs_names.to_numpy(), dtype=data_string_dt)
        if hasattr(data, 'var_names') and len(data.var_names) == data.n_vars:
            f.create_dataset('var_names', data=data.var_names.to_numpy(), dtype=data_string_dt)

    del data_in_memory
```

## Building
You can also install [HDF5](https://github.com/HDFGroup/hdf5/) with [vcpkg](https://github.com/microsoft/vcpkg) and use `-DCMAKE_TOOLCHAIN_FILE="[YOURPATHTO]/vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static-md` to point CMake to your vcpkg installation:
```bash
./vcpkg install hdf5[core,cpp,zlib]:x64-windows-static-md
```
Depending on your OS the `VCPKG_TARGET_TRIPLET` might vary, e.g. for linux you probably don't need to specify any triplet since it automatically defaults to building static libraries.
