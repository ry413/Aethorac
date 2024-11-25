#pragma once

#include "stdbool.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Initialize Ethernet connection
     */
    void ethernet_init();

    /**
     * Check if Ethernet connection is ready or not
     */
    bool ethernet_is_ready();

    /**
     * Wait for network connection to become ready
     */
    void ethernet_wait_for_network();

#ifdef __cplusplus
}
#endif
