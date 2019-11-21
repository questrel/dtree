////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// templates for matrices
// Levels of index are pealed away one at a time (matrix_<n>_t[i] returns a matrix_<n-1>_t)
// Avoids index calculations by precomputing submatrix addresses.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <valarray>

// two dimensions
template<typename T>
class matrix_2_t {
  typedef size_t size_type;
  size_type dim1;
  std::valarray<T *> submatrices;
  std::valarray<T> storage;
public:
  matrix_2_t() {}
  matrix_2_t(size_type dim1_, size_type dim2_, T fill_) { assign(dim1_, dim2_, fill_); }
  matrix_2_t(size_type dim1_, size_type dim2_, T *begin_, T *end_) { assign(dim1_, dim2_, begin_, end_); }
  size_type n_rows() { return dim1; }
  size_type n_colums() { return storage.size() / dim1; }
  void assign(size_type dim1_, size_type dim2_, T *begin_, T *end) {
    dim1 = dim1_;
    submatrices.resize(dim1, 0);
    for (size_type i = 0; i < dim1; ++i)
      submatrices[i] = begin_ + i * dim2_;
  }
  void assign(size_type dim1_, size_type dim2_, T fill_) { 
    storage.resize(dim1_ * dim2_, fill_);
    assign(dim1_, dim2_, &storage[0], &storage[dim1_ * dim2_]);
  }
  T *operator[](size_type i) { return submatrices[i]; }
  matrix_2_t &operator+=(const matrix_2_t &m) { my_assert(dim1 == m.dim1 && storage.size() == m.storage.size()); storage += m.storage; return *this; }
};

// three dimensions
template<typename T>
class matrix_3_t {
  typedef size_t size_type;
  size_type dim1;
  std::valarray<matrix_2_t<T>> submatrices;
public:
  matrix_3_t() {}
  matrix_3_t(size_type dim1_, size_type dim2_, size_type dim3_, T fill_) { assign(dim1_, dim2_, dim3_, fill_); }
  matrix_3_t(size_type dim1_, size_type dim2_, size_type dim3_, T *begin_, T *end) { assign(dim1_, dim2_, dim3_, begin_, end_); }
  void assign(size_type dim1_, size_type dim2_, size_type dim3_, T *begin_, T *end_) {
    dim1 = dim1_;
    submatrices.resize(dim1, matrix_2_t<T>());
    for (size_type i = 0; i < dim1; ++i)
      submatrices[i].assign(dim2_, dim3_, begin_ + i * (dim2_ * dim3_), begin_ + (i + 1) * (dim2_ * dim3_));
  }
  void assign(size_type dim1_, size_type dim2_, size_type dim3_, T fill_) {
    dim1 = dim1_;
    submatrices.assign(dim1, matrix_2_t<T>());
    for (size_type i = 0; i < dim1; ++i)
      submatrices[i].assign(dim2_, dim3_, fill_);
  }
  matrix_2_t<T> &operator[](size_type i) { return submatrices[i]; }
};

