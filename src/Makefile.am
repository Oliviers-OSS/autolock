sbin_PROGRAMS=autolockd

autolockd_SOURCES = autolock.cpp 
autolockd_SOURCES += Parameters.h Parameters.cpp 
autolockd_SOURCES += daemon.h daemon.cpp 
autolockd_SOURCES += ini.h ini.c
autolockd_SOURCES += privileges.h containment.h AsRoot.h syscall-filter.h capabilities.h
autolockd_SOURCES += ModuleVersionInfo.h  InnerException.h Exception.h
autolockd_SOURCES += tools.h debug.h debug.cpp tools.cpp

#if SYSCALL_REPORTER_ENABLED
#include syscall-reporter.mk
#autolockd_SOURCES += syscall-reporter.c syscall-reporter.h syscall-names.h
#endif

autolockd_CPPFLAGS =-D_SCM_LABEL="$(SCM_LABEL)"
autolockd_CPPFLAGS +=-D_SCM_HASH="$(SCM_HASH)"
autolockd_CPPFLAGS +=-D_CONFIGDIR="$(sysconfdir)"
autolockd_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir)/src
autolockd_CPPFLAGS += `pkg-config --cflags libcap`

autolockd_CXXFLAGS = -Wall -Wextra -Wformat
#autolockd_CXXFLAGS += -Wconversion -Winline  
autolockd_CXXFLAGS += -Wmissing-include-dirs -Wmissing-noreturn  -Wpacked  -Wparentheses
autolockd_CXXFLAGS += -Wpointer-arith -Wformat-nonliteral -Winit-self -Wwrite-strings
autolockd_CXXFLAGS += -Wmissing-braces  -Wmissing-field-initializers -Wmissing-format-attribute
autolockd_CXXFLAGS += -Wcast-qual -Wfloat-equal -Wshadow
autolockd_CXXFLAGS += -fstack-protector -Wl,-z,relro -Wl,-z,now -Wformat-security  


autolockd_LDFLAGS = -lpthread
autolockd_LDFLAGS += `pkg-config --libs libcap`
autolockd_LDFLAGS += -fPIE -flto -Wl,--build-id=sha1 -rdynamic

#all-local:
#	sudo /sbin/setcap "all=p" autolockd 
#	/sbin/getcap autolockd

#install-exec-hook:
#	sudo /sbin/setcap "all=p" $(DESTDIR)$(sbindir)/autolockd
#	sudo /usr/sbin/adduser --system --group --force-badname --no-create-home --disabled-password scop-manager

#dist-hook:
#	cd $(distdir); rm -f $(BUILT_SOURCES)

