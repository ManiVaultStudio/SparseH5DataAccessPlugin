#include <catch2/catch_test_macros.hpp>	// for info on testing see https://github.com/catchorg/Catch2/blob/devel/docs/tutorial.md#test-cases-and-sections

#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
#include <string>
#include <vector>

#include "H5Utils.h"
#include "test_utils.h"

namespace fs = std::filesystem;

const fs::path current_file_path = std::source_location::current().file_name();
const fs::path dataDir = current_file_path.parent_path() / "data";

TEST_CASE("Read sparse matrices from H5", "[H5][CRS][CSC]") {

	CSRReader             csrMatrix;
	CSCReader             cscMatrix;
	SparseMatrixReader*		sparseMatrix = nullptr;

	fs::path fileNameSparseMatrix;
	std::string	sparseMatrixType;

	SECTION("CRS") {
		info("\nTEST: CRS\n");
		sparseMatrix = &csrMatrix;	
		fileNameSparseMatrix = "csr.h5";
		sparseMatrixType = "CSR";
	}

	SECTION("CSC") {
		info("\nTEST: CSC\n");
		sparseMatrix = &cscMatrix;	
		fileNameSparseMatrix = "csc.h5";
		sparseMatrixType = "CSC";
	}

	assert(sparseMatrix != nullptr);

	bool readSuccess = sparseMatrix->readFile((dataDir / fileNameSparseMatrix).string());

	if (!readSuccess) {
		info("ERROR: test file not loaded, probably it does not exist");
		return;
	}

	sparseMatrix->setUseCache(false);

	REQUIRE(sparseMatrix->getTypeString() == sparseMatrixType);

	REQUIRE(sparseMatrix->hasObsNames());
	REQUIRE(sparseMatrix->hasVarNames());

	const std::vector<std::string>& obsNames = sparseMatrix->getObsNames();
	const std::vector<std::string>& varNames = sparseMatrix->getVarNames();

	REQUIRE(obsNames.size() == sparseMatrix->getNumRows());
	REQUIRE(varNames.size() == sparseMatrix->getNumCols());

	info("obsNames: ");
	print(obsNames);
	info("varNames: ");
	print(varNames);
	info("dense matrix:\n");
	print(*sparseMatrix);

	checkApprox(sparseMatrix->getRow(0), { 0.f,  10.f, 50.f,   0.f });
	checkApprox(sparseMatrix->getRow(1), { 0.f,   0.f, 20.2f,  0.f, });
	checkApprox(sparseMatrix->getRow(2), { 30.4f, 0.f,  0.f,  70.f, });
	checkApprox(sparseMatrix->getRow(3), { 0.f,   0.f,  0.f,  40.6f, });
	checkApprox(sparseMatrix->getRow(4), { 0.f,   0.f,  0.f,  60.f, });

	checkApprox(sparseMatrix->getColumn(0), { 0.f,  0.f, 30.4f, 0.f,   0.f });
	checkApprox(sparseMatrix->getColumn(1), { 10.f,  0.f,  0.f,  0.f,   0.f });
	checkApprox(sparseMatrix->getColumn(2), { 50.f, 20.2f, 0.f,  0.f,   0.f });
	checkApprox(sparseMatrix->getColumn(3), { 0.f,  0.f, 70.f, 40.6f, 60.f });
}

