/*
 * privileges.h
 *
 *  Created on: Apr 4, 2016
 *      Author: T0025640
 */

#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

#if (GCC_VERSION > 40000) /* GCC 4.0.0 */
#pragma once
#endif /* GCC 4.0.0 */

#ifndef _PRIVILEGES_H_
#define _PRIVILEGES_H_

#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/param.h>
#include <sys/types.h>
#include <linux/limits.h>

#ifndef _DEBUG_H_
#include <syslog.h>
#define DEBUG_EOL	"\n"
#define DEBUG_LOG_HEADER_POS    " [ %s ("  __FILE__ ":%d)]:"
#define WARNING_MSG(fmt,...)	syslog(LOG_WARNING, DEBUG_LOG_HEADER_POS fmt DEBUG_EOL,__FUNCTION__,__LINE__, ##__VA_ARGS__)
#define ERROR_MSG(fmt,...)		syslog(LOG_ERR,DEBUG_LOG_HEADER_POS fmt DEBUG_EOL,__FUNCTION__,__LINE__, ##__VA_ARGS__)
#endif /* _DEBUG_H_ */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)   (sizeof(x)/sizeof(x[0]))
#endif /* ARRAY_SIZE */

typedef struct Identity_
{
	uid_t uid;
	gid_t gid;
	int nbgroups;
	gid_t groups[NGROUPS_MAX];
} Identity;

static inline int __attribute__((warn_unused_result)) dropPrivileges(Identity *current)
{
	/*  CERT POS36-C compliant */

	int error = EXIT_SUCCESS;
	const bool permanent = (NULL == current);
	const uid_t uid = getuid();
	const gid_t gid = getgid();
	const uid_t euid = geteuid();
	const gid_t egid = getegid();

	if (current)
	{
		current->gid = egid;
		current->uid = euid;
		current->nbgroups = getgroups(NGROUPS_MAX,current->groups);
	}

	if (0 == euid)
	{
		if (setgroups(1,&gid) == -1)
		{
			error = errno;
			ERROR_MSG("setgroups %u error %d (%m)",gid,error);
			goto dropPrivilegesError; /* %RELAX<NOGOTO> : C_JMP_3 Guide DGA/MI n 518 S-CAT Ed01 p77 */
		}
	}

	if (gid != egid)
	{
		if (setregid(permanent?gid:((gid_t)-1),gid) == -1)
		{
			error = errno;
			ERROR_MSG("setregid %u error %d (%m)",gid,error);
			goto dropPrivilegesError; /* %RELAX<NOGOTO> : C_JMP_3 Guide DGA/MI n 518 S-CAT Ed01 p77 */
		}
	}

	if (uid != euid)
	{
		if (setreuid(permanent?uid:((uid_t)-1),uid) == -1)
		{
			error = errno;
			ERROR_MSG("setreuid %u error %d (%m)",uid,error);
			goto dropPrivilegesError; /* %RELAX<NOGOTO> : C_JMP_3 Guide DGA/MI n 518 S-CAT Ed01 p77 */
		}
	}

	/* check that everything went well (CERT POS37-C) */

	if (permanent)
	{
		if ((gid != egid) && ((setegid(egid) != -1) || (getegid() != gid)))
		{
			error = errno;
			ERROR_MSG("permanent egid %u check error %d (%m)",gid,errno);
			goto dropPrivilegesError; /* %RELAX<NOGOTO> : C_JMP_3 Guide DGA/MI n 518 S-CAT Ed01 p77 */
		}

		if ((uid != euid) && ((seteuid(euid) != -1) || (geteuid() != uid)))
		{
			error = errno;
			ERROR_MSG("permanent euid %u check error %d (%m)",gid,errno);
			goto dropPrivilegesError; /* %RELAX<NOGOTO> : C_JMP_3 Guide DGA/MI n 518 S-CAT Ed01 p77 */
		}
	}
	else
	{
		if (gid != egid && getegid() != gid)
		{
			error = errno;
			ERROR_MSG("egid %u check error %d (%m)",gid,errno);
			goto dropPrivilegesError; /* %RELAX<NOGOTO> : C_JMP_3 Guide DGA/MI n 518 S-CAT Ed01 p77 */
		} 

		if (uid != euid && geteuid() != uid)
		{
			error = errno;
			ERROR_MSG("euid %u check error %d (%m)",gid,errno);
			goto dropPrivilegesError; /* %RELAX<NOGOTO> : C_JMP_3 Guide DGA/MI n 518 S-CAT Ed01 p77 */
		}
	}

dropPrivilegesError:
	return error;
}

