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

ttyReader::~ttyReader()
{
    if (ttyfd > 0)
    {
        close(ttyfd);
        ttyfd = -1;
    }
    ttydev.clear();
}

ttyReader *ttyReader::ttyInstance(const char *ttydev)
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
        return;

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
        std::cerr << "setUart error" << std::endl;
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
    int epfd = epoll_create(10);

    epoll_event event;
    event.data.fd = ttyfd;
    event.events = EPOLLIN | EPOLLRDHUP;
    epoll_ctl(epfd, EPOLL_CTL_ADD, ttyfd, &event);

    auto pollid = std::this_thread::get_id();

    std::cerr << "polling thread start polling, thread id " << pollid << std::endl;
    epoll_event events[10];
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
                if (events[i].events & EPOLLRDHUP)
                {
                    break;
                }

                if (events[i].events & (EPOLLERR | EPOLLHUP))
                {
                    break;
                }

                if (events[i].events & EPOLLIN)
                {
                    if (recvAsync())
                        notifyAll();
                }
            }
        }
    } while (1);
    std::cerr << "polling thread quit polling" << std::endl;
}

bool ttyReader::sendAsync(const std::string &req)
{
    std::lock_guard<std::mutex> _lk(rwlock);

    if (ttyfd < 0 || req.size() == 0)
        return 0;

    return write(ttyfd, req.c_str(), req.size());
}

bool ttyReader::recvAsync()
{
    std::lock_guard<std::mutex> _lk(rwlock);

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
    if (observers.size())
    {
        auto o = observers.front();
        o->update(respstr);
    }
}
