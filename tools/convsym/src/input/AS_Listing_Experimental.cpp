
/* ------------------------------------------------------------ *
 * ConvSym utility version 2.12									*
 * Input wrapper for the AS listing format						*
 * ------------------------------------------------------------	*/

#include <cstdint>
#include <string>
#include <map>

#include <IO.hpp>

#include "InputWrapper.hpp"


struct Input__AS_Listing_Experimental : public InputWrapper {

	Input__AS_Listing_Experimental() {}
	~Input__AS_Listing_Experimental() {}

	void parse(SymbolTable& symbolTable, const char *fileName, const char * opts) {
		const int sBufferSize = 1024;
		if (*opts) {
			IO::Log(IO::warning, "-inopt is not supported by this parser");
		}

		// Variables
		uint8_t sBuffer[ sBufferSize ];
		std::multimap<uint32_t, std::string> SymbolMap;
		IO::FileInput input = IO::FileInput( fileName, IO::text );
		if ( !input.good() ) { throw "Couldn't open input file"; }
		uint32_t lastSymbolOffset = -1;		// tracks symbols offsets to ignore sections where PC is reset (mainly Z80 stuff)

		// For every string in a listing file ...
		while ( input.readLine( sBuffer, sBufferSize ) >= 0 ) {

			// Known issues for the Sonic 2 disassembly:
			//	* Some macros somehow define labels that looks like global ones (notably, _MOVE and such)
			//	* Labels from injected Z80 code sometimes make it to the list ...

			uint8_t* ptr = sBuffer;		// WARNING: dereffed type should be UNSIGNED, as it's used for certain range-based optimizations

			// Check if this line has file idention number (pattern: "(d)", where d is a decimal digit)
			if ( *ptr == '(' ) {
				bool foundDigits = false;
				ptr++;

				// Cycle through as long as found characters are digits
				while ( (unsigned)(*ptr-'0')<10 ) { foundDigits=true; ptr++; }

				// If digit pattern doesn't end with ")" as expected, or no digits were found at all, stop
				if ( *ptr++ != ')' || !foundDigits ) continue;
			}

			// Ensure line has a proper number (pattern: "ddddd/", where d is any digit)
			{
				bool foundDigits = false;
				while ( *ptr == ' ' ) { ptr++; } // skip spaces, if present

				// Cycle through as long as found characters are digits
				while ( (unsigned)(*ptr-'0')<10 ) { foundDigits=true; ptr++; }

				// If digit pattern doesn't end with "/" as expected, or no digits were found at all, stop
				if ( *ptr++ != '/' || !foundDigits ) continue;
			}

			// Ensure line has a proper offset (pattern: "hhhh :", where h is a hexidecimal character)
			while ( *ptr == ' ' ) { ptr++; }						// skip spaces, if present
			uint8_t* const sOffset = ptr;							// remember location, where offset should presumably start
			{
				bool foundHexDigits = false;

				// Cycle though as longs as proper hexidecimal characters are fetched
				while (	(unsigned)(*ptr-'0')<10 || (unsigned)(*ptr-'A')<6 ) {
					foundHexDigits = true;
					ptr++;
				}

				if ( *ptr == 0x00 ) continue;							// break if end of line was reached
				*ptr++ = 0x00;											// separate string pointered by "offset" variable from the rest of the line
				while ( *ptr == ' ' ) { ptr++; }						// skip spaces ...

				// If offset pattern doesn't end with ":" as expected, or no hex characters were found at all, stop
				if ( *ptr++ != ':' || !foundHexDigits ) continue;
			}

			// Check if line matches a label definition ...
			if ( *ptr == ' ' && *(ptr+1) == '(' ) continue;
			while ( *ptr && (ptr-sBuffer < 40) ) { ptr++; }			// align to column 40 (where line starts)
			while ( *ptr==' ' || *ptr=='\t' ) { ptr++; }			// skip spaces or tabs, if present
			uint8_t* const label = ptr;								// remember location, where label presumably starts...
			
			// The first character should be a latin letter (A..Z or a..z)
			if ( (unsigned)(*ptr-'A')<26 || (unsigned)(*ptr-'a')<26 ) {

				// Other characters should be letters are digits or _ char
				while ( 											
					(unsigned)(*ptr-'A')<26 || (unsigned)(*ptr-'a')<26 || (unsigned)(*ptr-'0')<10 || *ptr=='_'
				) { ptr++; }

				// If the word is the only on the line and it ends with ":", treat it as a label
				if ( *ptr==':' ) {
					*ptr++ = 0x00;		// separate string pointered by "label" variable from the rest of the line

					// Convert offset to uint32_t
					uint32_t offset = 0;
					for ( uint8_t* c = sOffset; *c; c++ ) {
						offset = offset*0x10 + (((unsigned)(*c-'0')<10) ? (*c-'0') : (*c-('A'-10)));
					}

					// Add label to the symbols table, if:
					//	1) Its absolute offset is higher than the previous offset successfully added
					//	2) When base offset is subtracted and the mask is applied, the resulting offset is within allowed boundaries
					if ( (lastSymbolOffset == (uint32_t)-1) || (offset >= lastSymbolOffset) ) {
						// Check if this is a label after ds or rs macro ...
						while ( *ptr==' ' || *ptr=='\t' ) { ptr++; }			// skip spaces or tabs, if present      
						if ( *ptr == 'd' && *(ptr+1) == 's' && *(ptr+2)=='.' ) continue;
						if ( *ptr == 'r' && *(ptr+1) == 's' && *(ptr+2)=='.' ) continue;

						const bool inserted = symbolTable.add(offset, (const char*)label);
						if (inserted) {
							lastSymbolOffset = offset;
						}
					}
					else {
						IO::Log( IO::debug, "Symbol %s at offset %X ignored: its offset is less than the previous symbol successfully fetched", label, offset );
					}
				}
			}
		}
	}

};
