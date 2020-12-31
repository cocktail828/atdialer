#include <iostream>
#include <thread>

#include "subjectIMPL.hpp"
#include "observerIMPL.hpp"
#include "scopeguard.hpp"

using namespace std;

int main(int argc, char **argv)
{
    auto reader = ttyReader::ttyInstance("/dev/ttyUSB2");
    std::thread polling_thread(&ttyReader::polling, reader);

    ON_SCOPE_EXIT { delete reader; };

    /**
     * wait pooling thread ready
     */
    int times = 0;
    do
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (reader->ready())
            break;
        if (times++ > 10)
            return -1;
    } while (1);

    ttyClient client(reader);
    client.start_machine();
    polling_thread.join();

    return 0;
}