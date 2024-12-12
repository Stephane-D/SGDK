
/* ------------------------------------------------------------ *
 * ConvSym utility version 2.12									*
 * Output formats base controller								*
 * ------------------------------------------------------------	*/

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <map>
#include <functional>

#include <IO.hpp>

#include "OutputWrapper.hpp"

#include "DEB1.cpp"
#include "DEB2.cpp"
#include "Log.cpp"
#include "ASM.cpp"


/* Input wrappers map */
std::unique_ptr<OutputWrapper> getOutputWrapper( const std::string& name ) {
	static const std::map<std::string, std::function<std::unique_ptr<OutputWrapper>()>>
	wrapperTable {
		{ "deb1",	[]() { return std::unique_ptr<OutputWrapper>(new Output__Deb1());	} },
		{ "deb2",	[]() { return std::unique_ptr<OutputWrapper>(new Output__Deb2());	} },
		{ "log",	[]() { return std::unique_ptr<OutputWrapper>(new Output__Log());	} },
		{ "asm",	[]() { return std::unique_ptr<OutputWrapper>(new Output__Asm());	} }
	};

	auto entry = wrapperTable.find(name);
	if (entry == wrapperTable.end()) {
		IO::Log(IO::fatal, "Unknown output format specifier: %s", name.c_str());
		throw "Bad output format specifier";
	}

	return (entry->second)();
}
