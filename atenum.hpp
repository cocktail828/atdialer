#ifndef __ATENUM__
#define __ATENUM__

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
