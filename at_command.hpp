#ifndef __INTERFACE_AT__
#define __INTERFACE_AT__

#include <string>
#include <iostream>

#include "state_machine.hpp"

class ATCommand
{
public:
    /**
     * indicate AT command end
     */
    static bool atCommandEnd(const std::string &str)
    {
        if (str.find("+CME ERROR") != std::string::npos ||
            str.find("OK") != std::string::npos)
            return true;
        return false;
    }

    /**
     * build an AT command to query SIM state
     */
    static std::string newQuerySIMinfo()
    {
        /**
         * on error: +CME ERROR: 10
         * on success: +CPIN: READY
         */
        return std::string("at+cpin?\r\n");
    }

    /**
     * build an AT command to query registration state
     */
    static std::string newQueryRegisterinfo()
    {
        return std::string("at+creg?\r\n");
    }

    /**
     * build an AT command to query data connection state
     */
    static std::string newQueryDataConnectinfo()
    {
        return std::string("at+qq?\r\n");
    }

    /**
     * build an AT command to setup data call state
     */
    static std::string newSetupDataCall()
    {
        return std::string("at+qnetdevctl=1,1,1\r\n");
    }

    /**
     * parser AT command, return a machine_state
     */
    static machine_state parserResp(const std::string &respstr)
    {
        std::cerr << "respstr: " << respstr << std::endl;
        if (respstr.find("+CPIN: READY") != std::string::npos)
        {
            return machine_state::STATE_SIM_READY;
        }
        else if (respstr.find("+CREG:") != std::string::npos)
        {
            return machine_state::STATE_CONNECT;
        }
        else
        {
            return machine_state::STATE_INVALID;
        }
    }
};
#endif //__INTERFACE_AT__
