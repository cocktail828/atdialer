#ifndef __INTERFACE_AT__
#define __INTERFACE_AT__

#include <string>
#include <iostream>
#include <sstream>

#include "atenum.hpp"
#include "state_machine.hpp"

const std::string endstr[] = {
    "OK",
    "+CME ERROR",
    "ERROR",
};

class ATCommand
{
    static bool bunsocial;
    static bool bsuccess;
    static int cid;       // contex id
    static int op;        // operate
    static bool state;    // autoconnect?
    static IPPROTO ctype; // 1 ipv4, 2 ipv6, 3 ipv4v6
    static std::string apn;
    static std::string user;
    static std::string passwd;
    static AUTH auth; // 0 None, 1 PAP, 2 CHAP, 3 PAP or CHAP

public:
    /**
     * indicate the message is a response for a request
     */
    static bool isUnsocial()
    {
        return bunsocial;
    }

    /**
     * indicate the request is success or not
     */
    static bool isSuccess()
    {
        return bsuccess;
    }

    /**
     * indicate AT command end
     */
    static bool atCommandEnd(const std::string &str)
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
    static std::string newQuerySIMinfo()
    {
        /**
         * on error: +CME ERROR: 10
         * on success: +CPIN: READY
         */
        return std::string("AT+CPIN?\r\n");
    }

    /**
     * build an AT command to query registration state
     */
    static std::string newQueryRegisterinfo()
    {
        return std::string("AT+CEREG?\r\n");
    }

    /**
     * build an AT command to config IP Protocol, APN, USR, PASSWD, AUTH
     */
    static std::string newATConfig()
    {
        return std::string("AT+QICSGP=") +
               std::to_string(cid) + "," +
               std::to_string(static_cast<int>(ctype)) + "," +
               "\"\"\r\n";
    }

    /**
     * build an AT command to query data connection state
     */
    static std::string newQueryDataConnectinfo()
    {
        return std::string("AT+QNETDEVSTATUS=") +
               std::to_string(cid) + "\r\n";
    }

    /**
     * build an AT command to setup data call state
     */
    static std::string newSetupDataCall()
    {
        return std::string("AT+QNETDEVCTL=") +
               std::to_string(cid) + "," +
               std::to_string(op) + "," +
               std::to_string(state) + "\r\n";
    }

    /**
     * parser AT command, return a machine_state
     */
    static machine_state parserResp(const std::vector<std::string> &vecstr)
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
                else if (line.find("NOT INSERTED"))
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
                int _cid, _op;

                sscanf(line.c_str(), "+QNETDEVSTATUS:%d,%d", &_cid, &_op);
                if (_cid == cid)
                    return _op ? machine_state::STATE_CONNECT : machine_state::STATE_DISCONNECT;
            }
        }

        return newstate;
    }
};

bool ATCommand::bunsocial;
bool ATCommand::bsuccess;
int ATCommand::cid = 1;
int ATCommand::op = 1;
bool ATCommand::state = true;
IPPROTO ATCommand::ctype = IPPROTO::PROTO_IPV4;
std::string ATCommand::apn;
std::string ATCommand::user;
std::string ATCommand::passwd;
AUTH ATCommand::auth = AUTH::AUTH_NONE;
#endif //__INTERFACE_AT__
