#ifndef __ASR_AT__
#define __ASR_AT__

#include <string>
#include <iostream>
#include <sstream>

#include "at_command.hpp"
#include "atenum.hpp"

class ASR final : public ATCommand
{
    static const std::string endstr[3];

public:
    ASR(const std::string &apn, const std::string &usr, const std::string &passwd,
        AUTH auth, const std::string &pincode, IPPROTO iptype, int cid)
        : ATCommand(apn, usr, passwd, auth, pincode, iptype, cid) {}

    /**
     * indicate AT command end
     */
    bool atCommandEnd(const std::string &str)
    {
        bsuccess = false;
        for (unsigned long idx = 0; idx < sizeof(endstr) / sizeof(endstr[0]); idx++)
        {
            std::string es = endstr[idx];
            if (!str.compare(0, es.size(), es))
            {
                if (es == "OK")
                    bsuccess = true;
                return true;
            }
        }
        return false;
    }

    /**
     * build an AT command to query SIM state
     */
    std::string newQuerySIMinfo()
    {
        /**
         * on error: +CME ERROR: 10
         * on success: +CPIN: READY
         */
        return std::string("AT+CPIN?\r\n");
    }

    /**
     * build an AT command to query SIM state
     */
    std::string newSetPincode()
    {
        return std::string("AT+CPIN=") +
               safety_string(pincode) + "\r\n";
    }

    /**
     * build an AT command to query registration state
     */
    std::string newQueryRegisterinfo()
    {
        return std::string("AT+CEREG?\r\n");
    }

    /**
     * build an AT command to config IP Protocol, APN, USR, PASSWD, AUTH
     */
    std::string newATConfig()
    {
        return std::string("AT+QICSGP=") +
               std::to_string(contexid) + "," +
               std::to_string(static_cast<int>(ipproto)) + "," +
               safety_string(apn) + "," +
               safety_string(user) + "," +
               safety_string(passwd) + "," +
               std::to_string(static_cast<int>(auth)) + "\r\n";
    }

    /**
     * build an AT command to query data connection state
     */
    std::string newQueryDataConnectinfo()
    {
        return std::string("AT+QNETDEVCTL?\r\n");
    }

    /**
     * build an AT command to setup data call state
     */
    std::string newSetupDataCall()
    {
        return std::string("AT+QNETDEVCTL=") +
               std::to_string(1) + "," +
               std::to_string(contexid) + "," +
               std::to_string(1) + "1\r\n";
    }

    /**
     * parser AT command, return a machine_state
     */
    machine_state parserResp(const std::vector<std::string> &vecstr)
    {
        machine_state newstate = machine_state::STATE_INVALID;

        bunsocial = true;
        if (vecstr.size() && vecstr[0].compare(0, 3, "AT+") == 0)
            bunsocial = false;

        for (auto iter = vecstr.begin(); iter != vecstr.end(); iter++)
        {
            std::string line = *iter;
            std::cerr << "RECV<< " << line << std::endl;
            if (line.find("+CPIN:") != std::string::npos)
            {
                if (line.find("READY"))
                    newstate = machine_state::STATE_SIM_READY;
                else
                    return machine_state::STATE_START;
            }
            else if (line.find("+CEREG:") != std::string::npos)
            {
                int n, stat;
                sscanf(line.c_str(), "+CEREG: %d,%d", &n, &stat);
                if (stat == 1)
                    return machine_state::STATE_REGISTERED;
            }
            else if (line.find("+QNETDEVSTATUS:") != std::string::npos)
            {
                int _op;

                sscanf(line.c_str(), "+QNETDEVSTATUS:%d", &_op);
                return _op ? machine_state::STATE_CONNECT : machine_state::STATE_DISCONNECT;
            }
        }

        return newstate;
    }
};
const std::string ASR::endstr[3] = {
    "OK",
    "+CME ERROR",
    "ERROR",
};

#endif //__ASR_AT__
