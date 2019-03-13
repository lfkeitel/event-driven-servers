############################################################## vim: ft=make ts=4
# Makefile.inc
# (C) 1997-2009 Marc Huber <Marc.Huber@web.de>
################################################################################

ifeq ($(MAKEFILE_INC_INCLUDED),)
MAKEFILE_INC_INCLUDED=1

TR	= /usr/bin/tr

ifneq ($(wildcard /usr/xpg4/bin/tr),)
	TR	= /usr/xpg4/bin/tr
endif

OSs	:= $(shell uname -s | env -i $(TR) "[:upper:]/ " "[:lower:]--")
OSr	:= $(shell uname -r | sed "s/(.*)//" | env -i $(TR) "[:upper:]/ " "[:lower:]--")
OSm	:= $(shell uname -m | env -i $(TR) "[:upper:]/ " "[:lower:]--")
OS	:= $(OSs)-$(OSr)-$(OSm)
OSTYPE	:= $(shell echo $(OSs) | sed "s/-.*//")
OSLEVEL	:= $(shell printf "0x%.2x%.2x%.4x" `echo $(OSr) | env -i $(TR) -c "[:digit:]" "[ *]"` 0 0 0 | cut -c-10)

MF_OBJ=build/Makefile.inc.$(OS)
ifeq ($(wildcard $(MF_OBJ)),)
	MF_OBJ=../build/Makefile.inc.$(OS)
	ifeq ($(wildcard $(MF_OBJ)),)
		MF_OBJ=../../build/Makefile.inc.$(OS)
		ifeq ($(wildcard $(MF_OBJ)),)
			MF_OBJ=../../../build/Makefile.inc.$(OS)
			ifeq ($(wildcard $(MF_OBJ)),)
$(error Please run the configure script to create Makefile.inc.$(OS))
			endif
		endif
	endif
endif

include $(MF_OBJ)

CC	?= gcc

CCV = $(shell set $(CC); $$1 -v 2>&1)

ifeq ($(findstring gcc,$(CCV)),)
	CC_GCC	=	0
	ifneq ($(findstring clang,$(CCV)),)
		CC_GCC		= 1
	endif
	ifneq ($(findstring pcc,$(CCV)),)
		CC_PCC		= 1
	endif
	CCV = $(shell $(CC) -V 2>&1)
	ifneq ($(findstring Intel,$(CCV)),)
		CC_INTEL	= 1
	endif
	ifneq ($(findstring "Sun C",$(CCV)),)
		CC_SUN		= 1
	endif
else
	CC_GCC	= 1
endif

LD	?= ld

LDV = $(shell set $(LD); $$1 -v 2>&1)

ifeq ($(findstring GNU,$(LDV)),)
	LD_GNU	= 0
else
	LD_GNU	= 1
endif

ifeq ($(CC_GCC),1)
	ifeq ($(findstring "4.8.2",$CCV),)
			CFLAGS		?= -Wall -W -Wno-strict-prototypes
	else
			CFLAGS		?= -O -Wall -W -Wno-strict-prototypes
	endif
	CFLAGS_PIC	?= -fPIC

	LDOPT_R		?= -Wl,-rpath,
	LD_SHARED	= $(CC) -shared
	ifeq ($(DEBUG),1)
		CFLAGS	+= -Werror
	endif
endif

ifeq ($(CC_PCC),1)
	# Currently broken -MH20090322
	CFLAGS		?= -O
	CFLAGS_PIC	?= -fPIC
	LDOPT_R		?= -Wl,-rpath,
	LD_SHARED	= $(CC) -shared
endif

ifeq ($(CC_SUN),1)
	#CFLAGS		?= -O -errwarn
	CFLAGS		?= -O
	CFLAGS_PIC	?= -G
	LDOPT_R		?= -Wl,-rpath,
	LD_SHARED	= $(CC) $(CFLAGS_PIC)
endif

ifeq ($(CC_INTEL),1)
	# May work, or possibly not.
	ifeq ($(findstring "Version 11.0",$CCV),)
