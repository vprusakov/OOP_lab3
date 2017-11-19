#pragma once
#include <exception>
#include <string>

class WavException : public std::exception {
public:
	WavException(const std::string &excMessage) : message(excMessage) {}
	std::string what() {
		return message;
	}
	~WavException() throw() {}
private:
	std::string message;
};

class ExcIO : public WavException {
public:
	ExcIO(const std::string& filename) : WavException("File " + filename + " can't be read.\n") {};
};
class ExcBadFormat : public WavException {
public:
	ExcBadFormat(const std::string& filename) : WavException("Incorrect format of " + filename + ".\n") {};
};
class ExcHeader : public WavException {
public:
	ExcHeader(const std::string& msg) : WavException(msg) {};
};
class FileIncorrectFormat : public WavException {
public:
	FileIncorrectFormat(const std::string& msg) : WavException(msg) {};
};