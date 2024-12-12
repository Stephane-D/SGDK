
/* ------------------------------------------------------------ *
 * ConvSym utility version 2.12									*
 * Input wrapper for the ASM68K listing format					*
 * ------------------------------------------------------------	*/

#include <cstdint>
#include <string>
#include <map>
#include <set>

#include <IO.hpp>
#include <OptsParser.hpp>

#include "InputWrapper.hpp"


struct Input__ASM68K_Listing : public InputWrapper {

	Input__ASM68K_Listing() {}
	~Input__ASM68K_Listing() {}

	void parse(SymbolTable& symbolTable, const char *fileName, const char * opts = "") {
		// Known issues:
		//	* Doesn't recognize line break character "&", as line continuations aren't properly listed by ASM68K

		// Supported options:
		//	/localSign=x			- determines character used to specify local labels
		//	/localJoin=x			- character used to join local label and its global "parent"
		//	/ignoreMacroDefs?		- specify if macro definitions listings should be ignored (lines between "macro" and "endm"); default: +
		//	/ignoreMacroExp?		- specify if lines representing macro expansions should be ignored; default: -
		//	/addMacrosAsOpcodes?	- set if macros that process label as parameter (defined as "macro *") should be recognized when used; default: +
		//	/processLocals?			- specify whether local labels will processed

		// Default processing options
		bool optIgnoreMacroExpansions = false;
		bool optIgnoreMacroDefinitions = true;
		bool optRegisterMacrosAsOpcodes = true;
		bool optProcessLocalLabels = true;

		// Variables
		std::string strLastGlobalLabel("");	// default global label name
		char localLabelSymbol = '@';		// default symbol for local labels
		char localLabelRef = '.';			// default symbol to reference local labels within global ones
		uint32_t lastSymbolOffset = -1;		// tracks symbols offsets to ignore sections where PC is reset (mainly Z80 stuff)

		// Fetch options from "-inopt" agrument's value
		const std::map<std::string, OptsParser::record>
			OptsList {
				{ "localSign",			{ .type = OptsParser::record::p_char,	.target = &localLabelSymbol				} },
				{ "localJoin",			{ .type = OptsParser::record::p_char,	.target = &localLabelRef				} },
				{ "ignoreMacroDefs",	{ .type = OptsParser::record::p_bool,	.target = &optIgnoreMacroDefinitions	} },
				{ "ignoreMacroExp",		{ .type = OptsParser::record::p_bool,	.target = &optIgnoreMacroExpansions		} },
				{ "addMacrosAsOpcodes",	{ .type = OptsParser::record::p_bool,	.target = &optRegisterMacrosAsOpcodes	} },
				{ "processLocals",		{ .type = OptsParser::record::p_bool,	.target = &optProcessLocalLabels		} }
			};
			
		OptsParser::parse( opts, OptsList );

		// Setup buffer, symbols list and file for input
		const int sBufferSize = 1024;
		uint8_t sBuffer[sBufferSize];
		std::multimap<uint32_t, std::string> SymbolMap;
		IO::FileInput input = IO::FileInput(fileName, IO::text);
		if (!input.good()) {
			throw "Couldn't open input file";
		}

		// Vocabulary for assembly directives that support labels
		// NOTICE: This will be also extended with macro names
		std::set<std::string> NamingOpcodes = {
			"=", "equ", "equs", "equr", "reg", "rs", "rsset", "set", "macro", "substr", "section", "group"
		};

		// Define re-usable conditions
		#define IS_HEX_CHAR(X) 			((unsigned)(X-'0')<10||(unsigned)(X-'A')<6)
		#define IS_START_OF_NAME(X)		((unsigned)(X-'A')<26||(unsigned)(X-'a')<26||(optProcessLocalLabels&&X==localLabelSymbol)||X=='.'||X=='_')
		#define IS_NAME_CHAR(X)			((unsigned)(X-'A')<26||(unsigned)(X-'a')<26||(unsigned)(X-'0')<10||X=='?'||X=='.'||X=='_')
		#define IS_START_OF_LABEL(X)	((unsigned)(X-'A')<26||(unsigned)(X-'a')<26||(optProcessLocalLabels&&X==localLabelSymbol)||X=='_')
		#define IS_LABEL_CHAR(X)		((unsigned)(X-'A')<26||(unsigned)(X-'a')<26||(unsigned)(X-'0')<10||X=='?'||X=='_')
		#define IS_WHITESPACE(X)		(X==' '||X=='\t')
		#define IS_ENDOFLINE(X)			(X=='\n'||X=='\r'||X==0x00)

		// For every string in a listing file ...
		for ( 
				int lineCounter = 0, lineLength; 
				lineLength = input.readLine( sBuffer, sBufferSize ), lineLength >= 0; 
				++lineCounter 
			) {

			// If line is too short, do not proceed
			if ( lineLength <= 36 ) {
				IO::Log( IO::debug, "Line %d is too short, skipping", lineCounter );
				continue;
			}

			//// Bugfix: for when string separation was forced at null-terminator
			//if ( lineLength < sBufferSize ) {
			//	sBuffer[ lineLength+1 ] = 0x00;
			//}

			uint8_t* const sLineOffset = sBuffer;		// E.g.: "00000AEE 301F <..>move.w (sp)+, d0\n"
			uint8_t* const sLineText = sBuffer+36;		// E.g.: "move.w (sp)+, d0\n"
			uint8_t* const cMacroMark = sBuffer+34;		// If contains "M" at the specified column (column 34), the line is macro expansion

			uint8_t* ptr = sBuffer;						// WARNING: Unsigned type is required here for certain range-based optimizations

			// Check for proper offset at the beginning of the listing line
			{
				bool hasProperOffset = true;
				for (int i = 0; i < 8; ++i) {
					if (!IS_HEX_CHAR(*ptr)) {
						hasProperOffset = false;
						break;
					}
					ptr++;
				}
				if (!hasProperOffset) {
					IO::Log(IO::debug, "Line %d doesn't have a proper offset, skipping...", lineCounter);
					continue;
				}
				*ptr++ = 0x00;					// separate offset, so "sLineOffset" is proper c-string, containing only offset
			}

			// If this line represents an expression result, ignore
			if (*ptr == '=') {
				continue;
			}
			
			// If this line is macro expansion and option is set to ignore expansions, ignore
			if (optIgnoreMacroExpansions && *cMacroMark == 'M') {
				continue;
			}

			// NOTICE: If line offset is present, it's guranteed that line is at least 36 characters long, so ...
			// ... "sLineText = sBuffer+36" is a valid location
			uint8_t* sLabel = nullptr;			// assume label is NULL, but the following blocks of code will attempt to find lable in the line
			ptr = sLineText;

			// -----------------------------------------------------------
			// Code to intentify if label or name is present on the line
			// -----------------------------------------------------------

			// Scenario #1 : Line doesn't have indention, meaning it starts with a name
			// NOTICE: In this case, label may use a wider range of allowed characters, hence it's referenced as "NAME" below ...
			if (IS_START_OF_NAME(*ptr)) {
				IO::Log(IO::debug, "Line %d: Possible label at the beginning of line", lineCounter);
				sLabel = ptr++;						// assume this as label
				while (IS_NAME_CHAR(*ptr)) ptr++;	// iterate through label characters

				// Make sure label ends properly
				if (IS_WHITESPACE(*ptr) || *ptr==':' || IS_ENDOFLINE(*ptr)) {
					*ptr++ = 0x00;			// mark labels end, so "sLabel" is a proper c-string containing label alone now
				}
				else {
					continue;				// cancel further processing
				}
			}

			// Scenario #2 : Line starts with idention (space or tab)
			// NOTICE: In this case, label cannot include certain characters allowed otherwise...
			else if (IS_WHITESPACE(*ptr)) {
				IO::Log(IO::debug, "Line %d: Possible label with idention", lineCounter);
				do { ptr++; } while (IS_WHITESPACE(*ptr)); 	// skip idention
				if (IS_START_OF_LABEL(*ptr)) {
					sLabel = ptr++;							// assume this as label
					while (IS_LABEL_CHAR(*ptr)) ptr++;		// iterate through label characters

					// Make sure label ends properly
					if (*ptr==':') {
						*ptr++ = 0x00;			// mark labels end, so "sLabel" is a proper c-string containing label alone now
					}
					else {
						continue;				// cancel further processing
					}
				}
			}

			// Scenario #3: Line doesn't seem to contain a label ...
			else {
				IO::Log(IO::debug, "Line %d: Didn't identify label, skipping", lineCounter);
				continue;
			}

			// If label was determined ...
			// WARNING: "ptr" should point past label's end!
			if (sLabel != nullptr) {
				// Construct full label's name as std::string object
				std::string strLabel;
				if (*sLabel == localLabelSymbol) {
					strLabel  = strLastGlobalLabel;
					strLabel += localLabelRef;
					strLabel += (char*)sLabel+1;	// +1 to skip local label symbol itself
				}
				else {
					strLabel = strLastGlobalLabel = (char*)sLabel;
				}

				// Fetch label's opcode into std::string object
				while (IS_WHITESPACE(*ptr)) ptr++; 		// skip indention
				uint8_t* const ptr_start = ptr;
				do { ptr++; } while (!IS_WHITESPACE(*ptr) && !IS_ENDOFLINE(*ptr));
				*ptr++ = 0x00;
				std::string strOpcode((char*)ptr_start, ptr-ptr_start-1);		// construct opcode string
				if (strOpcode[0] == localLabelSymbol) {					// in case opcode is a local label reference
					strOpcode = strLastGlobalLabel;
					strOpcode += localLabelRef;
					strOpcode += (char*)ptr_start+1;	// +1 to skip local label symbol itself
				}

				IO::Log(IO::debug, "Processing: %s: %s", strLabel.c_str(), strOpcode.c_str());

				// Make sure this label doesn't name any special object ...
				auto opcodeRef = NamingOpcodes.find(strOpcode);
				if (opcodeRef != NamingOpcodes.end()) {
					// If this label names a macro ...
					if (!opcodeRef->compare("macro")) {	// TODOh: Optimize by handling pointer to "macro" record within set

						IO::Log(IO::debug, "%s recognized as macro declaration", strLabel.c_str());

						// If macro processing option is on ...
						if (optRegisterMacrosAsOpcodes) {
							while (IS_WHITESPACE(*ptr)) ptr++; 	// skip indention

							// If macro uses labels as argument, add macro's name (the label) to the vocabulary
							if (*ptr == '*') {
								NamingOpcodes.insert(strLabel);
							}
						}

						// If ignore macro definitions option is on ...
						if (optIgnoreMacroDefinitions) {
							bool endmDirectiveReached = false;

							for (
								int macroLineCounter = 0, macroLineLength;
								macroLineLength = input.readLine( sBuffer, sBufferSize ), macroLineLength >= 0;
								++macroLineCounter
							) {
								// Maintain line counter to warn if suspiciously many lines were processed as macro definition alone
								if (macroLineCounter >= 1000) {
									IO::Log(IO::warning,
										// TODOh: Advise to enable ignore macro definitions option?
										"Too many lines (>=1000) found in definition of \"%s\" macro. This could be missing \"endm\" statement or a parsing error.",
										strLabel.c_str()
									);
									break;
								}

								// Make sure this line includes assembly text
								if (macroLineLength <= 36) {
									continue;
								}

								ptr = sBuffer+36;

								// If line starts with label, skip it ...
								if (!IS_WHITESPACE(*ptr)) {
									do { ptr++; } while (!IS_WHITESPACE(*ptr) && !IS_ENDOFLINE(*ptr));
								}
								
								// Fetch opcode, if present ...
								while (IS_WHITESPACE(*ptr)) ptr++;
								uint8_t* const ptr_start = ptr;
								do { ptr++; } while (!IS_WHITESPACE(*ptr) && !IS_ENDOFLINE(*ptr));
								*ptr++ = 0x00;
								
								// If opcode is "endm", stop processing
								if (!strcmp((char*)ptr_start, "endm")) {
									IO::Log(IO::debug,
										"Skipped definition of macro \"%s\" (lines %d-%d)",
										strLabel.c_str(), lineCounter, lineCounter+macroLineCounter
									);
									lineCounter += macroLineCounter;
									endmDirectiveReached = true;
									break;
								}
							}
							
							// If end of file was reached before "endm"
							if (!endmDirectiveReached) {
								IO::Log(IO::error,
									// TODOh: Advise to enable ignore macro definitions option?
									"Couldn't reach end of \"%s\" macro. This is possibly due to a parsing error.",
									strLabel.c_str()
								);
								break;
							}

							// Otherwise, cancel further processing
							else {
								continue;
							}
						}
					}

					IO::Log(IO::debug, "%s recognized as macro symbol", strLabel.c_str());
					continue;				// cancel further processing
				}

				// Decode symbol offset
				uint32_t offset = 0;
				for (uint8_t* c = sLineOffset; *c; c++) {
					offset = offset*0x10 + (((unsigned)(*c-'0')<10) ? (*c-'0') : (*c-('A'-10)));
				}

				// Add label to the symbols table, if:
				//	1) Its absolute offset is higher than the previous offset successfully added
				//	2) When base offset is subtracted and the mask is applied, the resulting offset is within allowed boundaries
				if ((lastSymbolOffset == (uint32_t)-1) || (offset >= lastSymbolOffset)) {
					const bool inserted = symbolTable.add(offset, strLabel);
					if (inserted) {
						lastSymbolOffset = offset;
					}
				}
				else {
					IO::Log( IO::debug, "Symbol %s at offset %X ignored: its offset is less than the previous symbol successfully fetched", strLabel.c_str(), offset );
				}
			}
		}

		// Undefine conditions, so they can be redefined in other format handlers
		#undef IS_HEX_CHAR
		#undef IS_START_OF_NAME
		#undef IS_NAME_CHAR
		#undef IS_START_OF_LABEL
		#undef IS_LABEL_CHAR
		#undef IS_WHITESPACE
		#undef IS_ENDOFLINE
	}
};
