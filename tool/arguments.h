#include <cstdlib>
#include <string>
#include <vector>

class Arguments {
	std::vector<std::string> arguments;
public:
	Arguments() {
	}

	Arguments(size_t c, const char **v) {
		for (size_t i = 0; i < c; i++)
			arguments.push_back(v[i]);
	}

	const std::string &get(size_t i) {
		return arguments[i];
	}

	void add(const std::string &argument) {
		arguments.push_back(argument);
	}

	int getCount() {
		return arguments.size();
	}

	bool hasFlag(const std::string &flag) {
		size_t i;
		for (i = 0; i < arguments.size(); i++) {
			if (flag == arguments[i])
				return true;
		}

		return false;
	}

	int getInt(const std::string &flag, int def) {
		size_t i;
		for (i = 0; i < arguments.size(); i++) {
			if (flag == arguments[i] && (i + 1 < arguments.size()))
				return std::atoi(arguments[i + 1].c_str());
		}

		return def;
	}

	float getFloat(const std::string &flag, float def) {
		size_t i;
		for (i = 0; i < arguments.size(); i++) {
			if (flag == arguments[i] && (i + 1 < arguments.size()))
				return std::atof(arguments[i + 1].c_str());
		}

		return def;
	}
	std::string getString(const std::string &flag, const std::string &def) {
		size_t i;
		for (i = 0; i < arguments.size(); i++) {
			if (flag == arguments[i] && (i + 1 < arguments.size()))
				return std::string(arguments[i + 1]);
		}

		return def;
	}

	void getVector(const std::string &flag, std::vector<std::string> &v) {
		size_t i;

		// find flag
		for (i = 0; i < arguments.size(); i++) {
			if (flag == arguments[i])
				break;
		}

		for (i++; i < arguments.size(); i++) {
			if (arguments[i][0] != '-')
				v.push_back(arguments[i]);
			else
				break;
		}

	}

};
