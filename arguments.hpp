/*
 * arguments.hpp
 *
 *  Created on: 06.05.2010
 *      Author: gmueller
 */

#include <cstdlib>
#include <string>
#include <vector>

class Arguments {
	int argc;
	const char **argv;
public:
	Arguments(int c, const char **v) :
		argc(c), argv(v) {

	}

	int getCount() {
		return argc;
	}

	bool hasFlag(const std::string &flag) {
		int i;
		for (i = 0; i < argc; i++) {
			if (flag == argv[i])
				return true;
		}

		return false;
	}

	int getInt(const std::string &flag, int def) {
		int i;
		for (i = 0; i < argc; i++) {
			if (flag == argv[i] && (i + 1 < argc))
				return std::atoi(argv[i + 1]);
		}

		return def;
	}

	int getFloat(const std::string &flag, float def) {
		int i;
		for (i = 0; i < argc; i++) {
			if (flag == argv[i] && (i + 1 < argc))
				return std::atof(argv[i + 1]);
		}

		return def;
	}
	std::string getString(const std::string &flag, const std::string &def) {
		int i;
		for (i = 0; i < argc; i++) {
			if (flag == argv[i] && (i + 1 < argc))
				return std::string(argv[i + 1]);
		}

		return def;
	}

	void getVector(const std::string &flag, std::vector<std::string> &v) {
		int i;

		// find flag
		for (i = 0; i < argc; i++) {
			if (flag == argv[i])
				break;
		}

		for (i++; i < argc; i++) {
			if (argv[i][0] != '-')
				v.push_back(argv[i]);
		}

	}

};
