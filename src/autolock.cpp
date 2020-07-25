#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define EOL "\n"

#include "debug.h"
#include "Parameters.h"
#include "Exception.h"
#include "capabilities.h"
#include "tools.h"
#include "daemon.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <linux/limits.h>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <pwd.h>
#include <signal.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>


#define O(l,s,t,o)	X(l,s,t,o)

#define CMDLINE_OPTS_TABLE \
		O(device,d,"=<device> keyboard device",NEED_ARG) \
		O(time,t,"=<time> duration (in minutes) without activity before starting program",NEED_ARG) \
		O(program,p,"=<program> locker program and its parameters to start",NEED_ARG) \
		O(user,u,"=<loginname> user account locker program to use",NEED_ARG) \
		O(lock-now,L," run the locker program at startup",NO_ARG) \
		O(failure-restart,r,"=<max> restart the locker program up to max times in case of failure",NEED_ARG) \
		O(configuration-file,c,"=<file> set configuration file to use",NEED_ARG) \
		O(console,C," console mode",NO_ARG) \
		O(log-level,l,"=<level> (optional, default is warning)",NEED_ARG) \
		O(help,h,"",NO_ARG) \
		O(version,v,"",NO_ARG) \

static const struct option longopts[] =
{
#define NEED_ARG	required_argument
#define NO_ARG		no_argument
#define OPT_ARG		optional_argument
#define X(l,s,t,o)	{ TO_STRING(l),o,NULL,TO_STRING(s)[0] },
		CMDLINE_OPTS_TABLE
#undef X
#undef NEED_ARG
#undef NO_ARG
#undef OPT_ARG
		{ NULL, 0, NULL, 0 }
};

static inline void printVersion(void)
{
	printf(PACKAGE_STRING EOL);
}

static inline void printHelp(const char *errorMsg = NULL)
{
#define X(l,s,t,o) "-" TO_STRING(s) ", --" TO_STRING(l) t EOL

#define USAGE "Usage: " TO_STRING(PROGNAME) " [OPTIONS]" EOL

	if (errorMsg != NULL) {
		fprintf(stderr, "Error %s" EOL USAGE CMDLINE_OPTS_TABLE, errorMsg);
	} else {
		fprintf(stdout, USAGE CMDLINE_OPTS_TABLE);
	}
#undef X
#undef USAGE
}


static int parseCmdLine(int argc, char *argv[], Parameters &parameters)
{
#define NEED_ARG	":"
#define NO_ARG		""
#define OPT_ARG		"::"
#define X(l,s,t,o) TO_STRING(s) o

	int error = EXIT_SUCCESS;
	const uid_t euid = geteuid();

	/*if (0 == euid) {
		error = setid("nobody"); //tmp move to nobody account during configuration (no need to be root & this will drop all it's effective root's capabilities)
	}*/

	if (EXIT_SUCCESS == error) {
		std::string errorMsg;
		int optc;

		while (((optc = getopt_long(argc, argv, CMDLINE_OPTS_TABLE, longopts, NULL)) != -1)
				&& (EXIT_SUCCESS == error)) {
			switch (optc)
			{
			case 'd':
				error = parameters.set_device(optarg);
				break;
			case 'p':
				error = parameters.set_program(optarg);
				break;
			case 'u':
				error = parameters.set_user(optarg);
				break;
			case 't':
				error = parameters.set_duration(optarg);
				break;
			case 'L':
				parameters.modes |= e_LockOnStartup;
				break;
			case 'l':
				error = parameters.set_syslogLevel(optarg);
				break;
			case 'c':
				error = parameters.set_configurationFile(optarg);
				break;
			case 'C':
				parameters.modes |= e_Console;
				break;
			case 'h':
				printHelp(NULL);
				exit(EXIT_SUCCESS);
				break;
			case 'v':
				printVersion();
				exit(EXIT_SUCCESS);
				break;
			case '?':
				error = EINVAL;
				printHelp("");
				break;
			default:
				error = EINVAL;
				printHelp("invalid parameter");
				break;
			} //switch(optc)
		} //while(((optc = getopt_long(argc,argv,"cln:phv",longopts,NULL))!= -1) && (EXIT_SUCCESS == error))

#undef X
#undef NEED_ARG
#undef NO_ARG
#undef OPT_ARG

		DEBUG_VAR(parameters.modes,"0x%X");
		if (parameters.isValid(errorMsg)) {
			/*if (0 == euid) {
					error = restoreid(0,0); // move back to root account and restore all initial effective capabilities
				}*/
		} else {
			error = EINVAL;
			printHelp(errorMsg.c_str());
		}

	} else {
		CRIT_MSG("Tmp move to account nobody error %d",error);
	}
	return error;
}

