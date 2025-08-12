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
import anndata as ad
import numpy as np
import h5py
from scipy import sparse as sp

def save_h5(sparse_mat: sp.csr_matrix | sp.csc_matrix, filename: str | Path, storage_type: str, var_names = None, obs_names = None):
    """
    Save sparse matrices in CSC (Compressed Sparse Column) or CSR (Compressed Sparse Row) format to HDF5.
    """
    if storage_type.lower() == 'csr_matrix' and not isinstance(sparse_mat, sp.csr_matrix):
        raise TypeError('Data is not CSR.')
    if storage_type.lower() == 'csc_matrix' and not isinstance(sparse_mat, sp.csc_matrix):
        raise TypeError('Data is not CSC.')

    data_string_dt = h5py.string_dtype(encoding='utf-8')

    if not isinstance(filename, Path):
        filename = Path(filename)

    filename.parent.mkdir(parents=True, exist_ok=True)

    with h5py.File(filename, 'w') as f:
        group_x = f.create_group("X")

        group_x.attrs['encoding-type'] = storage_type
        group_x.attrs['encoding-version'] = "0.1.0"
        group_x.attrs['shape'] = sparse_mat.shape

        group_x.create_dataset('data', data=sparse_mat.data)
        group_x.create_dataset('indices', data=sparse_mat.indices)  # row indices
        group_x.create_dataset('indptr', data=sparse_mat.indptr)  # column pointers

        # Optional metadata
        if obs_names is not None and len(obs_names) == sparse_mat.shape[0]:
            group_obs = f.create_group("obs")
            group_obs.create_dataset('_index', data=obs_names, dtype=data_string_dt)
        if var_names is not None and len(var_names) == sparse_mat.shape[1]:
            group_var = f.create_group("var")
            group_var.create_dataset('_index', data=var_names, dtype=data_string_dt)


sparse_matrix = myAnnData.X.to_memory()
save_h5(sparse_matrix, "myFile.h5", "csr_matrix")
```

## Building
You can also install [HDF5](https://github.com/HDFGroup/hdf5/) with [vcpkg](https://github.com/microsoft/vcpkg) and use `-DCMAKE_TOOLCHAIN_FILE="[YOURPATHTO]/vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static-md` to point CMake to your vcpkg installation:
```bash
./vcpkg install hdf5[core,cpp,zlib]:x64-windows-static-md
```
Depending on your OS the `VCPKG_TARGET_TRIPLET` might vary, e.g. for Linux you probably don't need to specify any triplet since it automatically defaults to building static libraries.

## Testing
Set the CMake option `MV_SH5A_BUILD_TESTS` to `ON` to build a test target. This requires [Catch2](https://github.com/catchorg/Catch2/):
```bash
./vcpkg install catch2:x64-windows-static-md
```
You'll also need to run `create_reference_data.py` in the `test` folder to create some test ground truth data.
