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
CND_PLATFORM=None-Linux
CND_DLIB_EXT=so
CND_CONF=Debug-dynamic
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
LDLIBSOPTIONS=`pkg-config --libs glib-2.0` `pkg-config --libs gobject-2.0` `pkg-config --libs ncurses` `pkg-config --libs gio-2.0`  

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/glavbot

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/glavbot: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/glavbot ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/src/camera/gvb-camera-opt.o: nbproject/Makefile-${CND_CONF}.mk src/camera/gvb-camera-opt.c 
	${MKDIR} -p ${OBJECTDIR}/src/camera
	${RM} "$@.d"
	$(COMPILE.c) -g -Isrc/camera -Isrc/common -Isrc/network -Isrc/ui `pkg-config --cflags glib-2.0` `pkg-config --cflags gobject-2.0` `pkg-config --cflags ncurses` `pkg-config --cflags gio-2.0`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/camera/gvb-camera-opt.o src/camera/gvb-camera-opt.c

${OBJECTDIR}/src/camera/gvb-camera.o: nbproject/Makefile-${CND_CONF}.mk src/camera/gvb-camera.c 
	${MKDIR} -p ${OBJECTDIR}/src/camera
	${RM} "$@.d"
	$(COMPILE.c) -g -Isrc/camera -Isrc/common -Isrc/network -Isrc/ui `pkg-config --cflags glib-2.0` `pkg-config --cflags gobject-2.0` `pkg-config --cflags ncurses` `pkg-config --cflags gio-2.0`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/camera/gvb-camera.o src/camera/gvb-camera.c

${OBJECTDIR}/src/camera/gvb-opt.o: nbproject/Makefile-${CND_CONF}.mk src/camera/gvb-opt.c 
	${MKDIR} -p ${OBJECTDIR}/src/camera
	${RM} "$@.d"
	$(COMPILE.c) -g -Isrc/camera -Isrc/common -Isrc/network -Isrc/ui `pkg-config --cflags glib-2.0` `pkg-config --cflags gobject-2.0` `pkg-config --cflags ncurses` `pkg-config --cflags gio-2.0`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/camera/gvb-opt.o src/camera/gvb-opt.c

${OBJECTDIR}/src/common/gvb-error.o: nbproject/Makefile-${CND_CONF}.mk src/common/gvb-error.c 
	${MKDIR} -p ${OBJECTDIR}/src/common
	${RM} "$@.d"
	$(COMPILE.c) -g -Isrc/camera -Isrc/common -Isrc/network -Isrc/ui `pkg-config --cflags glib-2.0` `pkg-config --cflags gobject-2.0` `pkg-config --cflags ncurses` `pkg-config --cflags gio-2.0`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/common/gvb-error.o src/common/gvb-error.c

${OBJECTDIR}/src/compat/compat.o: nbproject/Makefile-${CND_CONF}.mk src/compat/compat.c 
	${MKDIR} -p ${OBJECTDIR}/src/compat
	${RM} "$@.d"
	$(COMPILE.c) -g -Isrc/camera -Isrc/common -Isrc/network -Isrc/ui `pkg-config --cflags glib-2.0` `pkg-config --cflags gobject-2.0` `pkg-config --cflags ncurses` `pkg-config --cflags gio-2.0`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/compat/compat.o src/compat/compat.c

${OBJECTDIR}/src/main.o: nbproject/Makefile-${CND_CONF}.mk src/main.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -g -Isrc/camera -Isrc/common -Isrc/network -Isrc/ui `pkg-config --cflags glib-2.0` `pkg-config --cflags gobject-2.0` `pkg-config --cflags ncurses` `pkg-config --cflags gio-2.0`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/main.o src/main.c

