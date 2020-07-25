dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.69])
AC_INIT([autolock],[0.0.0.1],[oliviers.open.source.softwares@gmail.com])
AC_CONFIG_SRCDIR([src/autolock.cpp])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

AC_CANONICAL_TARGET

dnl Initialize automake
AM_INIT_AUTOMAKE([dist-bzip2 dist-xz])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_MAKE_SET

# Checks for header files.
AC_CHECK_HEADER_STDBOOL
AC_CHECK_HEADERS([stdlib.h stddef.h syslog.h fcntl.h sys/param.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UID_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_GETGROUPS
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([atexit clock_gettime fchdir ftruncate mkdir memset rmdir strcasecmp strchr strdup strtoul ])

# Checks for pkg config and needed parameters files
PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES(libcap, [libcap],,
    AC_MSG_ERROR([You need the cap library]
    [])
)

dnl parameters

dnl debug
AC_ARG_ENABLE(debug,
        [  --enable-debug enable debug mode ],
        [case "${enableval}" in
         yes) enable_debug="yes";;
          no) enable_debug="no" ;;
           *) AC_MSG_ERROR(bad value ${enableval} for --enable-debug) ;;
         esac],
        [enable_debug="no"]
)
if test "$enable_debug" = "yes"; then
        AC_DEFINE(_DEBUG_, 1, [Defined if debug mode is enabled (it's easier to check).])
        #CFLAGS="$CFLAGS -O0 -ggdb3"
fi
if test "$enable_debug" = "no"; then
        AC_DEFINE(NDEBUG, 1, [Defined if debug mode is disabled.])
fi

AC_ARG_ENABLE(debug-console,
        [  --enable-debug-console enable debug messages on console ],
        [case "${enableval}" in
         yes) enable_debug_console="yes";;
          no) enable_debug_console="no" ;;
           *) AC_MSG_ERROR(bad value ${enableval} for --enable-debug-console) ;;
         esac],
        [enable_debug_console="no"]
)
if test "$enable_debug_console" = "yes"; then
        AC_DEFINE(_CONSOLE_, 1, [Defined if debug console mode is enabled.])
        #CFLAGS="$CFLAGS -O0 -ggdb3"
fi

if test "${enable_retail}" = "yes"; then
        AC_DEFINE(_RETAIL_,1,[Defined is retail mode is enabled.])
fi

# Documentation
AC_CHECK_PROGS([DOXYGEN], [doxygen])
AC_CHECK_PROGS([DOT], [dot])
AC_CHECK_PROGS([MSCGEN], [mscgen])
if test -z "$DOXYGEN";
   then AC_MSG_WARN([Doxygen not found - continuing without Doxygen support])
fi

# Service
PKG_PROG_PKG_CONFIG
AC_ARG_WITH([systemdsystemunitdir],
	AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [Directory for systemd service files]),,
	[with_systemdsystemunitdir=auto])
   AS_IF([test "x$with_systemdsystemunitdir" = "xyes" -o "x$with_systemdsystemunitdir" = "xauto"], [
        def_systemdsystemunitdir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd)

        AS_IF([test "x$def_systemdsystemunitdir" = "x"],
            [AS_IF([test "x$with_systemdsystemunitdir" = "xyes"],
                   [AC_MSG_ERROR([systemd support requested but pkg-config unable to query systemd package])])
             with_systemdsystemunitdir=no],
            [with_systemdsystemunitdir=$def_systemdsystemunitdir])])
   AS_IF([test "x$with_systemdsystemunitdir" != "xno"],
         [AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])])
AM_CONDITIONAL(HAVE_SYSTEMD, [test "x$with_systemdsystemunitdir" != "xno"])
     
#AC_ARG_ENABLE([absolute-configuration-files-path],
#	AS_HELP_STRING([--disable-absolute-configuration-files-path], [disable configuration files located in /etc instead of sysconfdir/etc @<:@default=enabled@:>@]),
#	[case "${enableval}" in
#         yes) enable_modprobe_abspath="yes";;
#          no) enable_modprobe_abspath="no" ;;
#           *) AC_MSG_ERROR(bad value ${enableval} for --disable-absolute-configuration-files-path) ;;
#         esac], [enable_modprobe_abspath=yes]
#)
#AM_CONDITIONAL([ENABLE_MODPROBE_ABS_PATH], [test "x$enable_modprobe_abspath" = "xyes"])
           
# RPM
AC_ARG_WITH([rpmbuild-dir],
  [AS_HELP_STRING([--with-rpmbuild-dir],[location of the rpmbuild directory (optional, default $HOME/rpmbuild)])],
  [RPMBUILD_DIR="$withval"],
  [RPMBUILD_DIR="$HOME/rpmbuild"])
AC_SUBST(RPMBUILD_DIR)

AC_ARG_ENABLE(rpm-mode,
	[  --enable-rpm-mode rpm build special mode],
        [case "${enableval}" in
         yes) rpm_mode="yes";;
          no) rpm_mode="no" ;;
           *) AC_MSG_ERROR(bad value ${enableval} for --rpm_mode) ;;
         esac],
        [rpm_mode="no"]
)
AM_CONDITIONAL([RPM_MODE],[test "$rpm_mode" = "yes"])

AC_ARG_WITH([rpmbuild-pkgconfig-path],
  [AS_HELP_STRING([--with-rpmbuild-pkgconfig-path],[pkgconfig-path to use during rpmbuild directory (optional, default $PKG_CONFIG_PATH if set)])],
  [RPMBUILD_PKG_CONFIG_PATH="$withval"],
  [RPMBUILD_PKG_CONFIG_PATH="$PKG_CONFIG_PATH"])
AC_SUBST(RPMBUILD_PKG_CONFIG_PATH)

AC_SUBST(SCM_LABEL,[$(git describe --exact-match --tags $(git log -n1 --pretty='%n') 2>/dev/null)])
AC_SUBST(SCM_HASH,[$(git log -n1 --pretty='%H') 2>/dev/null])

# Qualimetry
AC_ARG_WITH([counttool-dir],
  [AS_HELP_STRING([--with-counttool-dir],[location of the comptage tool directory (optional, default is $HOME/comptage)])],
  [COMPTAGE_ROOT="$withval"],
  [COMPTAGE_ROOT="$HOME/comptage"])
AC_SUBST(COMPTAGE_ROOT)

AC_CONFIG_FILES(Makefile 
	src/Makefile
	autolock	
	autolock.service
	)
AC_OUTPUT

