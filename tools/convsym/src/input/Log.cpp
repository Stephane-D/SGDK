
/* ------------------------------------------------------------ *
 * ConvSym utility version 2.12									*
 * Input wrapper for log files									*
 * ------------------------------------------------------------	*/

#include <cstdint>
#include <string>
#include <map>
#include <set>

#include <IO.hpp>
#include <OptsParser.hpp>

#include "InputWrapper.hpp"


struct Input__Log : public InputWrapper {

	Input__Log() {}
	~Input__Log() {}

	void parse(SymbolTable& symbolTable, const char *fileName, const char * opts = "") {
		// Supported options:
		//	/separator=x	- determines character that separates labes and offsets, default: ":"
		//	/useDecimal?	- set if offsets should be treat as decimal numbers; default: -
				
		// Variables and options
		char labelSeparator = ':';
		bool optUseDecimal = false;
		
		const std::map<std::string, OptsParser::record>
			OptsList {
				{ "separator",	{ .type = OptsParser::record::p_char, .target = &labelSeparator	} },
				{ "useDecimal",	{ .type = OptsParser::record::p_bool, .target = &optUseDecimal	} }
			};
			
		OptsParser::parse( opts, OptsList );		
		
		// Setup buffer, symbols list and file for input
		const int sBufferSize = 1024;
		uint8_t sBuffer[ sBufferSize ];
		std::multimap<uint32_t, std::string> SymbolMap;
		IO::FileInput input = IO::FileInput( fileName, IO::text );
		if ( !input.good() ) { 
			throw "Couldn't open input file"; 
		}	

		// Define re-usable conditions
		#define IS_HEX_CHAR(X) 			((unsigned)(X-'0')<10||(unsigned)(X-'A')<6||(unsigned)(X-'a')<6)  
		#define IS_NUMERIC(X) 			((unsigned)(X-'0')<10)
		#define SKIP_SPACES(X)			while ( *X==' ' || *X=='\t' ) X++

		int lineNum = 0;
		while (input.readLine( sBuffer, sBufferSize ) >= 0) {
			lineNum++;

			uint8_t* ptr = sBuffer;						// WARNING: Unsigned type is required here for certain range-based optimizations
			
			SKIP_SPACES(ptr);
			
			// Decode the offset ...
			uint32_t offset = 0;
			if ( optUseDecimal ) {
				while ( IS_NUMERIC(*ptr) ) {
					offset = offset *10 + *ptr-'0';    
					ptr++;
				}
			}
			else {
				while ( IS_HEX_CHAR(*ptr) ) {         
					offset = offset * 0x10 + ( (unsigned)(*ptr-'0')<10 ? (*ptr-'0') : 0xA + ((*ptr-'A'<6) ? (*ptr-'A') : (*ptr-'a') ));
					ptr++;
				}
			}
		
			// If line doesn't include proper separator, skip this line ...
			if ( *ptr++ != labelSeparator ) {
				IO::Log(IO::debug, "Failed to parse line %d, skipping", lineNum);
				continue;
			} 
			SKIP_SPACES(ptr);
			
			// Fetch label ... 
			char* sLabel = (char*)ptr;
			while ( !(*ptr == '\t' || *ptr == ' ') && *ptr ) {
				ptr++;
			}
			*ptr = 0x00;
			
			symbolTable.add(offset, sLabel);
		}
		
		#undef IS_HEX_CHAR
		#undef IS_NUMERIC
		#undef SKIP_SPACES
	}
};
