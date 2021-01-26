/*
 * @Author: sinpo828
 * @Date: 2021-01-26 10:21:54
 * @LastEditors: sinpo828
 * @LastEditTime: 2021-01-26 17:07:11
 * @Description: file content
 */
#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <string>
#include <vector>

struct interface
{
    int cls;
    int subcls;
    int proto;
    int ifno;
    std::string ttyornet;
    interface() : cls(0), subcls(0), proto(0), ifno(0), ttyornet("") {}
};

struct usbdev
{
    int vid;
    int pid;
    std::string usbport;
    std::string devpath;
    std::vector<interface> ifaces;
    usbdev() : usbport(""), devpath("") {}
};

class Device final
{
private:
    std::vector<usbdev> m_usbdevs;

private:
    void reset();
    int scan_iface(int vid, int pid, std::string usbport, std::string rootdir);

public:
    Device() { reset(); }
    ~Device() {}

    int scan(const std::string &usbport);
    bool exist(int vid, int pid, int cls, int scls, int proto);
    bool exist(int vid, int pid, int ifno);
    interface get_interface(int vid, int pid, int cls, int scls, int proto);
    interface get_interface(int vid, int pid, int ifno);
};

#endif //__DEVICE_H__