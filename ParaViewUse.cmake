IF(ParaView_FOUND)

  IF(PARAVIEW_BUILD_SHARED_LIBS)
    ADD_DEFINITIONS(-DPARAVIEW_BUILD_SHARED_LIBS)
  ENDIF(PARAVIEW_BUILD_SHARED_LIBS)

  SET(VTK_DIR ${PARAVIEW_VTK_DIR})
  FIND_PACKAGE(VTK)
  IF(VTK_FOUND)
    INCLUDE_DIRECTORIES(${PARAVIEW_INCLUDE_DIRS})
    LINK_DIRECTORIES(${PARAVIEW_LIBRARY_DIRS})
    INCLUDE(${VTK_USE_FILE})
    MESSAGE(STATUS "Loading ParaView CMake commands")
    INCLUDE(${PARAVIEW_CMAKE_CLIENT_SERVER_EXTENSION})
    MESSAGE(STATUS "Loading ParaView CMake commands - done")
  ENDIF(VTK_FOUND)

  IF(PARAVIEW_BUILD_QT_GUI)
    SET(QT_QMAKE_EXECUTABLE ${PARAVIEW_QT_QMAKE_EXECUTABLE})
    FIND_PACKAGE(Qt4)
    IF(QT4_FOUND)
      INCLUDE(${QT_USE_FILE})
      INCLUDE_DIRECTORIES(${PARAVIEW_GUI_INCLUDE_DIRS})
    ENDIF(QT4_FOUND)
  ENDIF(PARAVIEW_BUILD_QT_GUI)

  INCLUDE(${ParaView_SOURCE_DIR}/CMake/ParaViewPlugins.cmake)

ENDIF(ParaView_FOUND)
