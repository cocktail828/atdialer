#include <iostream>
#include <thread>

#include "subjectIMPL.hpp"
#include "observerIMPL.hpp"
#include "scopeguard.hpp"
#include "atenum.hpp"

using namespace std;

int main(int argc, char **argv)
{
    auto reader = ttyReader::singleton("/dev/ttyUSB0");
    ttyClient client(reader);

    std::thread polling_thread(&ttyReader::polling, reader);
    ON_SCOPE_EXIT { delete reader; };

    /**
     * wait pooling thread ready
     */
    int times = 1;
    do
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (reader->ready())
        {
            cerr << "polling is ready" << endl;
            break;
        }

        if (times++ > 100)
            goto exit;

        cerr << "polling is not ready" << endl;
    } while (1);

    client.start_machine("", "", "", AUTH::AUTH_NONE, IPPROTO::PROTO_IPV4, 1);

exit:
    cerr << "detect reader error or max times(" + std::to_string(times) + ") reached" << endl;
    polling_thread.join();

    return 0;
}