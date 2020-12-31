#ifndef __OBSERVERIMPL__
#define __OBSERVERIMPL__

#include <string>

#include "subject_observer.hpp"
#include "subjectIMPL.hpp"
#include "state_machine.hpp"

class ttyClient final : public IObserver
{
private:
    static machine_state state;
    ttyReader *ttyreader;
    std::string atrespstr;
    bool is_success;

public:
    ttyClient() = delete;
    ttyClient(ttyReader *reader)
        : ttyreader(reader), atrespstr(""), is_success(false) {}

    ~ttyClient() {}

    void sendCommand(std::string &&cmd);

    void update(const std::string &);

    void start_machine();
};

#endif //__OBSERVERIMPL__
