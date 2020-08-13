/*
 * Parameters.h
 *
 *  Created on: 26 juil. 2020
 *      Author: oc
 */

#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

#if (GCC_VERSION > 40000) /* GCC 4.0.0 */
#pragma once
#endif /* GCC 4.0.0 */

#ifndef PARAMETERS_H_
#define PARAMETERS_H_

#include "debug.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <bitset>

#ifndef TO_STRING
#define STRING(x) #x
#define TO_STRING(x) STRING(x)
#endif /* STRING */

#define PROGNAME PACKAGE
#define CONFIGURATION_DIR TO_STRING(_CONFIGDIR)
#define VT_NOT_SET ((unsigned int)-1)

#define MODE(m)  X(m)
#define MODE_TABLE \
		MODE(LockOnStartup) \
		MODE(RunInTerminal) \
		MODE(SwitchTerminal) \
		MODE(Console)

#define X(m)    ev_##m,
typedef enum ModeBitValue_ {
	MODE_TABLE
} ModeBitValue;
#undef X

#define X(m)    e_##m = 1<<ev_##m,
typedef enum ModeValue_ {
	MODE_TABLE
} ModeValue;
#undef X

static inline const char* to_string(const ModeValue m) {
#define X(m)	case e_##m: return TO_STRING(m);
	switch(m) {
	MODE_TABLE
	}
#undef X
	return "";
}

#define SET_STRING_MEMBER(m) int set_##m(const char *s) { \
		if (!is_set(e_##m)) { \
			m = s; \
			set(e_##m); \
		} \
		return EXIT_SUCCESS; \
}

#define GET_STRING_MEMBER(m) const char * get_##m() const { \
		return m.c_str(); \
}

#define SET_UINT_MEMBER(m) int set_##m(const char *s) { \
		int error = EXIT_SUCCESS; \
		errno = EXIT_SUCCESS; \
		if (!is_set(e_##m)) { \
			m = strtoul(s,NULL,0); \
			error = errno; \
			if (EXIT_SUCCESS == error) { \
				set(e_##m); \
			} \
		} \
		return error; \
}
#define GET_UINT_MEMBER(m) unsigned int get_##m() const { \
		return m; \
}

struct Parameters
{
#define MEMBER(t,m)	X(t,m)
#define MEMBERS_TABLE \
		MEMBER(unsigned int,modes)\
		MEMBER(std::string,device)\
		MEMBER(char *,programAndParameters)\
		MEMBER(std::vector<const char*>,argv)\
		MEMBER(std::string,user)\
		MEMBER(unsigned int,duration)\
		MEMBER(unsigned int,nbRestart)\
		MEMBER(std::string,configurationFile) \
		MEMBER(int,syslogLevel)

	enum Members:unsigned int {
#define X(t,m) e_##m,
		MEMBERS_TABLE
		NbMembers
#undef X
	};
	std::bitset<NbMembers> memberset;
#define X(t,m) t m;
	MEMBERS_TABLE
#undef X

	bool is_set(const Members m) {
		return memberset.test(m);
	}

	void set(const Members m,bool v = true) {
		memberset.set(m,v);
	}

	const char* to_string(const Members m) {
#define X(t,m)	case e_##m: return TO_STRING(m);
		switch(m) {
		MEMBERS_TABLE
		case NbMembers: return "NbMembers";
		}
#undef X
		return "NbMembers";
	}


	// duration in sec


	Parameters()
	:modes(0x0),programAndParameters(NULL)
	,duration(10*60),nbRestart(0)
	,configurationFile(CONFIGURATION_DIR "/autolockd")
	,syslogLevel(LOG_UPTO(LOG_NOTICE))
	{
	}

	~Parameters(){
		free(programAndParameters);
	}

	SET_UINT_MEMBER(modes)
	SET_UINT_MEMBER(nbRestart)
	SET_STRING_MEMBER(device)
	SET_STRING_MEMBER(user)
	SET_STRING_MEMBER(configurationFile)

	GET_UINT_MEMBER(modes)
	GET_UINT_MEMBER(duration)
	GET_UINT_MEMBER(nbRestart)
	GET_STRING_MEMBER(device)
	GET_STRING_MEMBER(user)
	GET_STRING_MEMBER(configurationFile)

	int set_duration(const char *value) {
		int error = EXIT_SUCCESS;
		errno = EXIT_SUCCESS;
		if (!is_set(e_duration)) {
			char *endptr = NULL;
			const unsigned int v = strtoul(value,&endptr,0);
			error = errno;
			if (unlikely((endptr) && (*endptr != '\0'))) {
				error = EINVAL;
				fprintf(stderr,"Invalid duration parameter %s (at %s)",value,endptr);
			}
			if (EXIT_SUCCESS == error) {
				set(e_duration);
				duration = v * 60; // min -> sec
			}
		}
		return error;
	}

	int set_program(const char*v) {
		int error = EXIT_SUCCESS;
		programAndParameters = strdup(v);
		if (likely(programAndParameters)) {
			const char *token = strtok(programAndParameters," ");
			argv.push_back(token);
			while ( (token = strtok(NULL," ")) != NULL) {
				argv.push_back(token);
			}
			argv.push_back(NULL);
		} else {
			error = ENOMEM;
			ERROR_MSG("Failed to allocate %zu bytes for the program to run and its parameters",strlen(v));
		}
		return error;
	}

	int set_mode(const char*v) {
		int error = EXIT_SUCCESS;
		//if (!is_set(e_modes)) { //TODO: better mode flag management between cmd line and config file parameter ? current is add both of them
#define X(m)	if (strcasecmp(TO_STRING(m),v)==0) { modes |= e_##m; DEBUG_MSG("Mode %s enabled",v); } else
			MODE_TABLE
#undef X
			{
				ERROR_MSG("Unknown mode %s",v);
				error = EINVAL;
			}
		//}
		return error;
	}

	int set_syslogLevel(const char*level);
	int loadConfigurationFile();
	bool isValid(std::string &errorMsg);

	bool is_mode_set(const ModeValue mode) const {
		return ((modes & mode) == mode);
	}

	int set_mode(const ModeValue mode) {
		modes |= mode;
		//set(e_modes);
		return EXIT_SUCCESS;
	}

	int unset_mode(ModeValue mode) {
		modes &= ~mode;
		return EXIT_SUCCESS;
	}
};

#undef MEMBER
#undef SET_UINT_MEMBER
#undef SET_STRING_MEMBER
#undef GET_UINT_MEMBER
#undef GET_STRING_MEMBER

#endif /* PARAMETERS_H_ */
