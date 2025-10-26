#pragma once

#include <string>
#include <exception>

#include <errno.h>

namespace acc
{

// Covers runtime issues, like lost BT connections, failed Rx/Tx, ...
// To give the application threads the possibility to handle them in a custom way
class BTRuntimeError : public std::exception
{
public:
    BTRuntimeError(std::string const &errorMessage) : 
        m_errorNumber(errno),
        m_errorMessage(errorMessage)
    {}

    const char* what() const noexcept override 
    {
        return m_errorMessage.c_str();
    }

    int errNumber() const noexcept { return m_errorNumber; }

private:
    int m_errorNumber;
    std::string m_errorMessage;
};

} // namespace acc