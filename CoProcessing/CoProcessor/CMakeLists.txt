project(CoProcessor)
cmake_minimum_required(VERSION 2.6)
include_directories(${CoProcessor_SOURCE_DIR})
set(BUILD_SHARED_LIBS ON CACHE BOOL "Needed for python scripting.")

#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
IF (ParaView_SOURCE_DIR)
  INCLUDE_DIRECTORIES(
    ${PARAVIEW_INCLUDE_DIRS}
    ${PARAVIEW_GUI_INCLUDE_DIRS}
    ${PARAVIEW_KWSYS_INCLUDE_DIRS}
    ${VTK_INCLUDE_DIR}
  )
ELSE (ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF (ParaView_SOURCE_DIR)

if(VTK_USE_MPI)
  set(COPROCESSOR_USE_MPI 1)
endif(VTK_USE_MPI)

CONFIGURE_FILE("${CoProcessing_SOURCE_DIR}/CoProcessor/CPSystemInformation.h.in"
               "${CoProcessing_BINARY_DIR}/CoProcessor/CPSystemInformation.h"
               @ONLY IMMEDIATE)
include_directories(
  ${CoProcessing_BINARY_DIR}/CoProcessor
)

if(UNIX AND NOT WIN32)
  find_program(CMAKE_UNAME uname /bin /usr/bin /usr/local/bin)
  if(CMAKE_UNAME)
    exec_program(uname ARGS -m OUTPUT_VARIABLE CMAKE_SYSTEM_PROCESSOR)
    set(CMAKE_SYSTEM_PROCESSOR 
      ${CMAKE_SYSTEM_PROCESSOR}
      CACHE
      INTERNAL
      "processor type (i386 and x86_64)")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
      add_definitions(-fPIC)
    endif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
  endif(CMAKE_UNAME)
endif(UNIX AND NOT WIN32)

set(CoProcessor_SRCS
  vtkCPDataDescription.cxx
  vtkCPInputDataDescription.cxx
  vtkCPIntHolder.cxx
  vtkCPPipeline.cxx
  vtkCPProcessModulePythonHelper.cxx
  vtkCPProcessor.cxx
  vtkCPPythonHelper.cxx
  vtkCPPythonScriptPipeline.cxx
)
set_source_files_properties(
  vtkCPPipeline
  ABSTRACT)
set_source_files_properties(
  vtkCPProcessModulePythonHelper
  vtkCPPythonHelper
  WRAP_EXCLUDE)
add_library(vtkCoProcessor ${CoProcessor_SRCS})
target_link_libraries(
  vtkCoProcessor
  vtkCommon
  vtkFiltering
  vtkPVFilters
  vtkPVServerManager
  vtkPVPython
)

# Create Python bindings for the co-processing library
INCLUDE(FindPythonLibs)
INCLUDE(${VTK_CMAKE_DIR}/vtkWrapPython.cmake)

VTK_WRAP_PYTHON3(vtkCoProcessorPython CPPython_SRCS "${CoProcessor_SRCS}")

# Create a shared library containing the python wrappers.  Executables
# can link to this but it is not directly loaded dynamically as a
# module.
ADD_LIBRARY(vtkCoProcessorPythonD ${CPPython_SRCS})
TARGET_LINK_LIBRARIES(vtkCoProcessorPythonD vtkCoProcessor vtkIOPythonD)

# On some UNIX platforms the python library is static and therefore
# should not be linked into the shared library.  Instead the symbols
# are exported from the python executable so that they can be used by
# shared libraries that are linked or loaded.  On Windows and OSX we
# want to link to the python libray to resolve its symbols
# immediately.
IF(WIN32 OR APPLE)
  TARGET_LINK_LIBRARIES (vtkCoProcessorPythonD ${PYTHON_LIBRARIES})
ENDIF(WIN32 OR APPLE)

# Create a python module that can be loaded dynamically.  It links to
# the shared library containing the wrappers for this kit.
PYTHON_ADD_MODULE(vtkCoProcessorPython vtkCoProcessorPythonInit.cxx)
TARGET_LINK_LIBRARIES(vtkCoProcessorPython vtkCoProcessorPythonD)

# Python extension modules on Windows must have the extension ".pyd"
# instead of ".dll" as of Python 2.5.  Older python versions do support
# this suffix.
IF(WIN32 AND NOT CYGWIN)
  SET_TARGET_PROPERTIES(vtkCoProcessorPython PROPERTIES SUFFIX ".pyd")
ENDIF(WIN32 AND NOT CYGWIN)

IF(WIN32 OR APPLE)
  TARGET_LINK_LIBRARIES (vtkCoProcessorPython ${PYTHON_LIBRARIES})
ENDIF(WIN32 OR APPLE)


SET(PYTHON_MODULE_PREFIX ${CMAKE_SHARED_LIBRARY_PREFIX})
PYTHON_WRITE_MODULES_HEADER(${CMAKE_CURRENT_BINARY_DIR}/cppythonmodules.h)
GET_PROPERTY(PY_STATIC_MODULES_LIST GLOBAL PROPERTY PY_STATIC_MODULES_LIST)
IF(PY_STATIC_MODULES_LIST)
  TARGET_LINK_LIBRARIES(vtkCoProcessor ${PY_STATIC_MODULES_LIST})
ENDIF(PY_STATIC_MODULES_LIST)


if(BUILD_TESTING)
  add_subdirectory(Testing)
endif(BUILD_TESTING)

