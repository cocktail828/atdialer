#include <thread>
#include <iostream>

#include "observerIMPL.hpp"
#include "at_command.hpp"

machine_state ttyClient::state = machine_state::STATE_START;
void ttyClient::sendCommand(std::string &&cmd)
{
    ttyreader->sendAsync(cmd);
    ttyreader->addObserver(this);
}

/**
 * process message from tty, and change machine state if need
 */
void ttyClient::update(const std::string &respstr)
{
    atrespstr += respstr;
    if (ATCommand::atCommandEnd(respstr))
    {
        machine_state new_state = ATCommand::parserResp(atrespstr);
        if (state != new_state && new_state != machine_state::STATE_INVALID)
            state = new_state;

        atrespstr.clear();
    }
}

void ttyClient::start_machine()
{
    do
    {
        std::cerr << "current state: " << static_cast<int>(state) << std::endl;
        switch (state)
        {
        case machine_state::STATE_START:
        {
            sendCommand(ATCommand::newQuerySIMinfo());
        }

        case machine_state::STATE_SIM_READY:
        {
            // state change to connect
            sendCommand(ATCommand::newQueryRegisterinfo());
        }

        case machine_state::STATE_DISCONNECT:
        {
            sendCommand(ATCommand::newSetupDataCall());
        }

        case machine_state::STATE_CONNECT:
        {
            sendCommand(ATCommand::newQueryDataConnectinfo());
        }

        default:
        {
        }
        }

        std::this_thread::sleep_for(std::chrono::microseconds(1000000));
    } while (1);
}
