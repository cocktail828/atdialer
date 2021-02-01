#ifndef __RG801H_AT__
#define __RG801H_AT__

#include <string>
#include <iostream>
#include <sstream>

#include "at_command.hpp"
#include "atenum.hpp"

class Hisilicon final : public ATCommand
{
    static const std::string endstr[3];

public:
    Hisilicon(const std::string &apn, const std::string &usr, const std::string &passwd,
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
        expect_state = ATExpectResp::EXPT_DATA;
        return std::string("AT+CPIN?\r\n");
    }

    /**
     * build an AT command to query SIM state
     */
    std::string newSetPincode()
    {
        expect_state = ATExpectResp::EXPT_OK;
        return std::string("AT+CPIN=") +
               safety_string(pincode) + "\r\n";
    }

    /**
     * build an AT command to query registration state
     */
    std::string newQueryRegisterinfo()
    {
        expect_state = ATExpectResp::EXPT_DATA;
        return std::string("AT+CEREG?\r\n");
    }

    /**
     * build an AT command to config IP Protocol, APN, USR, PASSWD, AUTH
     */
    std::string newATConfig()
    {
        expect_state = ATExpectResp::EXPT_OK;
        auto reqstr = std::string("AT+QICSGP=") +
                      std::to_string(contexid) + "," +
                      std::to_string(static_cast<int>(ipproto));

        if (!apn.empty())
        {
            reqstr += "," + apn;
            if (!user.empty())
            {
                reqstr += "," + user;
                if (!passwd.empty())
                {
                    reqstr += "," + passwd;
                    reqstr += std::to_string(static_cast<int>(auth));
                }
            }
        }

        reqstr += "\r\n";
        return reqstr;
    }

    /**
     * build an AT command to query data connection state
     */
    std::string newQueryDataConnectinfo()
    {
        expect_state = ATExpectResp::EXPT_OK;
        return std::string("AT+QNETDEVSTATUS=") +
               std::to_string(contexid) + "\r\n";
    }

    /**
     * build an AT command to setup data call state
     */
    std::string newSetupDataCall()
    {
        expect_state = ATExpectResp::EXPT_OK;
        return std::string("AT+QNETDEVCTL=") +
               std::to_string(contexid) + "," +
               std::to_string(1) + "," +
               std::to_string(autoconnect) + "\r\n";
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
                if (line.find("READY") != std::string::npos)
                    newstate = machine_state::STATE_SIM_READY;
                else if (line.find("SIM PIN") != std::string::npos)
                    newstate = machine_state::STATE_SIM_NEED_PIN;
                else
                    newstate = machine_state::STATE_START;
            }
            else if (line.find("+CEREG:") != std::string::npos)
            {
                int n, stat;
                sscanf(line.c_str(), "+CEREG: %d,%d", &n, &stat);
                if (stat == 1)
                    newstate = machine_state::STATE_REGISTERED;
            }
            else if (line.find("+QNETDEVSTATUS:") != std::string::npos)
            {
                uint32_t _contexid, _op;

                sscanf(line.c_str(), "+QNETDEVSTATUS:%x,%x", &_contexid, &_op);
                if (_contexid == (uint32_t)contexid)
                {
                    // +QNETDEVSTATUS:1,0,"IPV4"
                    newstate = _op ? machine_state::STATE_CONNECT : machine_state::STATE_DISCONNECT;
                }
                else if (_contexid > 0xff)
                {
                    // +QNETDEVSTATUS: 1802F70A,F0FFFFFF,1102F70A,1102F70A,024E68DA,00000000, 85600, 85600
                    newstate = machine_state::STATE_CONNECT;
                }
            }
        }

        return newstate;
    }
};

const std::string Hisilicon::endstr[3] = {
    "OK",
    "+CME ERROR",
    "ERROR",
};

#endif //__RG801H_AT__
