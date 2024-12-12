
#pragma once

#include <cstdint>
#include <string>
#include <map>

#include "../util/SymbolTable.hpp"

/* Base class for the input formats handlers */
struct InputWrapper {
	InputWrapper() {}
	virtual ~InputWrapper() {}

	// Virtual function interface that handles input file parsing
	virtual void parse(
		SymbolTable& symbolTable,
		const char *fileName,
		const char *opts
	) = 0;
};
