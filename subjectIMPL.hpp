#ifndef __SUBJECTIMPL__
#define __SUBJECTIMPL__

#include <string>
#include <mutex>

#include "subject_observer.hpp"

class ttyReader final : public ISubject
{
private:
    std::string ttydev;
    int ttyfd;
    uint8_t recvbuf[1024 * 30];

private:
    ttyReader() = delete;
    ttyReader &operator=(const ttyReader &) = delete;
    ttyReader(const ttyReader &) = delete;
    ttyReader(ttyReader &&) = delete;
    ttyReader(const char *d) : ttydev(d), ttyfd(-1) {}

public:
    ~ttyReader();

    static ttyReader *ttyInstance(const char *ttydev);

    void init();

    /**
     * create a new thread to monitor the tty device
     */
    void polling();

    bool sendAsync(const std::string &req);

    bool recvAsync();

    bool ready();

    /* no need to notify observers. we need update machine state here */
    virtual void notifyAll() override;
};

#endif //__SUBJECTIMPL__