#ifndef __STATE_MACHINE__
#define __STATE_MACHINE__

typedef enum class __MACHINE_STATE
{
    STATE_INVALID = -1,
    STATE_START,
    STATE_SIM_READY,
    STATE_CONNECT,
    STATE_DISCONNECT,
} machine_state;

#endif //__STATE_MACHINE__
