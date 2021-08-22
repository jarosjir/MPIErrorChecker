# /**
# * @File      Makefile
# * @Author    Jiri Jaros
# *            Faculty of Information Technology
# *            Brno University of Technology 
# * @Email     jarosjir@fit.vutbr.cz
# * @Comments  Linux makefile
# * 
# * @Tool        Version 1.0            
# * @Created     20 August    2021, 15:42
# * @LastModif   22 August    2021, 20:54
# *
# * @copyright Copyright (C) 2021 SC\@FIT Research Group,
# *            Brno University of Technology, Brno, CZ.
# *
# */


################################################################################
#         Set following flags based on your compiler and library paths         #
################################################################################


# Select compiler, GNU is default on Linux
 COMPILER = GNU
#COMPILER = Intel


# Replace tabs by spaces
.RECIPEPREFIX +=

################################ GNU g++ + FFTW ################################
ifeq ($(COMPILER), GNU)
  # Compiler name
  CXX       = mpic++

  # C++ standard
  CPP_STD   = -std=c++17


  # Set compiler flags and header files directories
  CXXFLAGS  = $(CPP_STD)
              

# Dynamic link with runtime paths
  LDFLAGS = $(CPP_STD) 
endif

############################# Intel Compiler + FFTW ############################
ifeq ($(COMPILER), Intel)
  # Compiler name
  CXX       = mpiicpc

  # C++ standard
  CPP_STD   = -std=c++17


  # Set compiler flags and header files directories
  CXXFLAGS  = $(CPP_STD)

  LDFLAGS   = $(CPP_STD) 

endif


################################### Build ######################################
# Target binary name
TARGET       = MPIErrorChecker

# Units to be compiled
DEPENDENCIES = main.o           \
               DistException.o  \
               ErrorChecker.o
               

# Build target
all: $(TARGET)

# Link target
$(TARGET): $(DEPENDENCIES)
  $(CXX) $(LDFLAGS) $(DEPENDENCIES) $(LDLIBS) -o $@

# Compile units
%.o: %.cpp
  $(CXX) $(CXXFLAGS) -o $@ -c $<

# Clean repository
.PHONY: clean
clean:
  rm -f $(DEPENDENCIES) $(TARGET)

run:
   mpirun ./MPIErrorChecker 