int __attribute__((warn_unused_result)) impersonate(const char *username) {
	int error = EXIT_SUCCESS;
	struct passwd *passwordFileEntry = NULL;

	errno = 0;
	passwordFileEntry = getpwnam(username);
	if (likely(passwordFileEntry)) {
		try {
			// Permanent change (all) UID & GID
			cap_value_t effectCapNeeded[] = {
					CAP_SETUID,CAP_SETGID
			};
			MANAGE_EFFECTIVE_CAP(effectCapNeeded);

			if (likely(setgid(passwordFileEntry->pw_uid) == 0)) {
				if (unlikely(setuid(passwordFileEntry->pw_gid) == -1)) {
					error = errno;
					ERROR_MSG("setuid %s (%d) error %d (%m)",username,passwordFileEntry->pw_gid,error);
				}
			} else {
				error = errno;
				ERROR_MSG("setgid %s (%d) error %d (%m)",username,passwordFileEntry->pw_uid,error);
			}

		} //try
		catch(EffectiveCapabilitiesManager::Exception &e) {
			ERROR_MSG("EffectiveCapabilitiesManager::Exception %s",e.what());
			error = e.code();
		}
	} else {
		errno = ENOENT;
		error = errno;
		ERROR_MSG("getpwnam %s error %d (%m)",username,error);
	}

	return error;
}

static int runLocker(const Parameters &parameters,unsigned int nb_failures = 0) {
	int error = EXIT_SUCCESS;
	const char *program = parameters.argv[0];
	const pid_t pid = fork();
	if (likely(pid > 0)) {
		/* parent */
		int status = EXIT_SUCCESS;
		NOTICE_MSG("Locker program %s started, PID = %d",program,pid);
		pid_t child = waitpid(pid,&status,0);
		if (likely(child != -1)) {
			NOTICE_MSG("Locker program %s ended, status = %d",program,WEXITSTATUS(status));
			if (unlikely(WIFSIGNALED(status))) {
				if (unlikely(WIFSIGNALED(status))) {
					ERROR_MSG("child process %s ended by signal %d",program,WTERMSIG(status));
				}
				if (unlikely(WCOREDUMP(status))) {
					ERROR_MSG("child process %s has produced a core dump",program);
				}

				nb_failures++;
				if ((parameters.nbRestart) && (nb_failures <= parameters.nbRestart)) {
					NOTICE_MSG("Locker %s restart %u",program,nb_failures);
					error = runLocker(parameters, nb_failures);
				}
			}
		} else {
			error = errno;
			ERROR_MSG("waitpid error %d (%m)",error);
		}
	} else if (0 == pid) {
		/* child */
		if (!parameters.user.empty()) {
			error = impersonate(parameters.user.c_str());
			if (unlikely(error != EXIT_SUCCESS)) {
				ERROR_MSG("impersonate %s error %d",parameters.user.c_str(),error);
			}
		}

		const uid_t euid = geteuid();
		if (0 == euid) {
			//TODO
			TODO(Capabilities management);
			WARNING_MSG("Program %s will be started as root (without any capabilities)",program);
			//error = dropAllCapabilities();
		}

		if (likely(EXIT_SUCCESS == error)) {
			if(execvp(program,(char* const*)parameters.argv.data()) == -1) {
				error = errno;
				CRIT_MSG("execvp %s error %d (%m)",program,error);
			}
		} else {
			ERROR_MSG("impersonate %s error %d",parameters.user.c_str(),error);
		}
	} else {
		error = errno;
		ERROR_MSG("fork error %d (%m)",error);
	}
	return error;
}

static inline int getTime(struct timespec &t) {
	int error= EXIT_SUCCESS;
	if (unlikely(clock_gettime(CLOCK_MONOTONIC,&t) == -1)) {
		error = errno;
		ERROR_MSG("clock_gettime CLOCK_MONOTONIC error %d (%m)",error);
		t.tv_sec = 0;
	}
	return error;
}

static void onSigHup(int receivedSignal)
{
	NOTICE_MSG("SIGHUP (%d) received",receivedSignal);
	TODO(Sighup handler);
}

static void onSigTerm(int receivedSignal)
{
	NOTICE_MSG("Signal (%d) received",receivedSignal);
	TODO(Signal handler);
}

