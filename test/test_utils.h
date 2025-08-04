#pragma once

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "H5Utils.h"

/// //////// ///
/// PRINTING ///
/// //////// ///

void info(const std::string& message) {
	std::cout << message << std::flush;
}

template<typename T>
void print(const std::vector<T>& vec) {
	for (const auto& val : vec) {
		std::cout << val << " ";
	}
	std::cout << std::endl;
}

template<typename T>
void print(const std::vector<T>& vec, int width) {
	for (const auto& val : vec) {
		std::cout << std::setw(width) << val;
	}
	std::cout << std::endl;
}

void print(const SparseMatrixReader& sparseMat, int width = 5) {
	for (size_t row = 0; row < sparseMat.getNumRows(); row++) {
		print(sparseMat.getRow(row), width);
	}
	std::cout << std::endl;
}

/// /////// ///
/// TESTING ///
/// /////// ///

inline void checkApprox(const std::vector<float>& a, const std::vector<float>& b, float margin = 1e-5f)
{
	REQUIRE(a.size() == b.size());

	for (size_t i = 0; i < a.size(); ++i) {
		REQUIRE(a[i] == Catch::Approx(b[i]).margin(margin));
	}
}
