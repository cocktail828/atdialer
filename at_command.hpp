/*
 * @Author: sinpo828
 * @Date: 2021-01-22 13:22:01
 * @LastEditors: sinpo828
 * @LastEditTime: 2021-02-01 11:37:36
 * @Description: file content
 */
#ifndef __AT_COMMAND__
#define __AT_COMMAND__

#include <string>
#include <vector>

#include "atenum.hpp"
#include "state_machine.hpp"

enum class ATExpectResp
{
    EXPT_NONE, // the request does not care about response or whether it's success or not
    EXPT_OK,   // the request expect just OK response and do not care about response data
    EXPT_DATA, // the request expect a response with data
};

class ATCommand
{
protected:
    bool bunsocial;
    bool bsuccess;

    int contexid;     // contex id
    bool autoconnect; // autoconnect?
    IPPROTO ipproto;  // 1 ipv4, 2 ipv6, 3 ipv4v6
    std::string apn;
    std::string user;
    std::string passwd;
    AUTH auth; // 0 None, 1 PAP, 2 CHAP, 3 PAP or CHAP
    std::string pincode;

    /* expect response */
    ATExpectResp expect_state;

public:
    ATCommand() : bunsocial(false),
                  bsuccess(false),
                  contexid(1),
                  autoconnect(true),
                  ipproto(IPPROTO::PROTO_IPV4),
                  apn(""),
                  user(""),
                  passwd(""),
                  auth(AUTH::AUTH_NONE),
                  pincode(""),
                  expect_state(ATExpectResp::EXPT_NONE) {}

    ATCommand(const std::string &apn,
              const std::string &usr, const std::string &passwd,
              AUTH auth, const std::string &pincode, IPPROTO iptype, int cid)
        : bunsocial(false),
          bsuccess(false),
          contexid(cid),
          autoconnect(true),
          ipproto(iptype),
          apn(apn),
          user(usr),
          passwd(passwd),
          auth(auth),
          pincode(pincode),
          expect_state(ATExpectResp::EXPT_NONE) {}

    virtual ~ATCommand() {}

    const std::string safety_string(const std::string &str)
    {
        return str.empty() ? "\"\"" : str;
    }

    /**
     * indicate the message is a response for a request
     */
    bool isUnsocial()
    {
        return bunsocial;
    }

    /**
     * indicate the request is success or not
     */
    bool isSuccess()
    {
        return bsuccess;
    }

    ATExpectResp get_expt_state()
    {
        return expect_state;
    }

    /**
     * indicate AT command end
     */
    virtual bool atCommandEnd(const std::string &str) = 0;

    /**
     * build an AT command to query SIM state
     */
    virtual std::string newQuerySIMinfo() = 0;

    /**
     * build an AT command to query SIM state
     */
    virtual std::string newSetPincode() = 0;

    /**
     * build an AT command to query registration state
     */
    virtual std::string
    newQueryRegisterinfo() = 0;

    /**
     * build an AT command to config IP Protocol, APN, USR, PASSWD, AUTH
     */
    virtual std::string newATConfig() = 0;
    /**
     * build an AT command to query data connection state
     */
    virtual std::string newQueryDataConnectinfo() = 0;

    /**
     * build an AT command to setup data call state
     */
    virtual std::string newSetupDataCall() = 0;

    /**
     * parser AT command, return a machine_state
     */
    virtual machine_state parserResp(const std::vector<std::string> &vecstr) = 0;
};

#endif //__AT_COMMAND__
