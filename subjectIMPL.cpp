#include <iostream>
#include <mutex>
#include <thread>

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/epoll.h>

#include "subjectIMPL.hpp"
#include "subject_observer.hpp"
#include "scopeguard.hpp"

ttyReader::~ttyReader()
{
    if (ttyfd > 0)
    {
        close(ttyfd);
        ttyfd = -1;
    }
    ttydev.clear();
}

ttyReader *ttyReader::singleton(const char *ttydev)
{
    static ttyReader *instance_ = new ttyReader(ttydev);
    instance_->init();
    return instance_;
}

void ttyReader::init()
{
    struct termios tio;
    struct termios settings;
    int retval;

    ttyfd = ::open(ttydev.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (ttyfd < 0)
    {
        std::cerr << "fail to open " << ttydev << std::endl;
        return;
    }

    memset(&tio, 0, sizeof(tio));
    tio.c_iflag = 0;
    tio.c_oflag = 0;
    tio.c_cflag = CS8 | CREAD | CLOCAL; // 8n1, see termios.h for more information
    tio.c_lflag = 0;
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 5;
    cfsetospeed(&tio, B115200); // 115200 baud
    cfsetispeed(&tio, B115200); // 115200 baud
    tcsetattr(ttyfd, TCSANOW, &tio);
    retval = tcgetattr(ttyfd, &settings);
    if (-1 == retval)
    {
        close(ttyfd);
        ttyfd = -1;
        std::cerr << "init tty error" << std::endl;
        return;
    }
    cfmakeraw(&settings);
    settings.c_cflag |= CREAD | CLOCAL;
    tcflush(ttyfd, TCIOFLUSH);
    tcsetattr(ttyfd, TCSANOW, &settings);
}

/**
 * create a new thread to monitor the tty device
 */
void ttyReader::polling()
{
    epoll_event events[10];
    int epfd = epoll_create(10);
    auto pollid = std::this_thread::get_id();

    epoll_event event;
    event.data.fd = ttyfd;
    event.events = EPOLLIN | EPOLLRDHUP;

    ON_SCOPE_EXIT
    {
        std::cerr << "polling thread quit polling" << std::endl;
        if (ttyfd)
            close(ttyfd);
        ttyfd = -1;
        for (auto iter = observers.begin(); iter != observers.end(); iter++)
            (*iter)->update();
        observers.clear();
    };

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, ttyfd, &event))
        return;

    std::cerr << "polling thread start polling, thread id " << pollid << std::endl;
    do
    {
        int num = epoll_wait(epfd, events, 10, 3000);
        if (num < 0)
            break;
        else if (num == 0)
            continue;
        else
        {
            for (int i = 0; i < num; i++)
            {
                // serial closed?
                if ((events[i].events & EPOLLRDHUP) ||
                    (events[i].events & (EPOLLERR | EPOLLHUP)))
                    return;

                if (events[i].events & EPOLLIN)
                {
                    if (recvAsync())
                        notifyAll();
                }
            }
        }

    } while (1);
}

bool ttyReader::sendAsync(const std::string &req)
{
    if (ttyfd < 0 || req.size() == 0)
        return false;

    ssize_t ret = write(ttyfd, req.c_str(), req.size());
    if (ret < 0)
        return false;
    return (size_t)ret == req.size();
}

bool ttyReader::recvAsync()
{
    if (ttyfd < 0)
        return false;

    memset(recvbuf, 0, sizeof(recvbuf));
    return read(ttyfd, recvbuf, sizeof(recvbuf)) >= 0;
}

bool ttyReader::ready() { return (ttyfd > 0); }

/* no need to notify observers. we need update machine state here */
void ttyReader::notifyAll()
{
    const std::string respstr(reinterpret_cast<const char *>(recvbuf));
    for (auto iter = observers.begin(); iter != observers.end(); iter++)
        (*iter)->update(respstr);
}
