#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux
CND_DLIB_EXT=so
CND_CONF=Debug-static-x86_64
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/src/camera/gvb-camera-opt.o \
	${OBJECTDIR}/src/camera/gvb-camera.o \
	${OBJECTDIR}/src/camera/gvb-opt.o \
	${OBJECTDIR}/src/common/gvb-error.o \
	${OBJECTDIR}/src/compat/compat.o \
	${OBJECTDIR}/src/main.o \
	${OBJECTDIR}/src/network/gvb-client-opt.o \
	${OBJECTDIR}/src/network/gvb-client.o \
	${OBJECTDIR}/src/network/gvb-network-opt.o \
	${OBJECTDIR}/src/network/gvb-network.o \
	${OBJECTDIR}/src/network/gvb-server-opt.o \
	${OBJECTDIR}/src/network/gvb-server.o \
	${OBJECTDIR}/src/parser/h264/gst/gstbitreader.o \
	${OBJECTDIR}/src/parser/h264/gst/gstbytereader.o \
	${OBJECTDIR}/src/parser/h264/gst/gsth264parser.o \
	${OBJECTDIR}/src/parser/h264/gst/gstutils.o \
	${OBJECTDIR}/src/parser/h264/gst/nalutils.o \
	${OBJECTDIR}/src/tests/gvb-client2server.o \
	${OBJECTDIR}/src/tests/gvb-misc.o \
	${OBJECTDIR}/src/tests/gvb-server2client.o \
	${OBJECTDIR}/src/ui/gvb-ui.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L3rdparty/glib-2.0/2.48.1/x86_64/libs -ldl -lresolv -lz -lrt -lm -Wl,-Bstatic -lgio-2.0 -lgmodule-2.0 -lgobject-2.0 -lglib-2.0 -Wl,-Bdynamic `pkg-config --libs ncurses` `pkg-config --libs libffi` `pkg-config --libs libpcre` -lpthread   

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/glavbot

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/glavbot: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	gcc -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/glavbot ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/src/camera/gvb-camera-opt.o: nbproject/Makefile-${CND_CONF}.mk src/camera/gvb-camera-opt.c 
	${MKDIR} -p ${OBJECTDIR}/src/camera
	${RM} "$@.d"
	$(COMPILE.c) -g -I3rdparty/glib-2.0/2.48.1/x86_64/include -Isrc/common -Isrc/camera -Isrc/network -Isrc/ui -Isrc/tests `pkg-config --cflags ncurses` `pkg-config --cflags libffi` `pkg-config --cflags libpcre`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/camera/gvb-camera-opt.o src/camera/gvb-camera-opt.c

${OBJECTDIR}/src/camera/gvb-camera.o: nbproject/Makefile-${CND_CONF}.mk src/camera/gvb-camera.c 
	${MKDIR} -p ${OBJECTDIR}/src/camera
	${RM} "$@.d"
	$(COMPILE.c) -g -I3rdparty/glib-2.0/2.48.1/x86_64/include -Isrc/common -Isrc/camera -Isrc/network -Isrc/ui -Isrc/tests `pkg-config --cflags ncurses` `pkg-config --cflags libffi` `pkg-config --cflags libpcre`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/camera/gvb-camera.o src/camera/gvb-camera.c

${OBJECTDIR}/src/camera/gvb-opt.o: nbproject/Makefile-${CND_CONF}.mk src/camera/gvb-opt.c 
	${MKDIR} -p ${OBJECTDIR}/src/camera
	${RM} "$@.d"
	$(COMPILE.c) -g -I3rdparty/glib-2.0/2.48.1/x86_64/include -Isrc/common -Isrc/camera -Isrc/network -Isrc/ui -Isrc/tests `pkg-config --cflags ncurses` `pkg-config --cflags libffi` `pkg-config --cflags libpcre`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/camera/gvb-opt.o src/camera/gvb-opt.c

${OBJECTDIR}/src/common/gvb-error.o: nbproject/Makefile-${CND_CONF}.mk src/common/gvb-error.c 
	${MKDIR} -p ${OBJECTDIR}/src/common
	${RM} "$@.d"
	$(COMPILE.c) -g -I3rdparty/glib-2.0/2.48.1/x86_64/include -Isrc/common -Isrc/camera -Isrc/network -Isrc/ui -Isrc/tests `pkg-config --cflags ncurses` `pkg-config --cflags libffi` `pkg-config --cflags libpcre`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/common/gvb-error.o src/common/gvb-error.c

${OBJECTDIR}/src/compat/compat.o: nbproject/Makefile-${CND_CONF}.mk src/compat/compat.c 
	${MKDIR} -p ${OBJECTDIR}/src/compat
	${RM} "$@.d"
	$(COMPILE.c) -g -I3rdparty/glib-2.0/2.48.1/x86_64/include -Isrc/common -Isrc/camera -Isrc/network -Isrc/ui -Isrc/tests `pkg-config --cflags ncurses` `pkg-config --cflags libffi` `pkg-config --cflags libpcre`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/compat/compat.o src/compat/compat.c

