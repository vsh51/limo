#include "limo/numerics/Matrix.hpp"

#include <gtest/gtest.h>

using limo::numerics::Matrix;

TEST(MatrixConstructionTests, CreatesWithDimensionsAndInitializerList) {
    Matrix<int> matrix(2, 3, 7);
    EXPECT_EQ(matrix.rows(), 2u);
    EXPECT_EQ(matrix.cols(), 3u);
    EXPECT_EQ(matrix.size(), 6u);

    for (std::size_t r = 0; r < matrix.rows(); ++r) {
        for (std::size_t c = 0; c < matrix.cols(); ++c) {
            EXPECT_EQ(matrix(r, c), 7);
        }
    }
}

TEST(MatrixConstructionTests, HandlesInvalidInitializerList) {
    Matrix<int> empty{};
    EXPECT_TRUE(empty.empty());
    EXPECT_EQ(empty.rows(), 0u);
    EXPECT_EQ(empty.cols(), 0u);
    EXPECT_EQ(empty.size(), 0u);

    EXPECT_THROW((Matrix<int>{{1, 2}, {3}}), std::invalid_argument);
}

TEST(MatrixStateTests, ClearsResizesAndFills) {
    Matrix<int> matrix(2, 2, 1);
    matrix.fill(5);
    EXPECT_EQ(matrix(0, 0), 5);
    EXPECT_EQ(matrix(1, 1), 5);

    matrix.clear();
    EXPECT_TRUE(matrix.empty());
    EXPECT_EQ(matrix.rows(), 0u);
    EXPECT_EQ(matrix.cols(), 0u);

    matrix.resize(1, 3, 9);
    EXPECT_EQ(matrix.rows(), 1u);
    EXPECT_EQ(matrix.cols(), 3u);
    EXPECT_EQ(matrix(0, 2), 9);
}

TEST(MatrixAccessTests, ProvidesRowAccessAndBoundsChecks) {
    Matrix<int> matrix(2, 3);
    int value = 1;
    for (std::size_t r = 0; r < matrix.rows(); ++r) {
        for (std::size_t c = 0; c < matrix.cols(); ++c) {
            matrix(r, c) = value++;
        }
    }

    const int* data = matrix.data();
    EXPECT_EQ(data[0], 1);
    EXPECT_EQ(data[5], 6);

    auto row = matrix.row(1);
    EXPECT_EQ(row.size(), 3u);
    EXPECT_EQ(row[0], 4);
    EXPECT_EQ(row[2], 6);

    EXPECT_NO_THROW(matrix.at(0, 0));
    EXPECT_THROW(matrix.at(0, 3), std::out_of_range);
    EXPECT_THROW(matrix.at(2, 0), std::out_of_range);
    EXPECT_THROW(matrix.row(2), std::out_of_range);

    Matrix<int> empty;
    EXPECT_THROW(empty.row(0), std::out_of_range);

    const Matrix<int> constMatrix{{1, 2}, {3, 4}};
    auto constRow = constMatrix.row(0);
    EXPECT_EQ(constRow[0], 1);
    EXPECT_EQ(constRow[1], 2);
}

TEST(MatrixArithmeticTests, AddsAndSubtractsMatrices) {
    Matrix<int> left{{1, 2}, {3, 4}};
    Matrix<int> right{{4, 3}, {2, 1}};

    Matrix<int> sum = left + right;
    EXPECT_EQ(sum(0, 0), 5);
    EXPECT_EQ(sum(1, 1), 5);

    Matrix<int> diff = left - right;
    EXPECT_EQ(diff(0, 0), -3);
    EXPECT_EQ(diff(1, 1), 3);
    Matrix<int> mismatch(2, 3, 1);
    EXPECT_THROW(left + mismatch, std::invalid_argument);
    EXPECT_THROW(left - mismatch, std::invalid_argument);
}

TEST(MatrixArithmeticTests, MultipliesMatrices) {
    Matrix<int> left{{1, 2, 3}, {4, 5, 6}};
    Matrix<int> right{{7, 8}, {9, 10}, {11, 12}};

    Matrix<int> product = left * right;
    EXPECT_EQ(product.rows(), 2u);
    EXPECT_EQ(product.cols(), 2u);
    EXPECT_EQ(product(0, 0), 58);
    EXPECT_EQ(product(0, 1), 64);
    EXPECT_EQ(product(1, 0), 139);
    EXPECT_EQ(product(1, 1), 154);
    Matrix<int> mismatch(2, 2, 1);
    EXPECT_THROW(left * mismatch, std::invalid_argument);
}