# mostly GCC compatible, apparently ...
#		CFLAGS		?= -O
#		CFLAGS_PIC	?= -KPIC
#		LDOPT_R		?= -Qoption,link,-R
#		LD_SHARED	= $(CC) -shared
	else
		ifneq ($(findstring "Version 7.0",$(CCV)),)
			CFLAGS	+= -restrict
		endif
		CFLAGS		?= -O -Wall -Wno-strict-prototypes -wd981 -wd2259 -wd1684
		CFLAGS_PIC	?= -fPIC
		LDOPT_R		?= -Wl,-rpath,
		LD_SHARED	= $(CC) -shared
		ifeq ($(DEBUG),1)
			CFLAGS	+= -Werror
		endif
	endif
endif

SHLIB_PRE	= lib
SHLIB_EXT	= .so

ifeq ($(OSTYPE), darwin)
	LD_SHARED	= $(CC) -dynamiclib 
	ifeq ($(CC_GCC), 1)
	# CFLAGS		+= -Wno-shorten-64-to-32
	# CFLAGS		+= -arch i386 -arch x86_64 -arch ppc -arch ppc64
		ifeq ($(shell expr 11 - `uname -r | cut -c-2` \<= 0), 1)
			# MacOS 10.7 deprecates a couple of SSL functions. Disable compiler warnings.
			CFLAGS	+= -Wno-deprecated-declarations
		endif
	endif
	SHLIB_EXT	= .dylib
	LIB_MAVIS	= '$(LDOPT_R)@loader_path/../lib'
endif

ifeq ($(OSTYPE), cygwin_nt)
	CFLAGS_PIC	=
	SHLIB_PRE	= cyg
	SHLIB_EXT	= .dll
	EXEC_EXT	= .exe
endif

CFLAGS += $(AUX_CFLAGS)

INSTALL	?= install -c

ifeq ($(OSTYPE), sunos)
	ifdef LD_LIBRARY_PATH
$(error LD_LIBRARY_PATH may not be set for Solaris)
	endif
	LIB_NET	= -lnsl -lsocket
	ifeq ($(LD_GNU), 0)
		LDOPT_R	= -R
		LD_SHARED = $(LD) -G -z textwarn
	endif
	ifneq ($(CC_GCC), 1)
		CFLAGS	?= -xO3 -xstrconst -xdepend -Xa -errwarn=%all
		CFLAGS_PIC = -KPIC
	endif
	DEF	+= -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
	INSTALL = /usr/bin/ginstall
	ifeq ($(wildcard $(INSTALL)),)
		INSTALL = /usr/ucb/install
	endif
	ifeq ($(wildcard $(INSTALL)),)
		INSTALL = /usr/bin/install
	endif
	LIB_MAVIS	= '$(LDOPT_R)$$ORIGIN/../lib'
endif

ifeq ($(OSTYPE), linux)
	DEF	+= -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
	LIB_MAVIS	= '$(LDOPT_R)$$ORIGIN/../lib$(LIBARCH)'
endif

DEF	+= -DOSTYPE=$(OSTYPE) -DOSLEVEL=$(OSLEVEL) -DOS="\"$(OS)\""

ifeq ($(DEBUG),1)
	DEF	+= -DDEBUG
	CFLAGS	+= -g
	ifeq ($(CC_GCC), 1)
		CFLAGS	+= -ggdb
	endif
	CC_GCC	= 1
endif

DIR_MAVIS	= $(BASE)/mavis
LIB_MAVIS	+= "-L$(BASE)/build/$(OS)/mavis" "$(LDOPT_R)$(LIBARCHDIR_DEST)" -lmavis
DIR_MISC	= $(BASE)/misc
INC			+= -I$(BASE)

# Check for lwres library:

ifeq ($(WITH_LWRES),)
	ifneq ($(wildcard /lib$(LIBARCH)/liblwres$(SHLIB_EXT)* /usr/lib$(LIBARCH)/liblwres$(SHLIB_EXT)*),)
		ifneq ($(wildcard /usr/include/lwres),)
			WITH_LWRES=1
		endif
	endif
endif

ifeq ($(WITH_LWRES),)
	ifneq ($(wildcard /usr/local/lib$(LIBARCH)/liblwres$(SHLIB_EXT)*),)
		ifneq ($(wildcard /usr/local/include/lwres),)
			INCDIR_LWRES=/usr/local/include
			LIBDIR_LWRES=/usr/local/lib/$(LIBARCH)
			WITH_LWRES=1
		endif
	endif
