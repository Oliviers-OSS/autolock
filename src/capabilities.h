/*
 * capabilities.h
 *
 *  Created on: 26 août 2018
 *      Author: oc
 */

#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

#if (GCC_VERSION > 40000) /* GCC 4.0.0 */
#pragma once
#endif /* GCC 4.0.0 */

#ifndef _CAPABILITIES_H_
#define _CAPABILITIES_H_

#include "Exception.h"
#include "debug.h"

#include <cstdlib>
#include <cerrno>
#include <sys/capability.h>

static inline void printCapabilities(cap_t capabilities) {
	ssize_t length_p = 0;
	char *s = cap_to_text(capabilities,&length_p);
	if (s) {
		DEBUG_MSG("Capabilities = %s\n",s);
		cap_free(s);
		s = NULL;
	}
}

static inline void printCapabilities() {
	cap_t capabilities = cap_get_proc();
	if (capabilities) {
		ssize_t length_p = 0;
		char *s = cap_to_text(capabilities,&length_p);
		if (s) {
			DEBUG_MSG("Capabilities = %s\n",s);
			cap_free(s);
			s = NULL;
		}
	}
}

static inline int dropAllCapabilities() {
	int error = EXIT_SUCCESS;
	cap_t emptyCap = cap_init();

	if (likely(emptyCap != NULL)) {
		if (unlikely(cap_set_proc(emptyCap) != 0)) {
			error = errno;
			const uid_t euid = geteuid();
			ERROR_MSG("cap_set_proc error %d (%m), euid = %u",error,euid);
		}
		cap_free(emptyCap);
		emptyCap = NULL;
	} else {
		error = errno;
		ERROR_MSG("cap_init error %d (%m)",error);
	}
	return error;
}

static inline int manageCapabilities(cap_t capabilities,cap_value_t *capabilitiesValues,int nbValues, cap_flag_value_t operation = CAP_SET,cap_flag_t capabilitiesSet = CAP_EFFECTIVE) {
	int error = EXIT_SUCCESS;

	if (likely(cap_set_flag(capabilities,capabilitiesSet,nbValues,capabilitiesValues,operation) == 0)) {
		if (unlikely(cap_set_proc(capabilities) != 0)) {
			const uid_t euid = geteuid();
			error = errno;
			ERROR_MSG("cap_set_proc enable error %d (%m), euid = %u",error,euid);
		}
	} else {
		const uid_t euid = geteuid();
		error = errno;
		ERROR_MSG("cap_set_flag set error %d (%m), euid = %u",error,euid);
	}

	return error;
}

static inline int manageCapabilities(cap_value_t *capabilitiesValues,int nbValues, cap_flag_value_t operation = CAP_SET,cap_flag_t capabilitiesSet = CAP_EFFECTIVE) {
	int error = EXIT_SUCCESS;
	const uid_t euid = geteuid();

	if (likely(euid != 0)) {
		if (likely((CAP_IS_SUPPORTED(CAP_SETFCAP)) && (CAP_IS_SUPPORTED(CAP_SETPCAP)))) {
			cap_t capabilities = cap_get_proc();
			if (likely(capabilities != NULL)) {
				error = manageCapabilities(capabilities,capabilitiesValues,nbValues,operation,capabilitiesSet);
				cap_free(capabilities);
				capabilities = NULL;
			} else {
				error = errno;
				ERROR_MSG("cap_get_proc error %d (%m), euid = %u",error,euid);
			}
		} else {
			error = EPERM;
			CRIT_MSG("System doesn't have capabilities support enabled");
		}
	}
	return error;
}

#define ENABLE_CAPABILITIES(x)	manageCapabilities(x,ARRAY_SIZE(x))
#define DISABLE_CAPABILITIES(x)	manageCapabilities(x,ARRAY_SIZE(x),CAP_CLEAR)

#define THROW_EXCEPTION(code,fmt,...)       throw EffectiveCapabilitiesManager::Exception(__FILE__,__LINE__,__PRETTY_FUNCTION__,code,fmt, ##__VA_ARGS__)

class EffectiveCapabilitiesManager {
	cap_t capabilities;
	uid_t euid;
	cap_value_t *values;
	int nb;

public:

#include "InnerException.h"

	EffectiveCapabilitiesManager(cap_value_t *capabilitiesValues,int nbValues):capabilities(NULL),euid(0),values(capabilitiesValues),nb(nbValues) {
		euid = geteuid();
		if (likely(euid != 0)) {
			if (likely((CAP_IS_SUPPORTED(CAP_SETFCAP)) && (CAP_IS_SUPPORTED(CAP_SETPCAP)))) {
				capabilities = cap_get_proc();
				if (capabilities) {
					const int error = manageCapabilities(capabilities,values,nb,CAP_SET,CAP_EFFECTIVE);
					if (error != EXIT_SUCCESS) {
						cap_free(capabilities);
						capabilities = NULL;
						THROW_EXCEPTION(error,"manageCapabilities error %d, euid = %u",error,euid);
					}
				} else {
					const int error = errno;
					THROW_EXCEPTION(error,"cap_get_proc error %d (%m), euid = %u",error,euid);
				}
			} else {
				THROW_EXCEPTION(EPERM,"System doesn't have capabilities support enabled");
			}
		} // nothing to do if the effective user id is root all is already enabled
	}

	~EffectiveCapabilitiesManager() {
		if (capabilities) {
			manageCapabilities(capabilities,values,nb,CAP_CLEAR,CAP_EFFECTIVE);
			cap_free(capabilities);
			capabilities = NULL;
		}
	}
};

#define MANAGE_EFFECTIVE_CAP(x)	EffectiveCapabilitiesManager effectiveCapMgr(x,ARRAY_SIZE(x))

#undef THROW_EXCEPTION

#endif /* _CAPABILITIES_H_ */
