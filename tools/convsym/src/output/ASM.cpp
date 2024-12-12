
/* ------------------------------------------------------------ *
 * ConvSym utility version 2.12									*
 * Output wrapper for assembly file with equates				*
 * ------------------------------------------------------------	*/

#include <map>
#include <cstdint>
#include <string>
#include <algorithm>

#include <IO.hpp>

#include "OptsParser.hpp"
#include "OutputWrapper.hpp"


struct Output__Asm : public OutputWrapper {

	Output__Asm() {};
	~Output__Asm() {};

	/**
	 * Main function that generates the output
	 */
	void parse(
		std::multimap<uint32_t, std::string>& SymbolList,
		const char * fileName,
		uint32_t appendOffset = 0,
		uint32_t pointerOffset = 0,
		const char * opts = "",
		bool alignOnAppend = true
	) {
		if (appendOffset || pointerOffset || !alignOnAppend) {
			IO::Log(IO::warning, "Append options aren't supported by the \"asm\" output parser.");
		}

		// Supported options:
		//	/fmt='format-string' 	- overrides C-style format string (default: '%X: %s')

		// Default options
		std::string lineFormat = "%s:\tequ\t$%X";

		if (*opts && opts[0] == '/') {
			const std::map<std::string, OptsParser::record>
			OptsList {
				{ "fmt", { .type = OptsParser::record::p_string, .target = &lineFormat	} }
			};
			OptsParser::parse(opts, OptsList);
		}
		else if (*opts) {
			lineFormat = opts;
		}
		auto numSpecifiers = std::ranges::count(lineFormat, '%');
		if (numSpecifiers < 2) {
			IO::Log(IO::warning, "Line format string likely has too few arguments (try '%%s:\tequ\t$%%X')");
		}

		IO::FileOutput output = IO::FileOutput(fileName, IO::text);
		if (!output.good()) {
			IO::Log(IO::fatal, "Couldn't open file \"%s\"", fileName);
			throw "IO error";
		}

		const auto lineFormat_cstr = lineFormat.c_str();
		for (auto& symbol : SymbolList) {
			output.writeLine(lineFormat_cstr, symbol.second.c_str(), symbol.first);
		}
	}
};
