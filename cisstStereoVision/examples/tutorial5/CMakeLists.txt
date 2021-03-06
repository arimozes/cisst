#
# (C) Copyright 2014 Johns Hopkins University (JHU), All Rights
# Reserved.
#
# --- begin cisst license - do not edit ---
#
# This software is provided "as is" under an open source license, with
# no warranty.  The complete license can be found in license.txt and
# http://www.cisst.org/cisst/license.txt.
#
# --- end cisst license ---

cmake_minimum_required (VERSION 2.6)

# create a list of libraries needed for this project
set (REQUIRED_CISST_LIBRARIES cisstCommon cisstVector cisstOSAbstraction cisstMultiTask cisstStereoVision)

# find cisst and make sure the required libraries have been compiled
find_package (cisst REQUIRED ${REQUIRED_CISST_LIBRARIES})

if (cisst_FOUND)

  # load cisst configuration
  include (${CISST_USE_FILE})

  add_executable (svlExTutorial5
                  CMyEventHandler2.h
                  CMyEventHandler2.cpp
                  main.cpp
                  )

  set_property (TARGET svlExTutorial5 PROPERTY FOLDER "cisstStereoVision/examples")
  cisst_target_link_libraries (svlExTutorial5 ${REQUIRED_CISST_LIBRARIES})

  # Copy media files to the build directory
#  configure_file (${CMAKE_SOURCE_DIR}/wall.bmp ${CMAKE_BINARY_DIR}/wall.bmp COPYONLY)
#  configure_file (${CMAKE_SOURCE_DIR}/xray.avi ${CMAKE_BINARY_DIR}/xray.avi COPYONLY)

else (cisst_FOUND)
  message ("Information: code in ${CMAKE_CURRENT_SOURCE_DIR} will not be compiled, it requires ${REQUIRED_CISST_LIBRARIES}")
endif (cisst_FOUND)
