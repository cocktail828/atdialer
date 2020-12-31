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

class ttyClient final : public IObserver
{
private:
    static std::mutex usrlock;
    static std::condition_variable usrcond;
    static machine_state state;
    ttyReader *ttyreader;
    std::string atreqstr;
    std::vector<std::string> atrespstrlist;

public:
    ttyClient() = delete;
    ttyClient(ttyReader *reader) : ttyreader(reader), atreqstr("") { ttyreader->addObserver(this); }

    ~ttyClient() { std::cerr << __PRETTY_FUNCTION__ << std::endl; }

    void sendCommand(std::string &&);

    void update(const std::string &);

    void update();

    void start_machine(const char *apn, const char *usr, const char *passwd, AUTH auth, IPPROTO iptype, int cid);
};

#endif //__OBSERVERIMPL__