${OBJECTDIR}/src/main.o: nbproject/Makefile-${CND_CONF}.mk src/main.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -g -I3rdparty/glib-2.0/2.48.1/x86_64/include -Isrc/common -Isrc/camera -Isrc/network -Isrc/ui -Isrc/tests `pkg-config --cflags ncurses` `pkg-config --cflags libffi` `pkg-config --cflags libpcre`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/main.o src/main.c

${OBJECTDIR}/src/network/gvb-client-opt.o: nbproject/Makefile-${CND_CONF}.mk src/network/gvb-client-opt.c 
	${MKDIR} -p ${OBJECTDIR}/src/network
	${RM} "$@.d"
	$(COMPILE.c) -g -I3rdparty/glib-2.0/2.48.1/x86_64/include -Isrc/common -Isrc/camera -Isrc/network -Isrc/ui -Isrc/tests `pkg-config --cflags ncurses` `pkg-config --cflags libffi` `pkg-config --cflags libpcre`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/network/gvb-client-opt.o src/network/gvb-client-opt.c

${OBJECTDIR}/src/network/gvb-client.o: nbproject/Makefile-${CND_CONF}.mk src/network/gvb-client.c 
	${MKDIR} -p ${OBJECTDIR}/src/network
	${RM} "$@.d"
	$(COMPILE.c) -g -I3rdparty/glib-2.0/2.48.1/x86_64/include -Isrc/common -Isrc/camera -Isrc/network -Isrc/ui -Isrc/tests `pkg-config --cflags ncurses` `pkg-config --cflags libffi` `pkg-config --cflags libpcre`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/network/gvb-client.o src/network/gvb-client.c

${OBJECTDIR}/src/network/gvb-network-opt.o: nbproject/Makefile-${CND_CONF}.mk src/network/gvb-network-opt.c 
	${MKDIR} -p ${OBJECTDIR}/src/network
	${RM} "$@.d"
	$(COMPILE.c) -g -I3rdparty/glib-2.0/2.48.1/x86_64/include -Isrc/common -Isrc/camera -Isrc/network -Isrc/ui -Isrc/tests `pkg-config --cflags ncurses` `pkg-config --cflags libffi` `pkg-config --cflags libpcre`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/network/gvb-network-opt.o src/network/gvb-network-opt.c

${OBJECTDIR}/src/network/gvb-network.o: nbproject/Makefile-${CND_CONF}.mk src/network/gvb-network.c 
	${MKDIR} -p ${OBJECTDIR}/src/network
	${RM} "$@.d"
	$(COMPILE.c) -g -I3rdparty/glib-2.0/2.48.1/x86_64/include -Isrc/common -Isrc/camera -Isrc/network -Isrc/ui -Isrc/tests `pkg-config --cflags ncurses` `pkg-config --cflags libffi` `pkg-config --cflags libpcre`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/network/gvb-network.o src/network/gvb-network.c

${OBJECTDIR}/src/network/gvb-server-opt.o: nbproject/Makefile-${CND_CONF}.mk src/network/gvb-server-opt.c 
	${MKDIR} -p ${OBJECTDIR}/src/network
	${RM} "$@.d"
	$(COMPILE.c) -g -I3rdparty/glib-2.0/2.48.1/x86_64/include -Isrc/common -Isrc/camera -Isrc/network -Isrc/ui -Isrc/tests `pkg-config --cflags ncurses` `pkg-config --cflags libffi` `pkg-config --cflags libpcre`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/network/gvb-server-opt.o src/network/gvb-server-opt.c

${OBJECTDIR}/src/network/gvb-server.o: nbproject/Makefile-${CND_CONF}.mk src/network/gvb-server.c 
	${MKDIR} -p ${OBJECTDIR}/src/network
	${RM} "$@.d"
	$(COMPILE.c) -g -I3rdparty/glib-2.0/2.48.1/x86_64/include -Isrc/common -Isrc/camera -Isrc/network -Isrc/ui -Isrc/tests `pkg-config --cflags ncurses` `pkg-config --cflags libffi` `pkg-config --cflags libpcre`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/network/gvb-server.o src/network/gvb-server.c

${OBJECTDIR}/src/parser/h264/gst/gstbitreader.o: nbproject/Makefile-${CND_CONF}.mk src/parser/h264/gst/gstbitreader.c 
	${MKDIR} -p ${OBJECTDIR}/src/parser/h264/gst
	${RM} "$@.d"
	$(COMPILE.c) -g -I3rdparty/glib-2.0/2.48.1/x86_64/include -Isrc/common -Isrc/camera -Isrc/network -Isrc/ui -Isrc/tests `pkg-config --cflags ncurses` `pkg-config --cflags libffi` `pkg-config --cflags libpcre`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/parser/h264/gst/gstbitreader.o src/parser/h264/gst/gstbitreader.c

