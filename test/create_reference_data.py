import numpy as np
import scipy.sparse as sp
import anndata as ad
import h5py
from pathlib import Path

def save_h5(data: ad.AnnData, filename: str | Path, storage_type: str):
    """
    Save AnnData sparse matrix in CSC (Compressed Sparse Column) or CSR (Compressed Sparse Row) format to HDF5.

    CSC format stores:
    - data: non-zero values
    - indices: row/column indices for each non-zero value
    - indptr: pointers to start of each column/row in data/indices arrays
    - storage_type: sparse matrix storage type
    """
    if storage_type.upper() == 'CSR' and not isinstance(data.X, sp.csr_matrix):
        raise TypeError('Data is not CSR.')
    if storage_type.upper() == 'CSC' and not isinstance(data.X, sp.csc_matrix):
        raise TypeError('Data is not CSC.')

    data_string_dt = h5py.string_dtype() # encoding='utf-8'

    if not isinstance(filename, Path):
        filename = Path(filename)

    filename.parent.mkdir(parents=True, exist_ok=True)

    with h5py.File(filename, 'w') as f:
        f.attrs['format'] = storage_type
        f.create_dataset('data', data=data.X.data)
        f.create_dataset('indices', data=data.X.indices)  # row indices
        f.create_dataset('indptr', data=data.X.indptr)  # column pointers
        f.create_dataset('shape', data=data.X.shape)

        # Optional metadata
        if hasattr(data, 'obs_names') and len(data.obs_names) == data.n_obs:
            f.create_dataset('obs_names', data=data.obs_names.to_numpy(), dtype=data_string_dt)
        if hasattr(data, 'var_names') and len(data.var_names) == data.n_vars:
            f.create_dataset('var_names', data=data.var_names.to_numpy(), dtype=data_string_dt)

# Generate random sparse matrix in COO format
n_obs, n_vars = 5, 4                                        # observations × variables
row = np.array([0, 1, 2, 3, 0, 4, 2])                       # row indices
col = np.array([1, 2, 0, 3, 2, 3, 3])                       # column indices
data = np.array([10.0, 20.2, 30.4, 40.6, 50, 60, 70])       # non-zero values
coo = sp.coo_matrix((data, (row, col)), shape=(n_obs, n_vars), dtype=np.float32)

print(coo)
print(coo.todense())

# Convert to sparse formats
csc_data = coo.tocsc()
csr_data = coo.tocsr()

# Create AnnData objects
adata_csc = ad.AnnData(X=csc_data)
adata_csr = ad.AnnData(X=csr_data)

adata_csr.obs_names = [f"obs{i}" for i in range(n_obs)]
adata_csr.var_names = [f"var{j}" for j in range(n_vars)]

# Print info to confirm
print("CSC-format AnnData:")
print(adata_csc)

print("\nCSR-format AnnData:")
print(adata_csr)

save_h5(adata_csc, './data/csc.h5', 'csc')
save_h5(adata_csr, './data/csr.h5', 'csr')

