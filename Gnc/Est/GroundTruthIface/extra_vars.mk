-include $(BUILD_ROOT)/ROS/fprime_ws/src/fprime/mod.mk

EXTRA_INC_DIRS_GncEstGroundTruthIface_DARWIN := $(EXTRA_INC_DIRS_GncEstGroundTruthIface_DARWIN) $(EXTRA_INC_DIRS_DARWIN) $(EXTRA_INC_DIRS)
EXTRA_LIBS_GncEstGroundTruthIface_DARWIN := $(EXTRA_LIBS_GncEstGroundTruthIface_DARWIN) $(EXTRA_LIBS_DARWIN) $(EXTRA_LIBS)

EXTRA_INC_DIRS_GncEstGroundTruthIface_SDFLIGHT := $(EXTRA_INC_DIRS_GncEstGroundTruthIface_SDFLIGHT) $(EXTRA_INC_DIRS_SDFLIGHT) $(EXTRA_INC_DIRS)
EXTRA_LIBS_GncEstGroundTruthIface_SDFLIGHT := $(EXTRA_LIBS_GncEstGroundTruthIface_SDFLIGHT) $(EXTRA_LIBS_SDFLIGHT) $(EXTRA_LIBS)

EXTRA_INC_DIRS_GncEstGroundTruthIface_LINUX := $(EXTRA_INC_DIRS_GncEstGroundTruthIface_LINUX) $(EXTRA_INC_DIRS_LINUX) $(EXTRA_INC_DIRS)
EXTRA_LIBS_GncEstGroundTruthIface_LINUX := $(EXTRA_LIBS_GncEstGroundTruthIface_LINUX) $(EXTRA_LIBS_LINUX) $(EXTRA_LIBS)

EXTRA_INC_DIRS_DARWIN=
EXTRA_LIBS_DARWIN=
EXTRA_INC_DIRS_SDFLIGHT=
EXTRA_LIBS_SDFLIGHT=
EXTRA_INC_DIRS_LINUX=
EXTRA_LIBS_LINUX=
EXTRA_INC_DIRS=
EXTRA_LIBS=