endif

ifeq ($(WITH_LWRES),)
	ifneq ($(wildcard /usr/sfw/lib$(LIBARCH)/liblwres$(SHLIB_EXT)*),)
		ifneq ($(wildcard /usr/sfw/include/lwres),)
			INCDIR_LWRES=/usr/sfw/include
			LIBDIR_LWRES=/usr/sfw/lib/$(LIBARCH)
			WITH_LWRES=1
		endif
	endif
endif

ifeq ($(WITH_LWRES),1)
	ifneq ($(INCDIR_LWRES),)
		INC_LWRES	= -I$(INCDIR_LWRES)
	endif
	ifneq ($(LIBDIR_LWRES),)
		LIB_LWRES	= -L$(LIBDIR_LWRES) $(LDOPT_R)$(LIBDIR_LWRES)
	endif
	DEF += -DWITH_LWRES
	LIB_LWRES	+= -llwres
endif

# Check for OpenSSL library:

ifeq ($(WITH_SSL),)
	ifneq ($(wildcard /lib$(LIBARCH)/libcrypto$(SHLIB_EXT)* /usr/lib$(LIBARCH)/libcrypto$(SHLIB_EXT)*),)
		ifneq ($(wildcard /lib$(LIBARCH)/libssl$(SHLIB_EXT)* /usr/lib$(LIBARCH)/libssl$(SHLIB_EXT)*),)
			ifneq ($(wildcard /usr/include/openssl),)
				WITH_SSL=1
			endif
		endif
	endif
endif

ifeq ($(WITH_SSL),)
	ifneq ($(wildcard /usr/local/lib$(LIBARCH)/libcrypto$(SHLIB_EXT)*),)
		ifneq ($(wildcard /usr/local/lib$(LIBARCH)/libssl$(SHLIB_EXT)*),)
			ifneq ($(wildcard /usr/local/include/openssl),)
				INCDIR_SSL=/usr/local/include
				LIBDIR_SSL=/usr/local/lib$(LIBARCH)
				WITH_SSL=1
			endif
		endif
	endif
endif

ifeq ($(WITH_SSL),)
	ifneq ($(wildcard /usr/sfw/lib$(LIBARCH)/libcrypto$(SHLIB_EXT)*),)
		ifneq ($(wildcard /usr/sfw/lib$(LIBARCH)/libssl$(SHLIB_EXT)*),)
			ifneq ($(wildcard /usr/sfw/include/openssl),)
				INCDIR_SSL=/usr/sfw/include
				LIBDIR_SSL=/usr/sfw/lib$(LIBARCH)
				WITH_SSL=1
			endif
		endif
	endif
endif

ifeq ($(WITH_SSL),1)
	ifneq ($(INCDIR_SSL),)
		INC_SSL		= -I$(INCDIR_SSL)
	endif
	ifneq ($(LIBDIR_SSL),)
		LIB_SSL		= -L$(LIBDIR_SSL) $(LDOPT_R)$(LIBDIR_SSL)
		LIB_SSL_CRYPTO	= -L$(LIBDIR_SSL) $(LDOPT_R)$(LIBDIR_SSL)
	endif
	DEF += -DWITH_SSL
	LIB_SSL	+= -lssl -lcrypto
	LIB_SSL_CRYPTO	+= -lcrypto
endif

# Check for PCRE library:

ifeq ($(WITH_PCRE),)
	ifneq ($(wildcard /lib$(LIBARCH)/libpcre$(SHLIB_EXT)* /usr/lib$(LIBARCH)/libpcre$(SHLIB_EXT)*),)
		ifneq ($(wildcard /usr/include/pcre.h),)
			WITH_PCRE=1
		endif
	endif
endif

ifeq ($(WITH_PCRE),)
	ifneq ($(wildcard /usr/local/lib$(LIBARCH)/libpcre$(SHLIB_EXT)*),)
		ifneq ($(wildcard /usr/local/include/pcre.h),)
			INCDIR_PCRE=/usr/local/include
			LIBDIR_PCRE=/usr/local/lib$(LIBARCH)
			WITH_PCRE=1
		endif
	endif
