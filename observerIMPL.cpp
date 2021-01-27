#include <thread>
#include <iostream>
#include <sstream>
#include <fstream>

#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include "observerIMPL.hpp"

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
        std::cerr << "TTY NOT READY, CANNOT SEND>> " << cmd.substr(0, cmd.size() - 2) << std::endl;
    }
}

void ttyClient::update()
{
    std::cerr << "polling thread notify machine it's quit state" << std::endl;
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
            if (pATCmd->atCommandEnd(line))
            {
                // std::cerr << "cond notify" << std::endl;
                usrcond.notify_all();
            }
        }
    }
}

static void run_dhcp(const char *net)
{
    pid_t pid = 0;
    const char *udhcpc_path = "/usr/bin/busybox";
    const char *udhcpc_pidfile = "/tmp/udhcpc.pid";
    std::ifstream fin(udhcpc_pidfile);

    fin >> pid;
    fin.close();
    if (pid && !kill(pid, 0))
    {
        std::cerr << "udhcpc pid(" + std::to_string(pid) + ")" + " do renew operation" << std::endl;
        std::cerr << "udhcpc renew operation: " << (kill(pid, SIGUSR1) ? "fail" : "success") << std::endl;
    }
    else
    {
        std::cerr << "udhcpc pidfile(" << udhcpc_pidfile << ") is invalid, try to start a new client" << std::endl;
        pid = fork();
        if (pid < 0)
        {
            std::cerr << "fork failed" << std::endl;
        }
        else if (pid > 0)
        {
            std::cerr << "fork success, child pid is " << pid << std::endl;
        }
        else
        {
            const char *argv[] = {
                udhcpc_path,
                "udhcpc",
                "-f",
                "-i",
                net,
                "-p",
                udhcpc_pidfile,
                (const char *)NULL};
            int ret = execv(udhcpc_path, (char **)argv);
            std::cerr << "execv fail code " << ret << std::endl;
        }
    }
}

/**
 * state machine process URC or Response and change it's state
 */
void ttyClient::start_machine()
{
    do
    {
        int wait_time = 30;
        std::unique_lock<std::mutex> _lk(usrlock);

        // std::cerr << "current state: " << static_cast<int>(state) << std::endl;
        switch (state)
        {
        case machine_state::STATE_START:
        {
            sendCommand(pATCmd->newQuerySIMinfo());
            break;
        }

        case machine_state::STATE_SIM_NEED_PIN:
        {
            sendCommand(pATCmd->newSetPincode());
            break;
        }

        case machine_state::STATE_SIM_READY:
        {
            sendCommand(pATCmd->newQueryRegisterinfo());
            break;
        }

        case machine_state::STATE_REGISTERED:
        {
            sendCommand(pATCmd->newATConfig());
            break;
        }

        case machine_state::STATE_CONFIG_DONE:
        {
            sendCommand(pATCmd->newQueryDataConnectinfo());
            break;
        }

        case machine_state::STATE_DISCONNECT:
        {
            sendCommand(pATCmd->newSetupDataCall());
            break;
        }

        case machine_state::STATE_CONNECT:
        {
            sendCommand(pATCmd->newQueryDataConnectinfo());
            break;
        }

        default:
        {
            break;
        }
        }

        // check tty ready state before wait
        if (!ttyreader->ready())
        {
            std::cerr << "machine stop for polling reader is not ready" << std::endl;
            break;
        }

        if (usrcond.wait_for(_lk, std::chrono::seconds(wait_time)) == std::cv_status::timeout)
        {
            std::cerr << "TIMEOUT(" << std::to_string(wait_time) << "s) CMD: " << atreqstr << std::endl;
            continue;
        }

        // check tty ready state after wait
        if (!ttyreader->ready())
        {
            std::cerr << "machine stop for polling reader is not ready" << std::endl;
            break;
        }

        std::vector<std::string> vecstr;
        for (auto iter = atrespstrlist.begin(); iter != atrespstrlist.end(); iter++)
        {
            vecstr.push_back(*iter);
            if (pATCmd->atCommandEnd(*iter))
            {
                machine_state new_state = pATCmd->parserResp(vecstr);
                if (state == machine_state::STATE_SIM_NEED_PIN &&
                    !pATCmd->isUnsocial() && pATCmd->isSuccess())
                    state = machine_state::STATE_START;

                else if (state == machine_state::STATE_REGISTERED &&
                         !pATCmd->isUnsocial() && pATCmd->isSuccess())
                    state = machine_state::STATE_CONFIG_DONE;

                else if (state == machine_state::STATE_CONFIG_DONE &&
                         !pATCmd->isUnsocial() && !pATCmd->isSuccess())
                    state = machine_state::STATE_DISCONNECT;

                else if (state != new_state &&
                         new_state != machine_state::STATE_INVALID)
                    state = new_state;

                atrespstrlist.erase(atrespstrlist.begin(), ++iter);
                break;
            }
        }

        if (state == machine_state::STATE_CONNECT)
        {
            std::cerr << "trigger DHCP operation" << std::endl;
            run_dhcp(netinfo.c_str());
            if (usrcond.wait_for(_lk, std::chrono::seconds(wait_time)) == std::cv_status::timeout)
            {
                std::cerr << "Time reached " << std::to_string(wait_time) << " seconds" << std::endl;
                continue;
            }
        }
    } while (1);

    std::cerr << "machine is stoped" << std::endl;
}
