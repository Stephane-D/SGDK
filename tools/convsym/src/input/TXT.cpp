
/* ------------------------------------------------------------ *
 * ConvSym utility version 2.12									*
 * Input wrapper for TXT files									*
 * ------------------------------------------------------------	*/

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <map>
#include <set>
#include <algorithm>

#include <IO.hpp>
#include <OptsParser.hpp>

#include "InputWrapper.hpp"


struct Input__TXT : public InputWrapper {

	Input__TXT() {}
	~Input__TXT() {}

	void parse(SymbolTable& symbolTable, const char *fileName, const char * opts) {

		// Supported options:
		//	/fmt='format-string'	- C-style format string (default: '%s %X')
		//	/offsetFirst?			- specifies whether offset comes first in the input string (default is label followed by offset)

		// Default options
		std::string lineFormat = "%s %X";
		bool offsetFirst = false;

		const std::map<std::string, OptsParser::record>
			OptsList {
				{ "fmt",			{ .type = OptsParser::record::p_string,	.target = &lineFormat } },
				{ "offsetFirst",	{ .type = OptsParser::record::p_bool,	.target = &offsetFirst } },
			};
		OptsParser::parse(opts, OptsList);
		
		// Setup buffer, symbols list and file for input
		const int sBufferSize = 1024;
		uint8_t sBuffer[sBufferSize];
		std::multimap<uint32_t, std::string> SymbolMap;
		IO::FileInput input = IO::FileInput(fileName, IO::text);
		if (!input.good()) { 
			throw "Couldn't open input file"; 
		}

		auto numSpecifiers = std::ranges::count(lineFormat, '%');
		if (numSpecifiers < 2) {
			IO::Log(IO::warning, "Line format string likely has too few arguments (try '%%s %%X')");
		}

		int lineNum = 0;
		const auto lineFormat_cstr = lineFormat.c_str();
		while (input.readLine(sBuffer, sBufferSize) >= 0) {
			lineNum++;

			uint32_t offset = 0;
			char sLabel[512];

			const auto result = offsetFirst
				? sscanf((const char*)sBuffer, lineFormat_cstr, &offset, sLabel)
				: sscanf((const char*)sBuffer, lineFormat_cstr, sLabel, &offset);
			if (result != 2) {
				IO::Log(IO::debug, "Failed to parse line %d, skipping (result=%d)", lineNum, result);
				continue;
			}

			symbolTable.add(offset, sLabel);
		}
	}

};
