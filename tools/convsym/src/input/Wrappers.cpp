
/* ------------------------------------------------------------ *
 * ConvSym utility version 2.12									*
 * Input formats base controller								*
 * ------------------------------------------------------------	*/

#include <algorithm>
#include <map>
#include <string>
#include <memory>
#include <functional>

#include <IO.hpp>

#include "InputWrapper.hpp"

#include "ASM68K_Listing.cpp"
#include "ASM68K_Sym.cpp"
#include "AS_Listing.cpp"
#include "AS_Listing_Experimental.cpp"
#include "Log.cpp"
#include "TXT.cpp"


/* Input wrappers map */
std::unique_ptr<InputWrapper> getInputWrapper(const std::string& name) {
	static const std::map<std::string, std::function<std::unique_ptr<InputWrapper>()> >
	wrapperTable {
		{ "asm68k_sym", 	[]() { return std::unique_ptr<InputWrapper>(new Input__ASM68K_Sym()); } },
		{ "asm68k_lst", 	[]() { return std::unique_ptr<InputWrapper>(new Input__ASM68K_Listing()); } },
		{ "as_lst",			[]() { return std::unique_ptr<InputWrapper>(new Input__AS_Listing()); } },
		{ "as_lst_exp", 	[]() { return std::unique_ptr<InputWrapper>(new Input__AS_Listing_Experimental()); } },
		{ "log", 			[]() { return std::unique_ptr<InputWrapper>(new Input__Log()); } },
		{ "txt", 			[]() { return std::unique_ptr<InputWrapper>(new Input__TXT()); } }
	};

	auto entry = wrapperTable.find(name);
	if (entry == wrapperTable.end()) {
		IO::Log(IO::fatal, "Unknown input format specifier: %s", name.c_str());
		throw "Bad input format specifier";
	}

	return (entry->second)();
}
