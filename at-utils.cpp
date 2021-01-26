/*
 * @Author: sinpo828
 * @Date: 2020-12-29 10:37:58
 * @LastEditors: sinpo828
 * @LastEditTime: 2021-01-26 18:43:24
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
    AT_UNISOC = AT_HISILICON,
    AT_ASR,
};

enum class filter
{
    FLT_IFNO, // filter by interface number
    FLT_INTF, // filter by interface info
};

struct interface_info
{
    filter flt;
    int ifno;
    int cls;
    int scls;
    int proto;
};

struct match_usb_info
{
    attype type;

    int vid;
    int pid;
    interface_info atinfo;
    interface_info netinfo;
};

#define USB_INTF_INFO(ft, cls, scls, proto)    \
    {                                          \
        filter::FLT_INTF, -1, cls, scls, proto \
    }

#define USB_INTF_INNO(ft, ifno)            \
    {                                      \
        filter::FLT_IFNO, ifno, -1, -1, -1 \
    }

#define USB_AND_INTF_INFO(t, v, p, INFO_AT, INFO_NET) \
    {                                                 \
        t, v, p, INFO_AT, INFO_NET                    \
    }

match_usb_info device_list[] = {
    USB_AND_INTF_INFO(attype::AT_HISILICON, 0x2c7c, 0x8101,
                      USB_INTF_INFO(filter::FLT_INTF, 0xff, 0x02, 0x12),
                      USB_INTF_INFO(filter::FLT_INTF, 0x02, 0x0d, 0x00)),
    // USB_AND_INTF_INFO(attype::AT_ASR, intftype::INTF_TYPE_AT, 0x2c7c, 0x6004, 0x0a, 0x00, 0x00),
    // USB_AND_INTF_INFO(attype::AT_HISILICON, 0x0403, 0x6001, 0xff, 0xff, 0xff),
};

static string g_netinterface("");
static string g_ttyusb("");
static attype find_device(const std::string &usbport)
{
    Device dev;
    auto _find_device = [&](match_usb_info *info, interface_info *intfinfo) {
        if (intfinfo->flt == filter::FLT_INTF &&
            dev.exist(info->vid, info->pid, intfinfo->cls, intfinfo->scls, intfinfo->proto))
        {
            interface iface = dev.get_interface(info->vid, info->pid, intfinfo->cls, intfinfo->scls, intfinfo->proto);
            return iface.ttyornet;
        }
        else if (intfinfo->flt == filter::FLT_IFNO &&
                 dev.exist(info->vid, info->pid, intfinfo->ifno))
        {
            interface iface = dev.get_interface(info->vid, info->pid, intfinfo->ifno);
            return iface.ttyornet;
        }
        return string("");
    };

    dev.scan(usbport);
    for (long unsigned int idx = 0; idx < sizeof(device_list) / sizeof(match_usb_info); idx++)
    {
        match_usb_info *info = &device_list[idx];
        interface_info *atinfo = &device_list[idx].atinfo;
        interface_info *netinfo = &device_list[idx].netinfo;
        string ttyusb("");

        ttyusb = _find_device(info, atinfo);
        g_netinterface = _find_device(info, netinfo);
        if (!ttyusb.empty() && !g_netinterface.empty())
        {
            g_ttyusb = "/dev/" + ttyusb;
            return info->type;
        }
    }
    return attype::AT_INVALID;
}

static int start(const std::string &tty, const std::string &net, ATCommand *pATCmd)
{
    auto reader = ttyReader::singleton(tty);
    ttyClient client(reader, net, pATCmd);

    std::thread polling_thread(&ttyReader::polling, reader);
    ON_SCOPE_EXIT
    {
        delete reader;
    };

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
    cerr << "    -P pincode        Pincode(default \"\")" << endl;
    cerr << "    -A AUTH           AUTH Type" << endl;
    cerr << "                          none (default)" << endl;
    cerr << "                          pap" << endl;
    cerr << "                          chap" << endl;
    cerr << "                          pap_chap" << endl;
    // cerr << "    -4 IPv4           IP Protocol" << endl;
    // cerr << "    -6 IPv4           IP Protocol" << endl;
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
    attype type = attype::AT_INVALID;

    std::string usbport("");
    std::string apn, usr, passwd, pincode;
    AUTH auth = AUTH::AUTH_NONE;
    IPPROTO iptype = IPPROTO::PROTO_IPV4;
    int cid = 1;
    ATCommand *pATCmd = nullptr;

    ON_SCOPE_EXIT
    {
        if (pATCmd)
            delete pATCmd;
    };
    cerr << "current version: " << VERSION_STRING() << endl;
    while ((ch = getopt(argc, argv, "a:u:p:P:A:c:d:l")) != -1)
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
        case 'P':
            cerr << "pincode: " << optarg << std::endl;
            pincode = optarg;
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
        type = find_device(usbport);
        if (type != attype::AT_INVALID)
            break;

        if (retry > max_retry)
        {
            cerr << "cannot find supportted AT device" << endl;
            return -1;
        }
        cerr << "try to find AT port, times = " << std::dec << retry++ << endl;
        this_thread::sleep_for(chrono::seconds(1));
    } while (1);

    cerr << "will use AT port: " << g_ttyusb << endl;
    if (type == attype::AT_HISILICON)
    {
        cerr << "AT mode: Hisilicon" << endl;
        pATCmd = new Hisilicon(apn, usr, passwd, auth, pincode, iptype, cid);
    }
    else if (type == attype::AT_ASR)
    {
        cerr << "AT mode: ASR(Marvell)" << endl;
        pATCmd = new ASR(apn, usr, passwd, auth, pincode, iptype, cid);
    }
    else
    {
        cerr << "error AT mode" << endl;
        return -1;
    }

    return start(g_ttyusb, g_netinterface, pATCmd);
}