
#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <map>
#include <unordered_map>

#include "IO.hpp"

struct OffsetConversionOptions {
	uint32_t baseOffset;
	uint32_t offsetLeftBoundary;
	uint32_t offsetRightBoundary;
	uint32_t offsetMask;	
};

typedef std::unordered_map<std::string_view, std::reference_wrapper<uint32_t>> SymbolToOffsetResolveTable;

struct SymbolTable {
	const OffsetConversionOptions& offsetConversionOpts;
	const SymbolToOffsetResolveTable& symbolToOffsetResolveTable;

	std::multimap<uint32_t,std::string> symbols;

	SymbolTable(
		const OffsetConversionOptions& _offsetConversionOpts,
		const SymbolToOffsetResolveTable& _symbolToOffsetResolveTable
	): offsetConversionOpts(_offsetConversionOpts), symbolToOffsetResolveTable(_symbolToOffsetResolveTable), symbols({}) {}

	SymbolTable(const SymbolTable& symbolTable) = delete;
	SymbolTable& operator=(const SymbolTable& symbolTable) = delete;

	template<typename LabelType>
	inline bool add(uint32_t offset, LabelType label) {
		const uint32_t correctedOffset = (offset - offsetConversionOpts.baseOffset) & offsetConversionOpts.offsetMask;

		/* If we have symbols to resolve for some options (e.g. `-ref sym:MySymbolName`), resolve to offset if name matches */
		if (!symbolToOffsetResolveTable.empty()) {
			const auto symbolToOffsetEntry = symbolToOffsetResolveTable.find(label);
			if (symbolToOffsetEntry != symbolToOffsetResolveTable.end()) {
				symbolToOffsetEntry->second.get() = correctedOffset;
				IO::Log(IO::debug, "Resolved requested symbol offset: %X", correctedOffset);
			}
		}

		if (!(
			correctedOffset >= offsetConversionOpts.offsetLeftBoundary && 
			correctedOffset <= offsetConversionOpts.offsetRightBoundary
		)) {
			return false;	// symbol is not inserted when offset is out of range
		}

		if (IO::LogLevel <= IO::debug) {
			IO::Log(IO::debug, "Adding symbol: %s", std::string(label).c_str());
		}
		symbols.emplace(correctedOffset, std::string(label));

		return true;
	}

};
