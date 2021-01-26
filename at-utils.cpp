/*
 * @Author: sinpo828
 * @Date: 2020-12-29 10:37:58
 * @LastEditors: sinpo828
 * @LastEditTime: 2021-01-26 12:09:25
 * @Description: file content
 */
#include <iostream>
#include <thread>

#include <getopt.h>

#include "subjectIMPL.hpp"
#include "observerIMPL.hpp"
#include "scopeguard.hpp"
#include "atenum.hpp"
#include "devices.hpp"
#include "hisilicon.hpp"
#include "asr.hpp"

using namespace std;

#define MAJOR 0
#define MINOR 0
#define REVISION 1
#define STRINGIFY_HELPER(v) #v
#define STRINGIFY(v) STRINGIFY_HELPER(v)
#define VERSION_STRING() \
    STRINGIFY(MAJOR)     \
    "." STRINGIFY(MINOR) "." STRINGIFY(REVISION)

enum class attype
{
    AT_INVALID,
    AT_HISILICON,
    AT_ASR,
};

enum class filter
{
    FLT_IFNO, // filter by interface number
    FLT_INTF, // filter by interface info
};

struct match_usb_info
{
    filter flt;
    attype type;

    int vid;
    int pid;
    int ifno;
    int cls;
    int scls;
    int proto;
};

#define USB_AND_INTF_INFO(t, v, p, cls, scls, pro)    \
    {                                                 \
        filter::FLT_INTF, t, v, p, -1, cls, scls, pro \
    }

#define USB_AND_INTF_NUM(t, v, p, ifn)             \
    {                                              \
        filter::FLT_IFNO, t, v, p, ifn, -1, -1, -1 \
    }

match_usb_info device_list[] = {
    USB_AND_INTF_INFO(attype::AT_HISILICON, 0x2c7c, 0x8101, 0xff, 0x02, 0x12),
    // USB_AND_INTF_INFO(0x2c7c, 0x8101, 0xff, 0x03, 0x12),
    // USB_AND_INTF_INFO(0x2c7c, 0x8101, 0xff, 0x06, 0x12),
};

static std::pair<attype, string> find_at_device(const std::string &usbport)
{
    Device dev;

    dev.scan(usbport);
    for (long unsigned int idx = 0; idx < sizeof(device_list) / sizeof(match_usb_info); idx++)
    {
        match_usb_info *info = &device_list[idx];
        if (info->flt == filter::FLT_INTF &&
            dev.exist(info->vid, info->pid, info->cls, info->scls, info->proto))
        {
            interface iface = dev.get_interface(info->vid, info->pid, info->cls, info->scls, info->proto);
            return std::pair<attype, string>(info->type, iface.ttyusb);
        }
        else if (info->flt == filter::FLT_IFNO &&
                 dev.exist(info->vid, info->pid, info->ifno))
        {
            interface iface = dev.get_interface(info->vid, info->pid, info->ifno);
            return std::pair<attype, string>(info->type, iface.ttyusb);
        }
    }
    return std::pair<attype, string>(attype::AT_INVALID, "");
}

static int start(const std::string &tty, ATCommand *pATCmd)
{
    auto reader = ttyReader::singleton(tty);
    ttyClient client(reader, pATCmd);

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
        {
            cerr << "detect reader error or max times(" + std::to_string(times) + ") reached" << endl;
            goto exit;
        }

        cerr << "polling is not ready" << endl;
    } while (1);

    client.start_machine();

exit:
    polling_thread.join();

    return 0;
}

void usage()
{
    cerr << "usage: ./atdial [options]" << endl;
    cerr << "    -a apn            APN(default \"\")" << endl;
    cerr << "    -u user           Username(default \"\")" << endl;
    cerr << "    -p password       Password(default \"\")" << endl;
    cerr << "    -A AUTH           AUTH Type" << endl;
    cerr << "                          none (default)" << endl;
    cerr << "                          pap" << endl;
    cerr << "                          chap" << endl;
    cerr << "                          pap_chap" << endl;
    cerr << "    -P Protocol       IP Protocol" << endl;
    cerr << "                          4 for IPv4(default)" << endl;
    cerr << "                          6 for IPv6" << endl;
    cerr << "                          46 for IPv4 & IPv6" << endl;
    cerr << "    -c contextid       Context ID also called PDN, always start from 1(default)" << endl;
    cerr << "" << endl;
    cerr << "    -d                 Use this physical port" << endl;
    cerr << "    -l                 Show devices and quit" << endl;
    cerr << "    -h                 Show this message" << endl;
}