${OBJECTDIR}/src/parser/h264/gst/gstbytereader.o: nbproject/Makefile-${CND_CONF}.mk src/parser/h264/gst/gstbytereader.c 
	${MKDIR} -p ${OBJECTDIR}/src/parser/h264/gst
	${RM} "$@.d"
	$(COMPILE.c) -g -I3rdparty/glib-2.0/2.48.1/x86_64/include -Isrc/common -Isrc/camera -Isrc/network -Isrc/ui -Isrc/tests `pkg-config --cflags ncurses` `pkg-config --cflags libffi` `pkg-config --cflags libpcre`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/parser/h264/gst/gstbytereader.o src/parser/h264/gst/gstbytereader.c

${OBJECTDIR}/src/parser/h264/gst/gsth264parser.o: nbproject/Makefile-${CND_CONF}.mk src/parser/h264/gst/gsth264parser.c 
	${MKDIR} -p ${OBJECTDIR}/src/parser/h264/gst
	${RM} "$@.d"
	$(COMPILE.c) -g -I3rdparty/glib-2.0/2.48.1/x86_64/include -Isrc/common -Isrc/camera -Isrc/network -Isrc/ui -Isrc/tests `pkg-config --cflags ncurses` `pkg-config --cflags libffi` `pkg-config --cflags libpcre`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/parser/h264/gst/gsth264parser.o src/parser/h264/gst/gsth264parser.c

${OBJECTDIR}/src/parser/h264/gst/gstutils.o: nbproject/Makefile-${CND_CONF}.mk src/parser/h264/gst/gstutils.c 
	${MKDIR} -p ${OBJECTDIR}/src/parser/h264/gst
	${RM} "$@.d"
	$(COMPILE.c) -g -I3rdparty/glib-2.0/2.48.1/x86_64/include -Isrc/common -Isrc/camera -Isrc/network -Isrc/ui -Isrc/tests `pkg-config --cflags ncurses` `pkg-config --cflags libffi` `pkg-config --cflags libpcre`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/parser/h264/gst/gstutils.o src/parser/h264/gst/gstutils.c

${OBJECTDIR}/src/parser/h264/gst/nalutils.o: nbproject/Makefile-${CND_CONF}.mk src/parser/h264/gst/nalutils.c 
	${MKDIR} -p ${OBJECTDIR}/src/parser/h264/gst
	${RM} "$@.d"
	$(COMPILE.c) -g -I3rdparty/glib-2.0/2.48.1/x86_64/include -Isrc/common -Isrc/camera -Isrc/network -Isrc/ui -Isrc/tests `pkg-config --cflags ncurses` `pkg-config --cflags libffi` `pkg-config --cflags libpcre`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/parser/h264/gst/nalutils.o src/parser/h264/gst/nalutils.c

${OBJECTDIR}/src/tests/gvb-client2server.o: nbproject/Makefile-${CND_CONF}.mk src/tests/gvb-client2server.c 
	${MKDIR} -p ${OBJECTDIR}/src/tests
	${RM} "$@.d"
	$(COMPILE.c) -g -I3rdparty/glib-2.0/2.48.1/x86_64/include -Isrc/common -Isrc/camera -Isrc/network -Isrc/ui -Isrc/tests `pkg-config --cflags ncurses` `pkg-config --cflags libffi` `pkg-config --cflags libpcre`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/tests/gvb-client2server.o src/tests/gvb-client2server.c

${OBJECTDIR}/src/tests/gvb-misc.o: nbproject/Makefile-${CND_CONF}.mk src/tests/gvb-misc.c 
	${MKDIR} -p ${OBJECTDIR}/src/tests
	${RM} "$@.d"
	$(COMPILE.c) -g -I3rdparty/glib-2.0/2.48.1/x86_64/include -Isrc/common -Isrc/camera -Isrc/network -Isrc/ui -Isrc/tests `pkg-config --cflags ncurses` `pkg-config --cflags libffi` `pkg-config --cflags libpcre`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/tests/gvb-misc.o src/tests/gvb-misc.c

${OBJECTDIR}/src/tests/gvb-server2client.o: nbproject/Makefile-${CND_CONF}.mk src/tests/gvb-server2client.c 
	${MKDIR} -p ${OBJECTDIR}/src/tests
	${RM} "$@.d"
	$(COMPILE.c) -g -I3rdparty/glib-2.0/2.48.1/x86_64/include -Isrc/common -Isrc/camera -Isrc/network -Isrc/ui -Isrc/tests `pkg-config --cflags ncurses` `pkg-config --cflags libffi` `pkg-config --cflags libpcre`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/tests/gvb-server2client.o src/tests/gvb-server2client.c

${OBJECTDIR}/src/ui/gvb-ui.o: nbproject/Makefile-${CND_CONF}.mk src/ui/gvb-ui.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui
	${RM} "$@.d"
	$(COMPILE.c) -g -I3rdparty/glib-2.0/2.48.1/x86_64/include -Isrc/common -Isrc/camera -Isrc/network -Isrc/ui -Isrc/tests `pkg-config --cflags ncurses` `pkg-config --cflags libffi` `pkg-config --cflags libpcre`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/gvb-ui.o src/ui/gvb-ui.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/glavbot

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