${OBJECTDIR}/src/network/gvb-client-opt.o: nbproject/Makefile-${CND_CONF}.mk src/network/gvb-client-opt.c 
	${MKDIR} -p ${OBJECTDIR}/src/network
	${RM} "$@.d"
	$(COMPILE.c) -g -Isrc/camera -Isrc/common -Isrc/network -Isrc/ui `pkg-config --cflags glib-2.0` `pkg-config --cflags gobject-2.0` `pkg-config --cflags ncurses` `pkg-config --cflags gio-2.0`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/network/gvb-client-opt.o src/network/gvb-client-opt.c

${OBJECTDIR}/src/network/gvb-client.o: nbproject/Makefile-${CND_CONF}.mk src/network/gvb-client.c 
	${MKDIR} -p ${OBJECTDIR}/src/network
	${RM} "$@.d"
	$(COMPILE.c) -g -Isrc/camera -Isrc/common -Isrc/network -Isrc/ui `pkg-config --cflags glib-2.0` `pkg-config --cflags gobject-2.0` `pkg-config --cflags ncurses` `pkg-config --cflags gio-2.0`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/network/gvb-client.o src/network/gvb-client.c

${OBJECTDIR}/src/network/gvb-network-opt.o: nbproject/Makefile-${CND_CONF}.mk src/network/gvb-network-opt.c 
	${MKDIR} -p ${OBJECTDIR}/src/network
	${RM} "$@.d"
	$(COMPILE.c) -g -Isrc/camera -Isrc/common -Isrc/network -Isrc/ui `pkg-config --cflags glib-2.0` `pkg-config --cflags gobject-2.0` `pkg-config --cflags ncurses` `pkg-config --cflags gio-2.0`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/network/gvb-network-opt.o src/network/gvb-network-opt.c

${OBJECTDIR}/src/network/gvb-network.o: nbproject/Makefile-${CND_CONF}.mk src/network/gvb-network.c 
	${MKDIR} -p ${OBJECTDIR}/src/network
	${RM} "$@.d"
	$(COMPILE.c) -g -Isrc/camera -Isrc/common -Isrc/network -Isrc/ui `pkg-config --cflags glib-2.0` `pkg-config --cflags gobject-2.0` `pkg-config --cflags ncurses` `pkg-config --cflags gio-2.0`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/network/gvb-network.o src/network/gvb-network.c

${OBJECTDIR}/src/network/gvb-server-opt.o: nbproject/Makefile-${CND_CONF}.mk src/network/gvb-server-opt.c 
	${MKDIR} -p ${OBJECTDIR}/src/network
	${RM} "$@.d"
	$(COMPILE.c) -g -Isrc/camera -Isrc/common -Isrc/network -Isrc/ui `pkg-config --cflags glib-2.0` `pkg-config --cflags gobject-2.0` `pkg-config --cflags ncurses` `pkg-config --cflags gio-2.0`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/network/gvb-server-opt.o src/network/gvb-server-opt.c

${OBJECTDIR}/src/network/gvb-server.o: nbproject/Makefile-${CND_CONF}.mk src/network/gvb-server.c 
	${MKDIR} -p ${OBJECTDIR}/src/network
	${RM} "$@.d"
	$(COMPILE.c) -g -Isrc/camera -Isrc/common -Isrc/network -Isrc/ui `pkg-config --cflags glib-2.0` `pkg-config --cflags gobject-2.0` `pkg-config --cflags ncurses` `pkg-config --cflags gio-2.0`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/network/gvb-server.o src/network/gvb-server.c

${OBJECTDIR}/src/ui/gvb-ui.o: nbproject/Makefile-${CND_CONF}.mk src/ui/gvb-ui.c 
	${MKDIR} -p ${OBJECTDIR}/src/ui
	${RM} "$@.d"
	$(COMPILE.c) -g -Isrc/camera -Isrc/common -Isrc/network -Isrc/ui `pkg-config --cflags glib-2.0` `pkg-config --cflags gobject-2.0` `pkg-config --cflags ncurses` `pkg-config --cflags gio-2.0`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/gvb-ui.o src/ui/gvb-ui.c

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