endif

ifeq ($(WITH_PCRE),)
	ifneq ($(wildcard /usr/sfw/lib$(LIBARCH)/libpcre$(SHLIB_EXT)*),)
		ifneq ($(wildcard /usr/sfw/include/pcre.h),)
			INCDIR_PCRE=/usr/sfw/include
			LIBDIR_PCRE=/usr/sfw/lib$(LIBARCH)
			WITH_PCRE=1
		endif
	endif
endif

ifeq ($(WITH_PCRE),1)
	ifneq ($(INCDIR_PCRE),)
		INC_PCRE	= -I$(INCDIR_PCRE)
	endif
	ifneq ($(LIBDIR_PCRE),)
		LIB_PCRE	= -L$(LIBDIR_PCRE) $(LDOPT_R)$(LIBDIR_PCRE)
	endif
	DEF += -DWITH_PCRE
	LIB_PCRE	+= -lpcre
endif

# Check for FreeRADIUS client library:

ifeq ($(WITH_FREERADIUS),)
	ifneq ($(wildcard /lib$(LIBARCH)/libfreeradius-client$(SHLIB_EXT)* /usr/lib$(LIBARCH)/libfreeradius-client$(SHLIB_EXT)*),)
		ifneq ($(wildcard /usr/include/freeradius-client.h),)
			WITH_FREERADIUS=1
		endif
	endif
endif

ifeq ($(WITH_FREERADIUS),)
	ifneq ($(wildcard /usr/local/lib$(LIBARCH)/libfreeradius-client$(SHLIB_EXT)*),)
		ifneq ($(wildcard /usr/local/include/freeradius-client.h),)
			INCDIR_FREERADIUS=/usr/local/include
			LIBDIR_FREERADIUS=/usr/local/lib$(LIBARCH)
			WITH_FREERADIUS=1
		endif
	endif
endif

ifeq ($(WITH_FREERADIUS),)
	ifneq ($(wildcard /usr/sfw/lib$(LIBARCH)/libfreeradius-client$(SHLIB_EXT)*),)
		ifneq ($(wildcard /usr/sfw/include/freeradius-client.h),)
			INCDIR_FREERADIUS=/usr/sfw/include
			LIBDIR_FREERADIUS=/usr/sfw/lib$(LIBARCH)
			WITH_FREERADIUS=1
		endif
	endif
endif

ifeq ($(WITH_FREERADIUS),1)
	ifneq ($(INCDIR_FREERADIUS),)
		INC_FREERADIUS	= -I$(INCDIR_FREERADIUS)
	endif
	ifneq ($(LIBDIR_FREERADIUS),)
		LIB_FREERADIUS	= -L$(LIBDIR_FREERADIUS) $(LDOPT_R)$(LIBDIR_FREERADIUS)
	endif
	LIB_FREERADIUS	+= -lfreeradius-client
endif

# Check for ZLIB library:

ifeq ($(WITH_ZLIB),)
	ifneq ($(wildcard /lib$(LIBARCH)/libz$(SHLIB_EXT)* /usr/lib$(LIBARCH)/libz$(SHLIB_EXT)*),)
		ifneq ($(wildcard /usr/include/zlib.h),)
			WITH_ZLIB=1
		endif
	endif
endif

ifeq ($(WITH_ZLIB),)
	ifneq ($(wildcard /usr/local/lib$(LIBARCH)/libz$(SHLIB_EXT)*),)
		ifneq ($(wildcard /usr/local/include/zlib.h),)
			INCDIR_ZLIB=/usr/local/include
			LIBDIR_ZLIB=/usr/local/lib$(LIBARCH)
			WITH_ZLIB=1
		endif
	endif
endif

ifeq ($(WITH_ZLIB),1)
	ifneq ($(INCDIR_ZLIB),)
		INC_ZLIB	= -I$(INCDIR_ZLIB)
	endif
	ifneq ($(LIBDIR_ZLIB),)
		LIB_ZLIB	= -L$(LIBDIR_ZLIB) $(LDOPT_R)$(LIBDIR_ZLIB)
	endif
	DEF += -DWITH_ZLIB
	LIB_ZLIB	+= -lz
