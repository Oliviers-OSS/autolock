/*
 * tools.h
 *
 *  Created on: 13 nov. 2012
 *      Author: oc
 */

#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

#if (GCC_VERSION > 40000) /* GCC 4.0.0 */
#pragma once
#endif /* GCC 4.0.0 */

#ifndef TOOLS_H_
#define TOOLS_H_

#ifndef __cplusplus
#error C++ only include file
#endif /* __cplusplus */

#include <cstdarg>
#include <string>
#include <cstring>
#include <ctime>
#include <cstdlib>
#include "debug.h"
#include <cerrno>

/**
 * @brief vsprintf function for the std:string class.
 * @remark see vsprintf(3) for details.
 */
int vsprintf(std::string &s, const char *format,va_list params) __attribute__ ((format (printf, 2, 0))) __attribute__ ((nonnull(2)));

/**
 * @brief sprintf function for the std:string class
 * @remark see sprintf(3) for details.
 */
int sprintf(std::string &s, const char *format,...) __attribute__ ((format (printf, 2, 3))) __attribute__ ((nonnull(2)));

inline int strcasecmp(const std::string &s1,const char *s2)
{
  return strcasecmp(s1.c_str(),s2);
}

/**
 *  Subtract the ‘struct timespec’ values X and Y, storing the result in RESULT.
 * Return 1 if the difference is negative, otherwise 0.
 */

static inline int subtract(const struct timespec &x,const struct timespec &y,struct timespec &result)
{
	if (x.tv_nsec > y.tv_nsec)
	{
		result.tv_nsec = x.tv_nsec - y.tv_nsec;
		result.tv_sec = x.tv_sec - y.tv_sec;
	}
	else
	{
		result.tv_nsec = x.tv_nsec + 1000000 - y.tv_nsec;
		result.tv_sec = x.tv_sec - 1 - y.tv_sec;
	}
	return (result.tv_sec < 0);
}

static inline int getElapsedTime(const struct timespec &startTime,struct timespec &elapsedTime)
{
	int error = EXIT_SUCCESS;
	struct timespec currentTime;
	if (likely(clock_gettime(CLOCK_MONOTONIC,&currentTime) == 0))
	{
		if (unlikely(subtract(currentTime,startTime,elapsedTime) == 1))
		{
			error = EINVAL;
			WARNING_MSG("startTime < currentTime");
		}
	}
	else
	{
		error = errno;
		ERROR_MSG("clock_gettime CLOCK_MONOTONIC startTime error %d (%m)",error);
	}
	return error;
}
#endif /* TOOLS_H_ */