static inline int restorePrivileges(const Identity *original)
{
	int error = EXIT_SUCCESS;

	if (original)
	{
		if(geteuid() != original->uid)
		{
			if ((seteuid(original->uid) == -1) || (geteuid()!=original->uid))
			{
				error = errno;
				ERROR_MSG("seteuid %u error %d (%m)",original->uid,errno);
				goto restorePrivilegesError; /* %RELAX<NOGOTO> : C_JMP_3 Guide DGA/MI n 518 S-CAT Ed01 p77 */
			}
		}

		if (getegid() != original->gid)
		{
			if ((setegid(original->gid) == -1) || (getegid()!=original->gid))
			{
				error = errno;
				ERROR_MSG("setegid %u error %d (%m)",original->gid,errno);
				goto restorePrivilegesError; /* %RELAX<NOGOTO> : C_JMP_3 Guide DGA/MI n 518 S-CAT Ed01 p77 */
			}
		}

		if (original->uid != 0)
		{
			if (setgroups(original->nbgroups,original->groups) == -1)
			{
				error = errno;
				ERROR_MSG("setgroups %u error %d (%m)",original->gid,errno);
				goto restorePrivilegesError; /* %RELAX<NOGOTO> : C_JMP_3 Guide DGA/MI n 518 S-CAT Ed01 p77 */
			}
		}
	}
	else
	{
		error = EINVAL;
		ERROR_MSG("Invalid Identity parameter");
	}
restorePrivilegesError:
	return error;
}

static inline int __attribute__((warn_unused_result)) impersonateId(const uid_t uid,const gid_t gid,Identity *current)
{
	int error = EXIT_SUCCESS;
	const bool permanent = (NULL == current);
	const uid_t euid = geteuid();
	const gid_t egid = getegid();

	if (current)
	{
		current->gid = egid;
		current->uid = euid;
		current->nbgroups = getgroups(NGROUPS_MAX,current->groups);
	}

	if (0 == euid)
	{
		if (setgroups(1,&gid) != 0)
		{
			error = errno;
			ERROR_MSG("setgroups %u error %d (%m)",gid,error);
			goto impersonateIdError; /* %RELAX<NOGOTO> : C_JMP_3 Guide DGA/MI n 518 S-CAT Ed01 p77 */
		}
	}

	if (0 == euid)
	{
		if (setgroups(1,&gid) == -1)
		{
			error = errno;
			ERROR_MSG("setgroups %u error %d (%m)",gid,error);
			goto impersonateIdError; /* %RELAX<NOGOTO> : C_JMP_3 Guide DGA/MI n 518 S-CAT Ed01 p77 */
		}
	}

	if (gid != egid)
	{
		if (setregid(permanent?gid:((gid_t)-1),gid) == -1)
		{
			error = errno;
			ERROR_MSG("setregid %u error %d (%m)",gid,error);
			goto impersonateIdError; /* %RELAX<NOGOTO> : C_JMP_3 Guide DGA/MI n 518 S-CAT Ed01 p77 */
		}
	}

	if (uid != euid)
	{
		if (setreuid(permanent?uid:((uid_t)-1),uid) == -1)
		{
			error = errno;
			ERROR_MSG("setreuid %u error %d (%m)",uid,error);
			goto impersonateIdError; /* %RELAX<NOGOTO> : C_JMP_3 Guide DGA/MI n 518 S-CAT Ed01 p77 */
		}
	}

	/* check that everything went well (CERT POS37-C) */

	if (permanent)
	{
		if ((gid != egid) && ((setegid(egid) != -1) || (getegid() != gid)))
		{
			error = errno;
			ERROR_MSG("permanent egid %u check error %d (%m)",gid,errno);
			goto impersonateIdError; /* %RELAX<NOGOTO> : C_JMP_3 Guide DGA/MI n 518 S-CAT Ed01 p77 */
		}

		if ((uid != euid) && ((seteuid(euid) != -1) || (geteuid() != uid)))
		{
			error = errno;
			ERROR_MSG("permanent euid %u check error %d (%m)",gid,errno);
			goto impersonateIdError; /* %RELAX<NOGOTO> : C_JMP_3 Guide DGA/MI n 518 S-CAT Ed01 p77 */
		}
	}
	else
	{
		if (gid != egid && getegid() != gid)
		{
			error = errno;
			ERROR_MSG("egid %u check error %d (%m)",gid,errno);
			goto impersonateIdError; /* %RELAX<NOGOTO> : C_JMP_3 Guide DGA/MI n 518 S-CAT Ed01 p77 */
		}

		if (uid != euid && geteuid() != uid)
		{
			error = errno;
			ERROR_MSG("euid %u check error %d (%m)",gid,errno);
			goto impersonateIdError; /* %RELAX<NOGOTO> : C_JMP_3 Guide DGA/MI n 518 S-CAT Ed01 p77 */
		}
	}
impersonateIdError:
	return error;
}

