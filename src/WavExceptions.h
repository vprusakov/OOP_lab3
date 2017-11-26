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

class IO_Exception : public WavException {
public:
	IO_Exception(const std::string& filename) : WavException("File " + filename + " can't be read.\n") {};
};
class Format_Exception : public WavException {
public:
	Format_Exception(const std::string& msg) : WavException(msg) {};
};
class Header_Exception : public WavException {
public:
	Header_Exception(const std::string& msg) : WavException(msg) {};
};
class Parameters_Exception : public WavException {
public:
	Parameters_Exception(const std::string& msg) : WavException(msg) {};
};