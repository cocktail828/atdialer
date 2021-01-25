/*
 * @Author: sinpo828
 * @Date: 2020-12-30 15:17:52
 * @LastEditors: sinpo828
 * @LastEditTime: 2021-01-22 13:47:04
 * @Description: file content
 */
#ifndef __OBSERVERIMPL__
#define __OBSERVERIMPL__

#include <string>
#include <mutex>
#include <vector>
#include <condition_variable>

#include "subject_observer.hpp"
#include "subjectIMPL.hpp"
#include "state_machine.hpp"
#include "atenum.hpp"
#include "at_command.hpp"

class ttyClient final : public IObserver
{
private:
    static std::mutex usrlock;
    static std::condition_variable usrcond;
    static machine_state state;
    ttyReader *ttyreader;
    std::string atreqstr;
    std::vector<std::string> atrespstrlist;
    ATCommand *pATCmd;

public:
    ttyClient() = delete;
    ttyClient(ttyReader *reader, ATCommand *cmd) : ttyreader(reader), atreqstr(""), pATCmd(cmd) { ttyreader->addObserver(this); }

    ~ttyClient() { std::cerr << __PRETTY_FUNCTION__ << std::endl; }

    void sendCommand(std::string &&);

    void update(const std::string &);

    void update();

    void start_machine();
};

#endif //__OBSERVERIMPL__
