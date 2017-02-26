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
CND_CONF=Release
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
	${OBJECTDIR}/src/common/gvb-error.o \
	${OBJECTDIR}/src/common/gvb-opt.o \
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
	${OBJECTDIR}/src/parser/h264/gst/nalutils.o \
	${OBJECTDIR}/src/tests/gvb-client2server.o \
	${OBJECTDIR}/src/tests/gvb-h264-parser.o \
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
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/glavbot_github

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/glavbot_github: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/glavbot_github ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/src/camera/gvb-camera-opt.o: src/camera/gvb-camera-opt.c nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src/camera
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/camera/gvb-camera-opt.o src/camera/gvb-camera-opt.c

${OBJECTDIR}/src/camera/gvb-camera.o: src/camera/gvb-camera.c nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src/camera
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/camera/gvb-camera.o src/camera/gvb-camera.c

${OBJECTDIR}/src/common/gvb-error.o: src/common/gvb-error.c nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src/common
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/common/gvb-error.o src/common/gvb-error.c

${OBJECTDIR}/src/common/gvb-opt.o: src/common/gvb-opt.c nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src/common
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/common/gvb-opt.o src/common/gvb-opt.c

${OBJECTDIR}/src/compat/compat.o: src/compat/compat.c nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src/compat
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/compat/compat.o src/compat/compat.c

${OBJECTDIR}/src/main.o: src/main.c nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/main.o src/main.c

${OBJECTDIR}/src/network/gvb-client-opt.o: src/network/gvb-client-opt.c nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src/network
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/network/gvb-client-opt.o src/network/gvb-client-opt.c

${OBJECTDIR}/src/network/gvb-client.o: src/network/gvb-client.c nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src/network
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/network/gvb-client.o src/network/gvb-client.c

${OBJECTDIR}/src/network/gvb-network-opt.o: src/network/gvb-network-opt.c nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src/network
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/network/gvb-network-opt.o src/network/gvb-network-opt.c

${OBJECTDIR}/src/network/gvb-network.o: src/network/gvb-network.c nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src/network
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/network/gvb-network.o src/network/gvb-network.c

${OBJECTDIR}/src/network/gvb-server-opt.o: src/network/gvb-server-opt.c nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src/network
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/network/gvb-server-opt.o src/network/gvb-server-opt.c

${OBJECTDIR}/src/network/gvb-server.o: src/network/gvb-server.c nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src/network
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/network/gvb-server.o src/network/gvb-server.c

${OBJECTDIR}/src/parser/h264/gst/gstbitreader.o: src/parser/h264/gst/gstbitreader.c nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src/parser/h264/gst
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/parser/h264/gst/gstbitreader.o src/parser/h264/gst/gstbitreader.c

${OBJECTDIR}/src/parser/h264/gst/gstbytereader.o: src/parser/h264/gst/gstbytereader.c nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src/parser/h264/gst
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/parser/h264/gst/gstbytereader.o src/parser/h264/gst/gstbytereader.c

${OBJECTDIR}/src/parser/h264/gst/gsth264parser.o: src/parser/h264/gst/gsth264parser.c nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src/parser/h264/gst
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/parser/h264/gst/gsth264parser.o src/parser/h264/gst/gsth264parser.c

${OBJECTDIR}/src/parser/h264/gst/nalutils.o: src/parser/h264/gst/nalutils.c nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src/parser/h264/gst
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/parser/h264/gst/nalutils.o src/parser/h264/gst/nalutils.c

${OBJECTDIR}/src/tests/gvb-client2server.o: src/tests/gvb-client2server.c nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src/tests
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/tests/gvb-client2server.o src/tests/gvb-client2server.c

${OBJECTDIR}/src/tests/gvb-h264-parser.o: src/tests/gvb-h264-parser.c nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src/tests
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/tests/gvb-h264-parser.o src/tests/gvb-h264-parser.c

${OBJECTDIR}/src/tests/gvb-misc.o: src/tests/gvb-misc.c nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src/tests
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/tests/gvb-misc.o src/tests/gvb-misc.c

${OBJECTDIR}/src/tests/gvb-server2client.o: src/tests/gvb-server2client.c nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src/tests
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/tests/gvb-server2client.o src/tests/gvb-server2client.c

${OBJECTDIR}/src/ui/gvb-ui.o: src/ui/gvb-ui.c nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} -p ${OBJECTDIR}/src/ui
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ui/gvb-ui.o src/ui/gvb-ui.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
