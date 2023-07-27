#pragma once 
#include <exception>
#include <string>

class ACException : public std::exception
{
public:
	ACException(std::string str = "default exception"):m_except(str) {}
	
	virtual const char* what() const throw() {
		return m_except.c_str();
	}

private:
	std::string m_except;
};
