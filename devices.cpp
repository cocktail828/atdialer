#include <iostream>
#include <sstream>
#include <fstream>

#include <dirent.h>
#include <cstdlib>
#include <errno.h>
#include <cstring>
#include <cstdio>

#include <unistd.h>

#include "devices.hpp"

static std::string dir_find_net(const char *dirname)
{
    struct dirent *ent = NULL;
    DIR *pdir = NULL;

    pdir = opendir(dirname);
    if (!pdir)
        return "";

    while ((ent = readdir(pdir)) != NULL)
    {
        if (ent->d_name[0] == '.')
            continue;
        else
        {
            closedir(pdir);
            return std::string(ent->d_name);
        }
    }
    closedir(pdir);
    return "";
}

static std::string find_device(const char *dirname)
{
    struct dirent *ent = NULL;
    DIR *pdir = NULL;

    pdir = opendir(dirname);
    if (!pdir)
        return "";

    while ((ent = readdir(pdir)) != NULL)
    {
        if (ent->d_name[0] == '.')
            continue;

        if (!strncmp(ent->d_name, "ttyUSB", 6))
        {
            closedir(pdir);
            return std::string(ent->d_name);
        }
        else if (!strncmp(ent->d_name, "net", 3))
        {
            std::string path(dirname);
            closedir(pdir);
            path += std::string("/") + ent->d_name;
            return dir_find_net(path.c_str());
        }
    }
    closedir(pdir);

    return "";
}

static std::string file_get_line(const std::string &file)
{
    std::string line;
    std::ifstream fin(file);

    if (!fin.is_open())
        return "";

    fin >> line;
    fin.close();
    return line;
}

static int file_get_xint(const std::string &file)
{
    int line;
    std::ifstream fin(file);

    if (!fin.is_open())
        return -1;

    fin >> std::hex >> line;
    fin.close();
    return line;
}

void Device::reset()
{
    m_usbdevs.clear();
}

int Device::scan_iface(int vid, int pid, std::string usbport, std::string rootdir)
{
    struct dirent *ent = NULL;
    DIR *pdir = NULL;
    usbdev udev;

    udev.vid = vid;
    udev.pid = pid;
    udev.usbport = usbport;
    udev.devpath = rootdir;
    pdir = opendir(rootdir.c_str());
    if (!pdir)
        return -1;

    while ((ent = readdir(pdir)) != NULL)
    {
        std::string file;
        interface iface;
        std::string path(rootdir);
        if (ent->d_name[0] == '.')
            continue;

        path += "/" + std::string(ent->d_name);

        file = std::string(path) + "/bInterfaceClass";
        iface.cls = file_get_xint(file);

        file = std::string(path) + "/bInterfaceSubClass";
        iface.subcls = file_get_xint(file);

        file = std::string(path) + "/bInterfaceProtocol";
        iface.proto = file_get_xint(file);

        file = std::string(path) + "/bInterfaceNumber";
        iface.ifno = file_get_xint(file);

        iface.ttyornet = find_device(path.c_str());

        if (iface.cls == -1 || iface.subcls == -1 || iface.proto == -1 || iface.ifno == -1)
            continue;

        udev.ifaces.push_back(iface);
    }
    closedir(pdir);

    if (udev.ifaces.size())
        m_usbdevs.push_back(udev);

    return 0;
}

int Device::scan(const std::string &usbport)
{
    struct dirent *ent = NULL;
    DIR *pdir = NULL;
    const char *rootdir = "/sys/bus/usb/devices";

    pdir = opendir(rootdir);
    if (!pdir)
        return -1;

    while ((ent = readdir(pdir)) != NULL)
    {
        std::string path(rootdir);
        std::string file;
        std::string _usbport;
        int _vid;
        int _pid;
        if (ent->d_name[0] == '.' || ent->d_name[0] == 'u')
            continue;

        path += "/" + std::string(ent->d_name);
        file = std::string(path) + "/idVendor";
        _vid = file_get_xint(file);
        file = std::string(path) + "/idProduct";
        _pid = file_get_xint(file);
        file = std::string(path) + "/devpath";
        _usbport = file_get_line(file);

        if (!usbport.empty() && usbport != _usbport)
            continue;

        if (_vid != -1 && _pid != -1)
            scan_iface(_vid, _pid, _usbport, path);
    }
    closedir(pdir);

    for (auto iter = m_usbdevs.begin(); iter != m_usbdevs.end(); iter++)
    {
        std::cerr << "Device ID " << std::hex << iter->vid << ":" << iter->pid
                  << ", Port " << iter->usbport
                  << ", Path " << iter->devpath << std::endl;
        for (auto iter1 = iter->ifaces.begin(); iter1 != iter->ifaces.end(); iter1++)
        {
            std::cerr << "  |__ If " << iter1->ifno
                      << ", Class=" << iter1->cls
                      << ", SubClass=" << iter1->subcls
                      << ", Proto=" << iter1->proto
                      << ", NODE=" << iter1->ttyornet << std::endl;
        }
    }

    return 0;
}

bool Device::exist(int vid, int pid, int cls, int scls, int proto)
{
    for (auto iter = m_usbdevs.begin(); iter != m_usbdevs.end(); iter++)
    {
        if (iter->vid != vid || iter->pid != pid)
            continue;

        for (auto iter1 = iter->ifaces.begin(); iter1 != iter->ifaces.end(); iter1++)
        {
            if (iter1->cls == cls && iter1->subcls == scls && iter1->proto == proto)
                return true;
        }
    }
    return false;
}

bool Device::exist(int vid, int pid, int ifno)
{
    for (auto iter = m_usbdevs.begin(); iter != m_usbdevs.end(); iter++)
    {
        if (iter->vid != vid || iter->pid != pid)
            continue;

        for (auto iter1 = iter->ifaces.begin(); iter1 != iter->ifaces.end(); iter1++)
        {
            if (iter1->ifno == ifno)
                return true;
        }
    }
    return false;
}

interface Device::get_interface(int vid, int pid, int cls, int scls, int proto)
{
    for (auto iter = m_usbdevs.begin(); iter != m_usbdevs.end(); iter++)
    {
        if (iter->vid != vid || iter->pid != pid)
            continue;

        for (auto iter1 = iter->ifaces.begin(); iter1 != iter->ifaces.end(); iter1++)
        {
            if (iter1->cls == cls && iter1->subcls == scls && iter1->proto == proto)
                return *iter1;
        }
    }
    return interface();
}

interface Device::get_interface(int vid, int pid, int ifno)
{
    for (auto iter = m_usbdevs.begin(); iter != m_usbdevs.end(); iter++)
    {
        if (iter->vid != vid || iter->pid != pid)
            continue;

        for (auto iter1 = iter->ifaces.begin(); iter1 != iter->ifaces.end(); iter1++)
        {
            if (iter1->ifno == ifno)
                return *iter1;
        }
    }
    return interface();
}