int main(int argc, char **argv)
{
    bool show_and_quit = false;
    int max_retry = 10;
    int retry = 0;
    int ch;

    std::pair<attype, string> p;
    std::string usbport("");
    std::string ttyport("");
    std::string apn, usr, passwd;
    AUTH auth = AUTH::AUTH_NONE;
    IPPROTO iptype = IPPROTO::PROTO_IPV4;
    int cid = 1;
    ATCommand *pATCmd = nullptr;

    cerr << "current version: " << VERSION_STRING() << endl;
    while ((ch = getopt(argc, argv, "a:u:p:A:P:c:d:l")) != -1)
    {
        switch (ch)
        {
        case 'a':
            cerr << "apn: " << optarg << std::endl;
            apn = optarg;
            break;
        case 'u':
            cerr << "user: " << optarg << std::endl;
            usr = optarg;
            break;
        case 'p':
            cerr << "password: " << optarg << std::endl;
            passwd = optarg;
            break;
        case 'A':
            if (string(optarg).find("none") != string::npos)
                auth = AUTH::AUTH_NONE;
            else if (string(optarg).find("pap") != string::npos)
                auth = AUTH::AUTH_PAP;
            else if (string(optarg).find("chap") != string::npos)
                auth = AUTH::AUTH_CHAP;
            else if (string(optarg).find("pap_chap") != string::npos)
                auth = AUTH::AUTH_PAP_CHAP;
            else
            {
                cerr << "error AUTH: " << optarg << std::endl;
                return -1;
            }
            cerr << "AUTH: " << optarg << std::endl;
            break;
        case 'P':
            if (atoi(optarg) == 4)
                iptype = IPPROTO::PROTO_IPV4;
            else if (atoi(optarg) == 6)
                iptype = IPPROTO::PROTO_IPV4;
            else if (atoi(optarg) == 46)
                iptype = IPPROTO::PROTO_IPV4;
            else
            {
                cerr << "error IPTYPE: " << optarg << std::endl;
                return -1;
            }
            cerr << "IPPROTO: " << optarg << std::endl;
            break;
        case 'c':
            cerr << "contextid: " << optarg << std::endl;
            cid = atoi(optarg);
            break;
        case 'd':
            usbport = optarg;
            cerr << "usbport: " << optarg << std::endl;
            break;
        case 'l':
            show_and_quit = true;
            break;
        case 'h':
            usage();
            break;
        default:
            usage();
            return -1;
        }
    }

    if (show_and_quit)
    {
        Device dev;
        dev.scan(usbport);
        return 0;
    }

    do
    {
        p = find_at_device(usbport);
        if (p.first != attype::AT_INVALID && !p.second.empty())
        {
            ttyport += "/dev/" + p.second;
            break;
        }

        if (retry > max_retry)
        {
            cerr << "cannot find supportted AT device" << endl;
            return -1;
        }

        cerr << "try to find AT port, times = " << std::dec << retry++ << endl;
        this_thread::sleep_for(chrono::seconds(1));
    } while (1);

    cerr << "will use AT port: " << ttyport << endl;
    if (p.first == attype::AT_HISILICON)
    {
        cerr << "at mode: Hisilicon" << ttyport << endl;
        pATCmd = new Hisilicon(apn, usr, passwd, auth, iptype, cid);
    }
    else if (p.first == attype::AT_ASR)
    {
        cerr << "at mode: ASR(Marvell)" << ttyport << endl;
        pATCmd = new ASR(apn, usr, passwd, auth, iptype, cid);
    }
    ON_SCOPE_EXIT
    {
        if (pATCmd)
            delete pATCmd;
    };
    return start(ttyport, pATCmd);
}