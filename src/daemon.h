#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

#if (GCC_VERSION > 40000) /* GCC 4.0.0 */
#pragma once
#endif /* GCC 4.0.0 */

#ifndef _DAEMON_H_
#define _DAEMON_H_

//#include <signal.h>
#include <sys/types.h>

typedef void (*signalhandler_t)(int);

int deamonize(signalhandler_t sigHupHandler, signalhandler_t sigTermHandler,pid_t *daemonProcessId);

#endif /* _DAEMON_H_ */
