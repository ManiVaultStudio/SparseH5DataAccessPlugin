# Sparse H5 data access plugin

Analysis plugin for the [ManiVault](https://github.com/ManiVaultStudio/core) which populates point data selectively from sparse matrices stored on disk using [hdf5](https://github.com/HDFGroup/hdf5/).

Clone the repo:
```
git clone https://github.com/ManiVaultStudio/SparseH5DataAccessPlugin.git
```

You can also install HDF5 with [vcpkg](https://github.com/microsoft/vcpkg) and use `-DCMAKE_TOOLCHAIN_FILE="[YOURPATHTO]/vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static-md` to point to your vcpkg installation:
```bash
./vcpkg install hdf5[core,cpp,zlib]:x64-windows-static-md
```
Depending on your OS the `VCPKG_TARGET_TRIPLET` might vary, e.g. for linux you probably don't need to specify any since it automatically builds static libraries.
