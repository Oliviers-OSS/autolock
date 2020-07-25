/*
 * Parameters.cpp
 *
 *  Created on: 26 juil. 2020
 *      Author: oc
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define SYSLOG_NAMES

#include "debug.h"
#include "Parameters.h"
#include "ini.h"

#include <cstring>
#include <syslog.h>

int Parameters::set_syslogLevel(const char*level) {
	int syslogLevelValue = -1;
	int error = EXIT_SUCCESS;
	if (isdigit(level[0]))
	{
		/* 0x... (hexa value) ) 0.. (octal value or zero) 1 2 3 4 5 6 7 8 9 (decimal value) */
		char *endptr = NULL;
		syslogLevelValue = strtoul(level, &endptr, 0);
		if (endptr != level)
		{
			if (syslogLevelValue > LOG_DEBUG)
			{
				syslogLevelValue = -1;
				error = EINVAL;
				ERROR_MSG("bad syslog value %d", syslogLevelValue);
			}
			else
			{
				syslogLevel = LOG_UPTO(syslogLevelValue);
			}
			DEBUG_VAR(syslogLevelValue, "%d");
		}
		else
		{
			syslogLevelValue = -1;
			errno = EINVAL;
		}
	}
	else
	{
		/* !(isdigit(syslogLevel[0])) */
		CODE *cursor = prioritynames;
		while (cursor->c_name != NULL)
		{
			if (strcasecmp(level, cursor->c_name) != 0)
			{
				cursor++;
			}
			else
			{
				syslogLevelValue = cursor->c_val;
				syslogLevel = LOG_UPTO(syslogLevelValue);
				break;
			}
		} /* while(cursor->c_name != NULL) */

		if (NULL == cursor->c_name)
		{
#ifdef _DEBUG_
			if (isalnum(level[0]))
			{
				DEBUG_VAR(level, "%s");
			}
			else
			{
				DEBUG_VAR(level[0], "%d");
			}
#endif	/* _DEBUG_ */
			error = EINVAL;
		} /* (NULL == cursor->c_name) */
	} /* !(isdigit(syslogLevel[0])) */

	if (EXIT_SUCCESS == error)
	{
		setlogmask(LOG_UPTO(syslogLevelValue));
	}
	return error;
}

static int handler(void* user, const char* section, const char* name,const char* value)
{
	int error = EXIT_SUCCESS;
	Parameters* pconfig = (Parameters*)user;

#define SET(s, n,m) if ((strcmp(section, s) == 0) && (strcmp(name, n) == 0)) { error = pconfig->set_##m(value);} else

	SET("","log_level",syslogLevel)
	SET("","max_idle_duration_in_minutes",duration)
	SET("","events_source",device)
	SET("locker","program_run_account",user)
	SET("locker","program_with_parameters",program)
	SET("locker","max_number_of_restart_on_failure",nbRestart)
	if ((strcmp(section, "locker") == 0) && (strcmp(name, "run_on_startup") == 0)) {
		if (('F' != value[0]) && ('f' != value[0]) && ('0' != value[0]))
			error = pconfig->set_mode("LockOnStartup");
	} else {
		WARNING_MSG("Unknown parameter %s in section %s (value = %s)",name,section,value);
	}
#undef SET
	return (EXIT_SUCCESS == error);
}

int Parameters::loadConfigurationFile() {
	const char *configFile = configurationFile.c_str();
	int error = ini_parse(configFile, handler, this);
	if (unlikely(error != 0)) {
		if (likely(error > 0)) {
			ERROR_MSG("Error  at line %d in configuration file %s",error,configFile);
			error = EINVAL;
		} else {
			error = errno = -error;
			ERROR_MSG("Error %d (%m) reading configuration file %s",error,configFile);
		}
	}
	return error;
}

bool Parameters::isValid(std::string &errorMsg)
{
	bool valid(true);
	// Load configuration file if set
	if (!configurationFile.empty()) {
		loadConfigurationFile(); // error already printed, no fatal error
	}

	// check if mandatory parameters are set
	bool check = (duration != 0);
	valid &= check;
	if (unlikely(!check)) {
		errorMsg += " max IDLE duration is 0\n";
	}

	check = (!device.empty());
	valid &= check;
	if (unlikely(!check)) {
		errorMsg += " events source device is not set\n";
	}

	check = (programAndParameters != NULL);
	valid &= check;
	if (unlikely(!check)) {
		errorMsg += " missing locker program and its parameters\n";
	}
	return valid;
}