static inline int __attribute__((warn_unused_result)) impersonate(const char *username,Identity *current)
{
	int error = EXIT_SUCCESS;
	struct passwd *passwordFileEntry = NULL;

	errno = 0;
	passwordFileEntry = getpwnam(username);
	if (likely(passwordFileEntry))
	{
		error = impersonateId(passwordFileEntry->pw_uid,passwordFileEntry->pw_gid,current);
	}
	else
	{
		if (likely(0 == errno))
		{
			errno = ENOENT;
		}
		error = errno;
		ERROR_MSG("getpwnam %s error %d (%m)",username,error);
	}
	return error;
}

static inline int __attribute__((warn_unused_result)) getIdentityOf(const char *username,uid_t *uid,gid_t *gid)
{
	int error = EXIT_SUCCESS;
	struct passwd *passwordFileEntry = NULL;

	errno = 0;
	passwordFileEntry = getpwnam(username);
	if (passwordFileEntry)
	{
		*uid = passwordFileEntry->pw_uid;
		*gid = passwordFileEntry->pw_gid;
	}
	else
	{
		if (0 == errno)
		{
			errno = ENOENT;
		}
		error = errno;
		ERROR_MSG("getpwnam %s error %d (%m)",username,error);
	}
	return error;
}

static inline int __attribute__((warn_unused_result)) setid(const uid_t uid, const gid_t gid) {
	int error = EXIT_SUCCESS;
	if (setegid(gid) == 0) {
		if (seteuid(uid) == -1) {
			error = errno;
			ERROR_MSG("seteuid %d error %d (%m)",error,uid);
		}
	}  else {
		error = errno;
		ERROR_MSG("setegid %d error %d (%m)",error,gid);
	}

	return error;
}

static inline int __attribute__((warn_unused_result)) restoreid(const uid_t uid, const gid_t gid) {
	int error = EXIT_SUCCESS;
	if (seteuid(uid) == 0) {
		if (setegid(gid) == -1) {
			error = errno;
			ERROR_MSG("setegid %d error %d (%m)",error,gid);
		}
	}  else {
		error = errno;
		ERROR_MSG("seteuid %d error %d (%m)",error,uid);
	}

	return error;
}

static inline int __attribute__((warn_unused_result)) setid(const char *loginName) {
	int error = EXIT_SUCCESS;
	struct passwd *passwordFileEntry = NULL;

	errno = 0;
	passwordFileEntry = getpwnam(loginName);
	if (passwordFileEntry) {
		error = setid(passwordFileEntry->pw_uid,passwordFileEntry->pw_gid);
	} else {
		if (0 == errno)
		{
			errno = ENOENT;
		}
		error = errno;
		ERROR_MSG("getpwnam %s error %d (%m)",loginName,error);
	}
	return error;
}

static inline int __attribute__((warn_unused_result)) restoreid(const char *loginName) {
	int error = EXIT_SUCCESS;
	struct passwd *passwordFileEntry = NULL;

	errno = 0;
	passwordFileEntry = getpwnam(loginName);
	if (passwordFileEntry) {
		error = restoreid(passwordFileEntry->pw_uid,passwordFileEntry->pw_gid);
	} else {
		if (0 == errno)
		{
			errno = ENOENT;
		}
		error = errno;
		ERROR_MSG("getpwnam %s error %d (%m)",loginName,error);
	}
	return error;
}

#endif /* _PRIVILEGES_H_ */

