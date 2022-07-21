# Simplified existing Makefile
# Contact Sanha Cheong, sanha@stanford.edu for questions

################################################################################
# Magis DIS Makefile
################################################################################
PROJECT_ROOT=./

################################################################################
# Key paths and settings
################################################################################
CFLAGS += -std=c++11
ifeq ($(wildcard ${OPT_INC}),)
CXX = g++ ${CFLAGS}
ODIR  = .obj/build
SDIR  = ./src
MKDIR = mkdir -p
PLATFORM = $(shell uname)
ifeq ($(PLATFORM),Darwin)
OS = mac
endif
endif

OUTPUTNAME = camCharacterizationDataAcquisition 
OUTDIR = ./bin${D}

################################################################################
# Dependencies
################################################################################
# Spinnaker library dependencies
SPINNAKER_LIB = -L/opt/spinnaker/lib -lSpinnaker

################################################################################
# Master inc/lib/obj/dep settings
################################################################################
_OBJ = camCharacterizationDataAcquisition.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))
INC = -I./include
INC += -I/opt/spinnaker/include
LIB += -Wl,-Bdynamic ${SPINNAKER_LIB}

################################################################################
# Rules/recipes
################################################################################
# Final binary
# Final binary
${OUTPUTNAME}: ${OBJ}
	${CXX} -o ${OUTPUTNAME} ${OBJ} ${LIB}
	mv ${OUTPUTNAME} ${OUTDIR}

# Intermediate object files
${OBJ}: ${ODIR}/%.o : ${SDIR}/%.cpp
	@${MKDIR} ${ODIR}
	${CXX} ${CFLAGS} ${INC} -Wall -D LINUX -c $< -o $@

# Clean up intermediate objects
clean_obj:
	rm -f ${OBJ}
	@echo "intermediate objects cleaned up!"

# Clean up everything.
clean: clean_obj
	rm -f ${OUTDIR}/${OUTPUTNAME}
	@echo "all cleaned up!"


