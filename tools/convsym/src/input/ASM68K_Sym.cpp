
/* ------------------------------------------------------------ *
 * ConvSym utility version 2.12									*
 * Input wrapper for the ASM68K compiler's symbol format		*
 * ------------------------------------------------------------	*/

#include <cstdint>
#include <string>
#include <map>

#include <IO.hpp>
#include <OptsParser.hpp>

#include "InputWrapper.hpp"


struct Input__ASM68K_Sym : public InputWrapper {

	Input__ASM68K_Sym() {}
	~Input__ASM68K_Sym() {}

	void parse(SymbolTable& symbolTable, const char *fileName, const char * opts) {
		IO::FileInput input = IO::FileInput(fileName);
		if (!input.good()) { throw "Couldn't open input file"; }

		// Supported options:
		//	/localSign=x			- determines character used to specify local labels
		//	/localJoin=x			- character used to join local label and its global "parent"
		//	/processLocals?			- specify whether local labels will processed

		// Default processing options
		bool optProcessLocalLabels = true;

		// Variables and options
		char localLabelSymbol = '@';		// default symbol for local labels
		char localLabelRef = '.';			// default symbol to reference local labels within global ones
		
		const std::map<std::string, OptsParser::record>
			OptsList {
				{ "localSign",			{ .type = OptsParser::record::p_char,	.target = &localLabelSymbol			} },
				{ "localJoin",			{ .type = OptsParser::record::p_char,	.target = &localLabelRef			} },
				{ "processLocals",		{ .type = OptsParser::record::p_bool,	.target = &optProcessLocalLabels	} }
			};
			
		OptsParser::parse(opts, OptsList);

		// NOTICE: Symbols are usually written OUT OF ORDER in the symbols file,
		//	so we have to map them first before filtering
		std::multimap<uint32_t, std::string> UnfilteredSymbolsMap;
		input.setOffset(0x0008);

		for(;;) {
			uint32_t offset;
			try {				// read 32-bit label offset
				offset = input.readLong();
			} catch(...) {		// if reading failed, break
				break;
			}

			input.setOffset( 1, IO::current );					// skip 1 byte

			const size_t labelLength = (size_t)input.readByte();
			char sLabel[255];
			input.readData((uint8_t*)&sLabel, labelLength);	// read label

			UnfilteredSymbolsMap.insert({ offset, std::string((const char*)&sLabel, labelLength)});
		}

		// Now we can properly process symbols list IN ORDER
		const std::string *lastGlobalLabelRef = &UnfilteredSymbolsMap.cbegin()->second;	// default global label name

		for (const auto & [offset, label]: UnfilteredSymbolsMap) {
			if (label[0] == localLabelSymbol) {
				// Ignore local labels if "processLocals" is disabled
				if ( !optProcessLocalLabels ) {
					IO::Log(IO::debug, "Local symbol ignored: %s", label.c_str());
					continue;
				}

				std::string fullLabel = *lastGlobalLabelRef + localLabelRef + label.substr(1);
				symbolTable.add(offset, fullLabel);
			}
			else {
				lastGlobalLabelRef = &label;
				symbolTable.add(offset, label);
			}
		}
	}

};

