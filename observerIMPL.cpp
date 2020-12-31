#include <thread>
#include <iostream>
#include <sstream>

#include "observerIMPL.hpp"
#include "rg801h.hpp"

std::mutex ttyClient::usrlock;
std::condition_variable ttyClient::usrcond;
machine_state ttyClient::state = machine_state::STATE_START;
void ttyClient::sendCommand(std::string &&cmd)
{
    static std::string precmd;
    static int times = 1;

    if (precmd == cmd)
        std::this_thread::sleep_for(std::chrono::seconds(times));

    precmd = cmd;

    if (ttyreader->ready())
    {
        std::cerr << "SEND>> " << cmd.substr(0, cmd.size() - 2) << std::endl;
        ttyreader->sendAsync(cmd);
    }
    else
    {
        std::cerr << "CANNOT SEND>> " << cmd.substr(0, cmd.size() - 2) << std::endl;
    }
}

void ttyClient::update()
{
    ttyreader = nullptr;
    usrcond.notify_all();
}

void ttyClient::update(const std::string &respstr)
{
    std::string line;
    std::stringstream ss(respstr);

    while (getline(ss, line))
    {
        // skip '\n'
        line = line.substr(0, line.size() - 1);
        if (line.size())
        {
            atrespstrlist.push_back(line);
            if (ATCommand::atCommandEnd(line))
            {
                // std::cerr << "cond notify" << std::endl;
                usrcond.notify_all();
            }
        }
    }
}

/**
 * state machine process URC or Response and change it's state
 */
void ttyClient::start_machine(const char *apn, const char *usr, const char *passwd, AUTH auth, IPPROTO iptype, int cid)
{
    do
    {
        std::unique_lock<std::mutex> _lk(usrlock);

        // std::cerr << "current state: " << static_cast<int>(state) << std::endl;
        switch (state)
        {
        case machine_state::STATE_START:
        {
            sendCommand(ATCommand::newQuerySIMinfo());
            break;
        }

        case machine_state::STATE_SIM_READY:
        {
            sendCommand(ATCommand::newQueryRegisterinfo());
            break;
        }

        case machine_state::STATE_REGISTERED:
        {
            sendCommand(ATCommand::newATConfig());
            break;
        }

        case machine_state::STATE_CONFIG_DONE:
        {
            sendCommand(ATCommand::newQueryDataConnectinfo());
            break;
        }

        case machine_state::STATE_DISCONNECT:
        {
            sendCommand(ATCommand::newSetupDataCall());
            break;
        }

        case machine_state::STATE_CONNECT:
        {
            sendCommand(ATCommand::newQueryDataConnectinfo());
            break;
        }

        default:
        {
            break;
        }
        }

        if (usrcond.wait_for(_lk, std::chrono::seconds(30)) == std::cv_status::timeout)
        {
            std::cerr << "TIMEOUT(30s) CMD: " << atreqstr << std::endl;
            continue;
        }

        if (!ttyreader->ready())
        {
            std::cerr << "machine stop for reader is not ready" << std::endl;
            break;
        }

        std::vector<std::string> vecstr;
        for (auto iter = atrespstrlist.begin(); iter != atrespstrlist.end(); iter++)
        {
            vecstr.push_back(*iter);
            if (ATCommand::atCommandEnd(*iter))
            {
                machine_state new_state = ATCommand::parserResp(vecstr);

                if (state == machine_state::STATE_REGISTERED &&
                    !ATCommand::isUnsocial() && ATCommand::isSuccess())
                    state = machine_state::STATE_CONFIG_DONE;

                else if (state == machine_state::STATE_CONFIG_DONE &&
                         !ATCommand::isUnsocial() && !ATCommand::isSuccess())
                    state = machine_state::STATE_DISCONNECT;

                else if (state != new_state &&
                         new_state != machine_state::STATE_INVALID)
                    state = new_state;

                atrespstrlist.erase(atrespstrlist.begin(), ++iter);
                break;
            }
        }

        if (state == machine_state::STATE_CONNECT)
            std::cerr << "trigger DHCP operation" << std::endl;
    } while (1);
}
