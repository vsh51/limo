#pragma once

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace limo::numerics {

/**
 * @brief Cache-friendly dense matrix with row-major storage
 * 
 * @author Volodymyr Shpyrka
 */
template <typename T>
class Matrix {
public:
	using value_type = T;
	using size_type = std::size_t;
	using iterator = typename std::vector<T>::iterator;
	using const_iterator = typename std::vector<T>::const_iterator;

	Matrix() = default;

	Matrix(size_type rows, size_type cols, const T& value = T{}) { resize(rows, cols, value); }

	Matrix(std::initializer_list<std::initializer_list<T>> values) {
		rows_ = values.size();
		cols_ = rows_ == 0 ? 0 : values.begin()->size();
		data_.reserve(rows_ * cols_);

		for (const auto& row : values) {
			if (row.size() != cols_) {
				throw std::invalid_argument("Matrix initializer rows must have equal length");
			}
			data_.insert(data_.end(), row.begin(), row.end());
		}
	}

	size_type rows() const { return rows_; }
	size_type cols() const { return cols_; }
	size_type size() const { return data_.size(); }
	bool empty() const { return data_.empty(); }

	void resize(size_type rows, size_type cols, const T& value = T{}) {
		rows_ = rows;
		cols_ = cols;
		data_.assign(rows_ * cols_, value);
	}

	void clear() {
		rows_ = 0;
		cols_ = 0;
		data_.clear();
	}

	void fill(const T& value) { std::fill(data_.begin(), data_.end(), value); }

	T* data() { return data_.data(); }
	const T* data() const { return data_.data(); }

	T& operator()(size_type row, size_type col) { return data_[index(row, col)]; }
	const T& operator()(size_type row, size_type col) const { return data_[index(row, col)]; }

	T& at(size_type row, size_type col) {
		ensure_in_range(row, col);
		return data_[index(row, col)];
	}

	const T& at(size_type row, size_type col) const {
		ensure_in_range(row, col);
		return data_[index(row, col)];
	}

	std::span<T> row(size_type rowIndex) {
		if (rowIndex >= rows_) {
			throw std::out_of_range("Matrix row index out of range");
		}
		return {data_.data() + rowIndex * cols_, cols_};
	}

	std::span<const T> row(size_type rowIndex) const {
		if (rowIndex >= rows_) {
			throw std::out_of_range("Matrix row index out of range");
		}
		return {data_.data() + rowIndex * cols_, cols_};
	}

	Matrix transpose() const {
		Matrix result(cols_, rows_);
		for (size_type rowIndex = 0; rowIndex < rows_; ++rowIndex) {
			for (size_type colIndex = 0; colIndex < cols_; ++colIndex) {
				result(colIndex, rowIndex) = (*this)(rowIndex, colIndex);
			}
		}
		return result;
	}

	void swap_rows(size_type left, size_type right) {
		if (left >= rows_ || right >= rows_) {
			throw std::out_of_range("Matrix row index out of range");
		}
		if (left == right) {
			return;
		}
		for (size_type col = 0; col < cols_; ++col) {
			std::swap((*this)(left, col), (*this)(right, col));
		}
	}

	void scale_row(size_type rowIndex, const T& factor) {
		if (rowIndex >= rows_) {
			throw std::out_of_range("Matrix row index out of range");
		}
		for (size_type col = 0; col < cols_; ++col) {
			(*this)(rowIndex, col) = (*this)(rowIndex, col) * factor;
		}
	}

	void add_scaled_row(size_type targetRow, size_type sourceRow, const T& factor) {
		if (targetRow >= rows_ || sourceRow >= rows_) {
			throw std::out_of_range("Matrix row index out of range");
		}
		if (factor == T{}) {
			return;
		}
		for (size_type col = 0; col < cols_; ++col) {
			(*this)(targetRow, col) = (*this)(targetRow, col) + (*this)(sourceRow, col) * factor;
		}
	}

	Matrix operator+(const Matrix& other) const {
		ensure_same_size(other, "Matrix addition requires equal dimensions");
		Matrix result(rows_, cols_);
		for (size_type i = 0; i < data_.size(); ++i) {
			result.data_[i] = data_[i] + other.data_[i];
		}
		return result;
	}

	Matrix operator-(const Matrix& other) const {
		ensure_same_size(other, "Matrix subtraction requires equal dimensions");
		Matrix result(rows_, cols_);
		for (size_type i = 0; i < data_.size(); ++i) {
			result.data_[i] = data_[i] - other.data_[i];
		}
		return result;
	}

	Matrix operator*(const Matrix& other) const {
		if (cols_ != other.rows_) {
			throw std::invalid_argument("Matrix multiplication requires left cols = right rows");
		}

		Matrix result(rows_, other.cols_, T{});
		for (size_type rowIndex = 0; rowIndex < rows_; ++rowIndex) {
			for (size_type k = 0; k < cols_; ++k) {
				const T left = (*this)(rowIndex, k);
				for (size_type colIndex = 0; colIndex < other.cols_; ++colIndex) {
					result(rowIndex, colIndex) += left * other(k, colIndex);
				}
			}
		}
		return result;
	}

	Matrix inverse() const {
		if (rows_ != cols_) {
			throw std::invalid_argument("Matrix inverse requires a square matrix");
		}

		const size_type n = rows_;
		Matrix work(*this);
		Matrix inv = identity(n);

		for (size_type i = 0; i < n; ++i) {
			size_type pivotRow = i;
			while (pivotRow < n && work(pivotRow, i) == T{}) {
				++pivotRow;
			}
			if (pivotRow == n) {
				throw std::invalid_argument("Matrix is singular and cannot be inverted");
			}
			if (pivotRow != i) {
				work.swap_rows(pivotRow, i);
				inv.swap_rows(pivotRow, i);
			}

			const T pivot = work(i, i);
			if (pivot == T{}) {
				throw std::invalid_argument("Matrix is singular and cannot be inverted");
			}
			for (size_type col = 0; col < n; ++col) {
				work(i, col) = work(i, col) / pivot;
				inv(i, col) = inv(i, col) / pivot;
			}

			for (size_type row = 0; row < n; ++row) {
				if (row == i) {
					continue;
				}
				const T factor = work(row, i);
				if (factor == T{}) {
					continue;
				}
				for (size_type col = 0; col < n; ++col) {
					work(row, col) = work(row, col) - factor * work(i, col);
					inv(row, col) = inv(row, col) - factor * inv(i, col);
				}
			}
		}

		return inv;
	}

	bool operator==(const Matrix& other) const {
		return rows_ == other.rows_ && cols_ == other.cols_ && data_ == other.data_;
	}

	bool operator!=(const Matrix& other) const { return !(*this == other); }

	iterator begin() { return data_.begin(); }
	iterator end() { return data_.end(); }
	const_iterator begin() const { return data_.begin(); }
	const_iterator end() const { return data_.end(); }
	const_iterator cbegin() const { return data_.cbegin(); }
	const_iterator cend() const { return data_.cend(); }

private:
	size_type rows_ = 0;
	size_type cols_ = 0;
	std::vector<T> data_;

	size_type index(size_type row, size_type col) const { return row * cols_ + col; }

	static Matrix identity(size_type size) {
		Matrix result(size, size, T{});
		for (size_type i = 0; i < size; ++i) {
			result(i, i) = T{1};
		}
		return result;
	}

	void ensure_same_size(const Matrix& other, const char* message) const {
		if (rows_ != other.rows_ || cols_ != other.cols_) {
			throw std::invalid_argument(message);
		}
	}

	void ensure_in_range(size_type row, size_type col) const {
		if (row >= rows_ || col >= cols_) {
			throw std::out_of_range("Matrix index out of range");
		}
	}
};

} // namespace limo::numerics