TODO(syscall filter);

static int autolock(const Parameters &parameters) {
	int error= EXIT_SUCCESS;
	char keyboardDevice[PATH_MAX];
	snprintf(keyboardDevice,sizeof(keyboardDevice),"/dev/input/%s",parameters.device.c_str());
	int fd = open(keyboardDevice,O_RDONLY);
	if (likely(fd != -1)) {
		int epollfd = epoll_create1(0);
		if (likely(epollfd != -1)) {
			struct epoll_event events;
			events.data.fd = fd;
			events.events = EPOLLIN;
			if (likely(epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&events) != -1)) {
				int timeout = parameters.duration * 1000; // sec -> millisec

				if ((parameters.modes & e_LockOnStartup) == e_LockOnStartup) {
					NOTICE_MSG("Run locker on startup enabled");
					runLocker(parameters); // anyway go on, errors if any are already logged
				}

				do {
					struct timespec startWaitTime;
					getTime(startWaitTime);
					int nb_fds = epoll_wait(epollfd, &events, 1, timeout);
					if (likely(nb_fds != -1) ){
						if (nb_fds) {
							struct input_event event;
							const ssize_t n = read(fd,&event,sizeof(event));
							if (likely(n != -1)) {
								DEBUG_VAR(event.type,"%u");
								switch (event.type) {
								// cf. event-codes.txt in the Linux Kernel documentation
								case EV_KEY:
									// break is missing
								case EV_REL:
									// break is missing
								case EV_ABS:
									// reset the time out
									timeout = parameters.duration * 1000; // sec -> millisec
									break;
								default: {
									// ignore this event
									struct timespec elapsedWaitTime = {0,0};
									const int newDurationError = getElapsedTime(startWaitTime,elapsedWaitTime);
									if (likely(EXIT_SUCCESS == newDurationError)) {
										timeout -= (elapsedWaitTime.tv_sec * 1000);
										if (unlikely(timeout < 0)) {
											timeout = 0;
										}
									} else {
										WARNING_MSG("Failed to computed current elapsed time since last event => no timeout update");
									}
								}
								break;
								}
							} else if (-1 == n) {
								error = errno;
								ERROR_MSG("read %s error %d (%m)",keyboardDevice,error);
							}
						} else {
							// time out
							DEBUG_MSG("Time out (%d ms) !!!",timeout);
							error = runLocker(parameters);
							timeout = parameters.duration * 1000; // sec -> millisec
						}
					} else {
						error = errno;
						ERROR_MSG("epoll_wait error %d (%m)",error);
					}
				} while(EXIT_SUCCESS == error);

			} else {
				error = errno;
				ERROR_MSG("epoll_ctl error %d (%m)",error);
			}
		} else {
			error = errno;
			ERROR_MSG("epoll_create1 error %d (%m)",error);
		}
		close(fd);
		fd = -1;
	} else {
		error = errno;
		ERROR_MSG("open %s error %d (%m)",keyboardDevice,error);
	}
	return error;
}

int main(int argc, char *argv[]) {
	Parameters parameters;
	openlog(PACKAGE_NAME, LOG_PID|LOG_CONS, LOG_AUTH);
	int error = parseCmdLine(argc, argv, parameters);
	if (likely(EXIT_SUCCESS == error)) {
		pid_t daemonProcessId = 0;
		if (likely((parameters.modes & e_Console) != e_Console)) {
			error = deamonize(onSigHup, onSigTerm, &daemonProcessId);
			NOTICE_MSG(PROGNAME " daemon started (PID = %u,error = %d)",daemonProcessId,error);
		}

		if ((EXIT_SUCCESS == error) && (0 == daemonProcessId)) { // daemonProcessId != 0 means the current running process is the parent/launcher
			error = autolock(parameters);
			NOTICE_MSG(PROGNAME " daemon ended with status %d",error);
		}
	} else {
		CRIT_MSG("Configuration failure (error %d), aborting...",error);
	}
	closelog();
	return error;
}

#include <ModuleVersionInfo.h>
MODULE_NAME_AUTOTOOLS;
MODULE_AUTHOR_AUTOTOOLS;
MODULE_VERSION_AUTOTOOLS;
MODULE_FILE_VERSION(0.0.0.1);
MODULE_QUOTED_DESCRIPTION("Daemon to manage screen locker for consoles and tty");
MODULE_COMMENTS("");
MODULE_COPYRIGHT(GPL);
MODULE_SCM_LABEL_AUTOTOOLS;