endif

# Check for execinfo:

ifeq ($(WITH_EXECINFO),)
	ifneq ($(wildcard /usr/include/execinfo.h),)
		WITH_EXECINFO=1
	endif
	ifneq ($(wildcard /usr/lib/libexecinfo$(SHLIB_EXT)*),)
		LIB_EXECINFO	+= -lexecinfo
	endif
endif

ifeq ($(WITH_EXECINFO),)
	ifneq ($(wildcard /usr/local/lib/libexecinfo$(SHLIB_EXT)*),)
		ifneq ($(wildcard /usr/local/include/execinfo.h),)
			WITH_EXECINFO=1
			LIB_EXECINFO	+= -lexecinfo
			INCDIR_EXECINFO=/usr/local/include
			LIBDIR_EXECINFO=/usr/local/lib
		endif
	endif
endif

ifeq ($(WITH_EXECINFO),1)
	ifneq ($(INCDIR_EXECINFO),)
		INC_EXECINFO	= -I$(INCDIR_EXECINFO)
	endif
	ifneq ($(LIBDIR_EXECINFO),)
		LIB_EXECINFO	= -L$(LIBDIR_EXECINFO) $(LDOPT_R)$(LIBDIR_EXECINFO)
		LIB_EXECINFO	+= -lexecinfo
	endif
	DEF += -DHAVE_EXECINFO_H
endif

# Check for crypt library:

