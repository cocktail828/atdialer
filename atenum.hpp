/*
 * @Author: sinpo828
 * @Date: 2020-12-31 14:58:36
 * @LastEditors: sinpo828
 * @LastEditTime: 2021-01-22 13:53:55
 * @Description: file content
 */
#ifndef __ATENUM__
#define __ATENUM__

enum class OPERATE
{
    OPT_CONNECT,
    OPT_DISCONNECT,
};

enum class IPPROTO
{
    PROTO_IPV4 = 1,
    PROTO_IPV6,
    PROTO_IPV4V6,
};

enum class AUTH
{
    AUTH_NONE,
    AUTH_PAP,
    AUTH_CHAP,
    AUTH_PAP_CHAP,
};

#endif //__ATENUM__
