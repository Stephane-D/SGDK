
/* ------------------------------------------------------------ *
 * Debugging Modules Utilities Core								*
 * Argument values parser helper 								*
 * (c) 2017-2018, Vladikcomper									*
 * ------------------------------------------------------------	*/

#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "IO.hpp"


namespace ArgvParser {

	/* Structure that handles parameter definitions */
	struct record {
		enum { flag, hexNumber, hexRange, string, string_list, custom } type;
		void * target;
		void * target2;
	};

	/**
	 * Function to parse command line arguments (argv, argc) according to parameter data structures
	 */
	inline void parse(const char ** argv, int argc, std::map<std::string,record> ParametersList) {

		for (int i=0; i<argc; ++i) {
			auto parameter = ParametersList.find(argv[i]);
			#define _GET_NEXT_ARGUMENT \
				if (++i==argc) { \
					IO::Log(IO::fatal, "Expected value for parameter \"%s\"", parameter->first.c_str()); \
					break; \
				}

			if (parameter != ParametersList.end()) {
				switch (parameter->second.type) {
					case record::flag:
						*((bool*)parameter->second.target) = true;
						break;

					case record::hexNumber:
						_GET_NEXT_ARGUMENT
						*(unsigned int*)parameter->second.target = std::stoul(argv[i], 0, 16);
						break;

					case record::hexRange:
						_GET_NEXT_ARGUMENT
						*(unsigned int*)parameter->second.target = std::stoul(argv[i], 0, 16);
						_GET_NEXT_ARGUMENT
						*(unsigned int*)parameter->second.target2 = std::stoul(argv[i], 0, 16);
						break;

					case record::string:
						_GET_NEXT_ARGUMENT
						*((std::string*)parameter->second.target) = argv[i];
						break;

					case record::string_list:
						_GET_NEXT_ARGUMENT
						((std::vector<std::string>*)parameter->second.target)->emplace_back(argv[i]);
						break;

					case record::custom:
						_GET_NEXT_ARGUMENT
						(*(std::function<void(const char*, void*)>*)parameter->second.target2)(argv[i], parameter->second.target);

					default:
						throw "Incorrect or broken parameters data";
				}
			}
			else {
				IO::Log(IO::warning, "Unknown parameter \"%s\" passed. Parameter is ignored", argv[i]);
			}

			#undef _GET_NEXT_ARGUMENT
		}
	}
}