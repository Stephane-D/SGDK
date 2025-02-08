
/* ------------------------------------------------------------ *
 * ConvSym utility version 2.12									*
 * Output wrapper for the Debug Information format version 2.0	*
 * ------------------------------------------------------------	*/

#include <cassert>
#include <map>
#include <cstdint>
#include <string>
#include <vector>

#include <OptsParser.hpp>
#include <Huffman.hpp>
#include <BitStream.hpp>
#include <IO.hpp>
#include <utils.hpp>

#include "OutputWrapper.hpp"


struct Output__Deb2 : public OutputWrapper {

	Output__Deb2() {};
	~Output__Deb2() {};

	/**
	 * Main function that generates the output
	 */
	void parse(
		std::multimap<uint32_t, std::string>& symbols,
		const char * fileName,
		uint32_t appendOffset = 0,
		uint32_t pointerOffset = 0,
		const char * opts = "",
		bool alignOnAppend = true
	) {
		assert(!symbols.empty());

		auto output = OutputWrapper::setupOutput( fileName, appendOffset, pointerOffset, alignOnAppend );

		/* Parse options from "-inopt" agrument's value */
		bool optFavorLastLabels = false;

		const std::map<std::string, OptsParser::record>
			OptsList {
				{ "favorLastLabels",	{ .type = OptsParser::record::p_bool,	.target = &optFavorLastLabels	} }
			};
		
		OptsParser::parse( opts, OptsList );

		/* Write format version token */
		output->writeBEWord(0xDEB2);

		/* Allocate space for blocks offsets table */
		auto lastSymbolPtr = symbols.rbegin();
		uint16_t lastBlock = (lastSymbolPtr->first) >> 16;

		if (lastBlock > 0xFF) {		// blocks index table is limited to $100 entries (which is enough to cover all the 24-bit addressable space)
			IO::Log(IO::error, "Too many memory blocks to allocate ($%X), truncating to $100 blocks. Some symbols will be lost.", lastBlock+1);
			lastBlock = 0xFF;
		}

		std::vector<uint32_t> blockOffsets(lastBlock+1);
		const size_t blockOffsetsSize = blockOffsets.size() * sizeof(uint32_t);

		output->writeBEWord( blockOffsetsSize + 2 );				// write relative pointer to the Huffman table
		uint32_t loc_BlockOffsets = output->getCurrentOffset();	// remember the offset where blocks offset table should start
		output->setOffset( blockOffsetsSize, IO::current );		// reserve space to write down offsets table later

		/* ------------------------------------------------ */
		/* Generate Huffman-codes and create decoding table */
		/* ------------------------------------------------ */

		IO::Log( IO::debug, "Building an encoding table...");

		/* Generate table of character frequencies based on symbol names */
		uint32_t freqTable[0x100] = { 0 };
		for (auto& symbol : symbols) {
			for (auto& character : symbol.second) {
				freqTable[(int)character]++;
			}
			freqTable[0x00]++;	// include null-terminator
			// TODOh: Guess whether NULL will be appended to the string of specified length
		}

		/* Generate table of Huffman codes (sorted by code) */
		Huffman::RecordSet codesTable = Huffman::encode( freqTable );

		/* Write down the decoding table */
		const Huffman::Record* characterToRecord[0x100] = { nullptr };	// LUT that links each character to its Huffman-coding record
		for (auto& entry : codesTable) {
			if (entry.codeLength > 16) {
				const char * errorMessage = "Some encoding table code lengths exceed 16 bits, try -tolower or -toupper option to reduce entropy";
				throw errorMessage;
			}

			characterToRecord[entry.data] = &entry;		// assign character this Huffman::Record entity
			output->writeBEWord(entry.code);			// write Huffman-code
			output->writeByte(entry.codeLength);		// write Huffman-code length (in bits)
			output->writeByte(entry.data);				// write down original character
		}
		output->writeWord(-1);			// write 0xFFFF at the end of Huffman-table to stop searching and cause error

		/* ------------------------------------- */
		/* Generate per block symbol information */
		/* ------------------------------------- */

		{
			IO::Log(IO::debug, "Generating symbol data blocks...");

			auto symbolPtr = symbols.begin();
			struct SymbolRecord { uint16_t offset; uint16_t symbolDataPtr; };

			/* For 64kb block within symbols range */
			for (uint16_t block = 0x00; block <= lastBlock; block++) { 
				/* Align block on even address */
				if (output->getCurrentOffset() & 1) {
					output->writeByte(0x00);
				}

				uint32_t loc_Block = output->getCurrentOffset();	// remember offset, where this block starts ...

				BitStream SymbolsHeap;
				std::vector<SymbolRecord> offsetsData;

				/* For every symbol within the block ... */
				for (; (symbolPtr->first>>16) <= block && (symbolPtr != symbols.cend()); ++symbolPtr) {
					if ((symbolPtr->first>>16) < block) {
						continue;
					}

					/* 
					 * For records with the same offsets, fetch only the last or the first processed symbol,
					 * depending "favor last labels" option ...
					 */
					if ((optFavorLastLabels && std::next(symbolPtr) != symbols.end()
							&& std::next(symbolPtr)->first == symbolPtr->first) ||
						 (!optFavorLastLabels && symbolPtr != symbols.begin()
							&& std::prev(symbolPtr)->first == symbolPtr->first)
					) {
						continue;
					}

					if (SymbolsHeap.getCurrentPos() > 0xFFFF) {
						IO::Log(IO::error,"Symbols heap for block %02X exceeded 64kb limit, no more symbols can be stored in this block.", block);
						break;
					}
					
					else if (offsetsData.size() > 0x3FFF) {
						IO::Log(IO::error, "Too many symbols in block %02X, no more symbols can be stored in this block.", block);
						break;
					}

					else {
						/* Generate symbol structure, that includes offset and encoded symbol text pointer */
						offsetsData.push_back({
							.offset = (uint16_t)(symbolPtr->first & 0xFFFF),
							.symbolDataPtr = (uint16_t)SymbolsHeap.getCurrentPos()
						});
	
						/* Encode each symbol character with the generated Huffman-codes and store it in the bitsteam */
						for (auto& character : symbolPtr->second) {
							auto *record = characterToRecord[ (int)character ];
							SymbolsHeap.pushCode( record->code, record->codeLength );
						}
						
						/* Finally, add null-terminator */
						{
							auto *record = characterToRecord[ 0x00 ];
							SymbolsHeap.pushCode( record->code, record->codeLength );
							SymbolsHeap.flush();
						}
					}

				}

				/* Write offsets block and their corresponding encoded symbols heap */
				if ( offsetsData.size() > 0 ) {
					IO::Log(IO::debug,
						"Block %02X: %d bytes (offsets list: %d bytes, symbols heap: %d bytes)",
						block, 2+offsetsData.size()*4+SymbolsHeap.size(), offsetsData.size()*4, SymbolsHeap.size()
					);

					/* Insert pointer to the end of the list (where heap starts) */
					output->writeBEWord(2 + offsetsData.size()*4);		// write down offset

					/* Write down records that store offset and symbol pointer */
					for (auto& entry : offsetsData) {
						output->writeBEWord(entry.offset);			// write down offset
						output->writeBEWord(entry.symbolDataPtr);	// write down expected relative pointer to the heap
					}

					output->writeData(SymbolsHeap.begin(), SymbolsHeap.size());
					blockOffsets[block] = swap32((loc_Block-loc_BlockOffsets));
				}
			}
		}
		
		/* Finally, write down block offsets table in the header */
		output->setOffset(loc_BlockOffsets, IO::start);
		output->writeData(blockOffsets.data(), blockOffsetsSize);
	};

};