TEST(MatrixArithmeticTests, InverseHandlesValidAndInvalidMatrices) {
    Matrix<double> matrix{{4.0, 7.0}, {2.0, 6.0}};
    Matrix<double> inverse = matrix.inverse();
    EXPECT_NEAR(inverse(0, 0), 0.6, 1e-9);
    EXPECT_NEAR(inverse(0, 1), -0.7, 1e-9);
    EXPECT_NEAR(inverse(1, 0), -0.2, 1e-9);
    EXPECT_NEAR(inverse(1, 1), 0.4, 1e-9);

    Matrix<double> rowSwap{{0.0, 1.0}, {2.0, 3.0}};
    Matrix<double> rowSwapInverse = rowSwap.inverse();
    EXPECT_NEAR(rowSwapInverse(0, 0), -1.5, 1e-9);
    EXPECT_NEAR(rowSwapInverse(0, 1), 0.5, 1e-9);
    EXPECT_NEAR(rowSwapInverse(1, 0), 1.0, 1e-9);
    EXPECT_NEAR(rowSwapInverse(1, 1), 0.0, 1e-9);

    Matrix<double> identity{{1.0, 0.0}, {0.0, 1.0}};
    Matrix<double> identityInverse = identity.inverse();
    EXPECT_NEAR(identityInverse(0, 0), 1.0, 1e-9);
    EXPECT_NEAR(identityInverse(0, 1), 0.0, 1e-9);
    EXPECT_NEAR(identityInverse(1, 0), 0.0, 1e-9);
    EXPECT_NEAR(identityInverse(1, 1), 1.0, 1e-9);

    Matrix<double> nonSquare(2, 3, 1.0);
    EXPECT_THROW(nonSquare.inverse(), std::invalid_argument);

    Matrix<double> singular{{1.0, 2.0}, {2.0, 4.0}};
    EXPECT_THROW(singular.inverse(), std::invalid_argument);

    Matrix<double> zeroColumn{{0.0, 1.0}, {0.0, 2.0}};
    EXPECT_THROW(zeroColumn.inverse(), std::invalid_argument);
}

TEST(MatrixArithmeticTests, EqualityOperatorsCompareSizesAndData) {
    Matrix<int> left{{1, 2}, {3, 4}};
    Matrix<int> same{{1, 2}, {3, 4}};
    Matrix<int> other{{1, 2}, {3, 5}};
    Matrix<int> differentSize(1, 2, 0);

    EXPECT_TRUE(left == same);
    EXPECT_TRUE(left != other);
    EXPECT_TRUE(left != differentSize);
}

TEST(MatrixRowOperationTests, SwapsScalesAndAddsRows) {
    Matrix<int> matrix{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    matrix.swap_rows(0, 2);
    EXPECT_EQ(matrix(0, 0), 7);
    EXPECT_EQ(matrix(2, 2), 3);

    matrix.scale_row(1, 2);
    EXPECT_EQ(matrix(1, 0), 8);
    EXPECT_EQ(matrix(1, 2), 12);

    matrix.add_scaled_row(0, 1, -1);
    EXPECT_EQ(matrix(0, 0), -1);
    EXPECT_EQ(matrix(0, 2), -3);

    EXPECT_THROW(matrix.swap_rows(0, 3), std::out_of_range);
    EXPECT_THROW(matrix.scale_row(3, 2), std::out_of_range);
    EXPECT_THROW(matrix.add_scaled_row(0, 4, 1), std::out_of_range);
}

TEST(MatrixTransformTests, TransposesMatrix) {
    Matrix<int> matrix{{1, 2, 3}, {4, 5, 6}};
    Matrix<int> transposed = matrix.transpose();

    EXPECT_EQ(transposed.rows(), 3u);
    EXPECT_EQ(transposed.cols(), 2u);
    EXPECT_EQ(transposed(0, 0), 1);
    EXPECT_EQ(transposed(1, 0), 2);
    EXPECT_EQ(transposed(2, 0), 3);
    EXPECT_EQ(transposed(0, 1), 4);
    EXPECT_EQ(transposed(1, 1), 5);
    EXPECT_EQ(transposed(2, 1), 6);
}
