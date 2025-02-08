#pragma once

#include <map>
#include <cstdint>
#include <memory>
#include <string>

#include <IO.hpp>


/* Base class for the output formats handlers */
struct OutputWrapper {

	OutputWrapper() { }
	virtual ~OutputWrapper() { }

	// Function to setup output
	static std::unique_ptr<IO::FileOutput> setupOutput( const char * fileName, int32_t appendOffset, int32_t pointerOffset, bool alignOnAppend ) {

		// If append offset was specified, don't overwrite the contents of file
		if ( appendOffset != 0 ) {
			std::unique_ptr<IO::FileOutput> output = std::make_unique<IO::FileOutput>(fileName, IO::append);

			// Make sure IO operation was successful
			if (!output->good()) {
				IO::Log(IO::fatal, "Couldn't open file \"%s\"", fileName);
				throw "IO error";
			}

			// If append mode is specified: append to the end of file
			if ( appendOffset == -1 ) {
				output->setOffset( 0, IO::end );			// move pointer to the end of file ...
				appendOffset = output->getCurrentOffset();

				// If align on append option is on (default), make sure to pad `appendOffset` if it's not even
				if (alignOnAppend && ((appendOffset & 1) != 0)) {
					IO::Log(IO::debug, "Auto-aligning append offset.");
					output->writeByte(0);
					appendOffset++;
				}
			}
			else {
				if (alignOnAppend && ((appendOffset & 1) != 0)) {
					IO::Log(IO::warning, "An odd append offset is specified; the offset wasn't auto-aligned.");
				}

				output->setOffset( appendOffset );		// move pointer to the specified append offset
			}

			// If pointer offset is specified
			if ( pointerOffset != 0 ) {
				output->setOffset( pointerOffset );
				output->writeBELong( appendOffset );
			}

			// Treat "appendOffset" as the base offset from now on ...
			output->setBaseOffset( appendOffset );
			output->setOffset( 0 );						// move to the start of appending section ...

			return output;
		}

		// Otherwise, discard file contents if exists
		else {
			return std::make_unique<IO::FileOutput>(fileName);
		}

	}

	// Virtual function interface that handles generating output data
	virtual void parse( 
		std::multimap<uint32_t, std::string>& SymbolMap, 
		const char * fileName, 
		uint32_t appendOffset = 0, 
		uint32_t pointerOffset = 0,
		const char * opts = "",
		bool alignOnAppend = true
	) = 0;

};
