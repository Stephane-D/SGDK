
/* ------------------------------------------------------------ *
 * Debugging Modules Utilities Core								*
 * Basic Input / Output wrapper 								*
 * (c) 2017-2018, Vladikcomper									*
 * ------------------------------------------------------------	*/

#pragma once

#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cstdint>


namespace IO {

	/* -------------------- */
	/* Function for logging */
	/* -------------------- */
	
	/* Logging levels */
	enum eLogLevel{ debug, warning, error, fatal }
		LogLevel = warning;

	void Log(eLogLevel level, const char * format, ...) {
		if ( level >= LogLevel ) {
			const char* levelText[] = {
				"", "WARNING: ", "ERROR: ", "FATAL: "
			};
			fputs( levelText[level], stderr );
			va_list args;
			va_start (args, format);
			vfprintf (stderr, format, args);
			va_end (args);
			fputs("\n", stderr);	// add a newline
		}
	}

	/* -------------------------- */
	/* Base class for Binary file */
	/* -------------------------- */

	/* Access mode enumeration */
	enum eMode {
		read = 0,
		write = 1,
		text = 2,
		append = 4
	};

	/* Seeking modes */
	enum eSeekOrigin {
		start = 0,
		end = 1,
		current = 2
	};

	struct File {

		File(const char * path, int mode): baseOffset(0) {	// Constructor
			const char* modeToCode[] = { "rb", "wb", "r", "w", "r+b", "r+b", "r+", "r+" };

			if ((strncmp(path, "-", 2) == 0) && ((mode & 1) == read || (mode & 1) == write)) {
				if ((mode & 1) == read) {
					file = stdin;
				}
				else {
					file = stdout;
				}
			}
			else {
				file = fopen( path, modeToCode[ mode ] );
			}
		}
		
		File(): file(nullptr), baseOffset(0) {

		}
		
		virtual ~File() {	// Destructor
			if (file && (file != stdin) && (file != stdout) && (file != stderr)) {
				fclose( file );
				file = nullptr;
			}
		}

		/**
		 * Function to return error state
		 */
		inline bool good() {
			return file != nullptr;
		}

		/**
		 * Function to set specified offset within the file
		 */
		inline void setOffset(uint32_t offset, eSeekOrigin origin = start) {
			const int originToFlag[] = { SEEK_SET, SEEK_END, SEEK_CUR };
			if ( baseOffset && (origin == start) ) {
				fseek( file, baseOffset + offset, originToFlag[ origin ] );
			}
			else {
				fseek( file, offset, originToFlag[ origin ] );
			}
		}
		
		/**
		 * Function to set the base offset for I/O operations within file
		 */
		inline void setBaseOffset(uint32_t offset) {
			baseOffset = offset;
		}

		/**
		 * Function to get current offset in the file
		 */
		inline uint32_t getCurrentOffset() {
			return ftell( file ) - baseOffset;
		}

	protected:
		FILE* file;
		uint32_t baseOffset;

	};

	/* Class for binary file output */
	struct FileOutput : File {

		FileOutput(const char * path, int mode = 0) : File(path, write|mode) {};

		inline void writeByte(const uint8_t& byte) {	// write byte
			fputc((int)byte, file);
		}

		inline void writeWord(const uint16_t& word) {	// write word (unmodified)
			fwrite(&word, 2, 1, file);
		}

		inline void writeBEWord(uint16_t word) {	// write word (LE to BE conversion)
			word = (word<<8) | (word>>8);
			fwrite(&word, 2, 1, file);
		}

		inline void writeLong(const uint32_t& lword) {	// write long (unmodified)
			fwrite(&lword, 4, 1, file);
		}

		inline void writeBELong(uint32_t lword) {	// write long (LE to BE conversion)
			lword = (lword<<24) | ((lword<<8)&0xFF0000) | ((lword>>8)&0xFF00) | (lword>>24);
			fwrite(&lword, 4, 1, file);
		}
		
		inline void writeData(const void * buffer, int size) {	// write series of data
			fwrite((char*)buffer, 1, size, file);
		};

		inline void putString(const char * str) {				// put *unformatted* string
			fputs(str, file);
		}

		inline void putLine(const char * str) {				// put *unformatted* string
			fputs( str, file );
			fputc( '\n', file );
		}

		inline void writeString(const char * format, ...) {	// write a formatted string to file
			va_list args;
			va_start(args, format);
			vfprintf(file, format, args);
			va_end(args);
		}

		inline void writeLine(const char * format, ...) {	// write a formatted string to file
			va_list args;
			va_start(args, format);
			vfprintf(file, format, args);
			va_end(args);
			fputc('\n', file);
		}

	};

	/* Class for binary file input */
	struct FileInput : File {
	
		FileInput(const char * path, int mode = 0) : File(path, read|mode) { };

		inline uint8_t readByte() {		// read byte
			return fgetc(file);
		}

		inline uint16_t readWord() {	// read word (unmodified)
			uint16_t word;
			if (!fread( &word, 2, 1, file)) {
				throw "readWord() failed, possibly end of file reached";
			}
			return word;
		}

		inline uint16_t readBEWord() {	// read word (LE to BE conversion)
			uint8_t buffer[2];
			if (!fread(&buffer, 2, 1, file)) {
				throw "readBEWord() failed, possibly end of file reached";
			}
			return (buffer[0]<<8) + buffer[1];
		}

		inline uint32_t readLong() {	// read long (unmodified)
			uint32_t dword;
			if (!fread(&dword, 4, 1, file)) {
				throw "readLong() failed, possibly end of file reached";
			}
			return dword;
		}

		inline uint32_t readBELong() {	// read long (LE to BE conversion)
			uint8_t buffer[4];
			if (!fread(&buffer, 4, 1, file)) {
				throw "readBELong() failed, possibly end of file reached";
			}
			return (buffer[0]<<24) + (buffer[1]<<16) + (buffer[2]<<8) + buffer[3];
		}
		
		inline void readData(uint8_t* const buffer, int size) {	// read series of data
			if (!fread((char*)buffer, size, 1, file)) {
				throw "readData() failed, possibly end of file reached";
			}
		};

		inline int readLine(uint8_t* const buffer, int size, bool trim_ending = true) {	// read line from file and strip newline characters
			if (fgets((char*)buffer, size, file) != nullptr) {
				int line_length;
				if ( trim_ending ) {
					line_length = strcspn((char*)buffer, "\r\n");
					buffer[line_length] = 0x00;
				}
				else {
					line_length = strlen((char*)buffer);
				}
				return line_length;
			}
			else {
				return -1;
			}
		}
	};
}