ifeq ($(HAS_LIBCRYPT),)
	ifneq ($(wildcard /lib$(LIBARCH)/libcrypt$(SHLIB_EXT)* /usr/lib$(LIBARCH)/libcrypt$(SHLIB_EXT)* /lib$(LIBARCH)/*/libcrypt$(SHLIB_EXT)*),)
		HAS_LIBCRYPT=1
	endif
endif

ifeq ($(HAS_LIBCRYPT),1)
	LIBCRYPT	= -lcrypt
	DEF += -DWITH_LIBCRYPT
endif

# Check for IPC:

ifeq ($(WITH_IPC),)
	ifneq ($(wildcard /usr/include/sys/ipc.h),)
		WITH_IPC=1
	endif
endif

# Check for CURL:

ifeq ($(WITH_CURL),)
	ifneq ($(wildcard /usr/lib$(LIBARCH)/libcurl$(SHLIB_EXT)*),)
		ifneq ($(wildcard /usr/include/curl),)
			WITH_CURL=1
		endif
	endif
endif

ifeq ($(WITH_CURL),)
	ifneq ($(wildcard /usr/local/lib/libcurl$(SHLIB_EXT)*),)
		ifneq ($(wildcard /usr/local/include/curl),)
			INCDIR_CURL=/usr/local/include
			LIBDIR_CURL=/usr/local/lib
			WITH_CURL=1
		endif
	endif
endif

ifeq ($(WITH_CURL),)
	# NetBSD, possibly
	ifneq ($(wildcard /usr/pkg/lib/libcurl$(SHLIB_EXT)*),)
		ifneq ($(wildcard /usr/pkg/include/curl),)
			INCDIR_CURL=/usr/pkg/include
			LIBDIR_CURL=/usr/pkg/lib
			WITH_CURL=1
		endif
	endif
endif

ifeq ($(WITH_CURL),1)
	ifneq ($(INCDIR_CURL),)
		INC_CURL	= -I$(INCDIR_CURL)
	endif
	ifneq ($(LIBDIR_CURL),)
		LIB_CURL	= -L$(LIBDIR_CURL) $(LDOPT_R)$(LIBDIR_CURL)
	endif
	LIB_CURL	+= -lcurl
endif

# Check for PAM:

ifeq ($(WITH_PAM),)
	# NetBSD
	ifneq ($(wildcard /usr/pkg/lib/libpam$(SHLIB_EXT)*),)
		ifneq ($(wildcard /usr/pkg/include/pam),)
			INCDIR_PAM=/usr/pkg/include
			LIBDIR_PAM=/usr/pkg/lib
			WITH_PAM=1
		endif
	endif
endif

ifeq ($(WITH_PAM),)
	ifneq ($(wildcard /usr/include/security/pam_appl.h),)
		DEF += -DHAVE_SECURITY_PAM_APPL_H
		WITH_PAM=1
	endif
endif


ifeq ($(WITH_PAM),)
	ifneq ($(wildcard /usr/lib$(LIBARCH)/libpam$(SHLIB_EXT)*),)
		ifneq ($(wildcard /usr/include/pam),)
			WITH_PAM=1
		endif
	endif
endif

ifeq ($(WITH_PAM),1)
	ifneq ($(INCDIR_PAM),)
		INC_PAM	= -I$(INCDIR_PAM)
	endif
	ifneq ($(LIBDIR_PAM),)
		LIB_PAM	= -L$(LIBDIR_PAM) $(LDOPT_R)$(LIBDIR_PAM)
	endif
	LIB_PAM	+= -lpam
endif

# Check for SCTP:

ifeq ($(WITH_SCTP),)
	ifneq ($(wildcard /usr/include/netinet/sctp.h),)
		WITH_SCTP=1
	endif
endif

ifeq ($(WITH_SCTP),1)
	ifneq ($(wildcard /lib$(LIBARCH)/libsctp$(SHLIB_EXT)* /usr/lib$(LIBARCH)/libsctp$(SHLIB_EXT)*),)
		LIBSCTP=-lsctp
	endif
	DEF += -DWITH_SCTP
endif

#

ifeq ($(OSTYPE), linux)
	LIB_DL	= -ldl
endif
	
ifeq ($(OSTYPE), sunos)
	LIB_DL	= -ldl
	ifeq ($(WITH_PORT),)
		ifneq ($(wildcard /usr/include/port.h),)
			WITH_PORT=1
		endif
	endif
	ifeq ($(WITH_DEVPOLL),)
		# Deprecated by Sun, and rightfully so.
		ifneq ($(wildcard /usr/include/sys/devpoll.h),)
			WITH_DEVPOLL=1
		endif
	endif
endif

ifeq ($(wildcard /usr/include/sys),)
		# FIXME. Consider removing Ubuntu from the list of supported systems.
		USRINCLUDESYS=/usr/include/$(shell uname -m)-linux-gnu/sys
else
		USRINCLUDESYS=/usr/include/sys
endif

ifeq ($(WITH_EPOLL),)
	ifneq ($(wildcard $(USRINCLUDESYS)/epoll.h),)
		WITH_EPOLL=1
	endif
endif

ifeq ($(WITH_KQUEUE),)
	ifneq ($(wildcard /usr/include/sys/event.h),)
		WITH_KQUEUE=1
	endif
endif

ifeq ($(WITH_POLL),)
	ifneq ($(wildcard $(USRINCLUDESYS)/poll.h),)
		WITH_POLL=1
	endif
endif

ifeq ($(WITH_SELECT),)
	ifneq ($(wildcard $(USRINCLUDESYS)/select.h),)
		WITH_SELECT=1
	endif
endif

ifeq ($(WITH_PORT)$(WITH_DEVPOLL)$(WITH_EPOLL)$(WITH_KQUEUE)$(WITH_POLL)$(WITH_SELECT),)
$(error None of the supported event polling mechanisms seems to be available on your system. Check your installation, some include files may be missing.)
endif

ifeq ($(WITH_PORT),1)
	DEF += -DWITH_PORT
endif

ifeq ($(WITH_DEVPOLL),1)
	DEF += -DWITH_DEVPOLL
endif

ifeq ($(WITH_EPOLL),1)
	DEF += -DWITH_EPOLL
endif

ifeq ($(WITH_KQUEUE),1)
	DEF += -DWITH_KQUEUE
endif

ifeq ($(WITH_POLL),1)
	DEF += -DWITH_POLL
endif

ifeq ($(WITH_SELECT),1)
	DEF += -DWITH_SELECT
endif

ifeq ($(WITH_CURL),1)
	DEF += -DWITH_CURL
endif

ifeq ($(WITH_IPC),1)
	DEF += -DWITH_IPC
endif

%:: %,v

%:: RCS/%,v

%:: RCS/%

%:: SCCS/%

%:: %.c

%:: %.pl

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

%:: %.h

endif

