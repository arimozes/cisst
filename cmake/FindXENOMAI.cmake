# (C) Copyright 2005-2007 Johns Hopkins University (JHU), All Rights
# Reserved.
#
# --- begin cisst license - do not edit ---
# 
# This software is provided "as is" under an open source license, with
# no warranty.  The complete license can be found in license.txt and
# http://www.cisst.org/cisst/license.txt.
# 
# --- end cisst license ---

if( UNIX )

  # set the search paths
  set( XENOMAI_SEARCH_PATH /usr/local/xenomai /usr/xenomai /usr/include/xenomai)
  
  # find xeno-config.h
  find_path( XENOMAI_DIR
    NAMES include/xeno_config.h xeno_config.h
    PATHS ${XENOMAI_SEARCH_PATH} )

  # did we find xeno_config.h?
  if( XENOMAI_DIR ) 
    MESSAGE(STATUS "xenomai found: \"${XENOMAI_DIR}\"")

    # set the include directory
    if( "${XENOMAI_DIR}" MATCHES "/usr/include/xenomai" )
      # on ubuntu linux, xenomai install is not rooted to a single dir
      set( XENOMAI_INCLUDE_DIR ${XENOMAI_DIR} )
      set( XENOMAI_INCLUDE_POSIX_DIR ${XENOMAI_DIR}/posix )
    else( "${XENOMAI_DIR}" MATCHES "/usr/include/xenomai")
      # elsewhere, xenomai install is packaged
      set( XENOMAI_INCLUDE_DIR ${XENOMAI_DIR}/include )
      set( XENOMAI_INCLUDE_POSIX_DIR ${XENOMAI_DIR}/include/posix )
    endif( "${XENOMAI_DIR}" MATCHES "/usr/include/xenomai")
    
    set( XENO_CONFIG ${XENOMAI_DIR}/bin/xeno-config )
    execute_process(COMMAND ${XENO_CONFIG} --version OUTPUT_VARIABLE XENO_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REPLACE "." ";" XENO_VERSION_LIST ${XENO_VERSION} )
    list(GET XENO_VERSION_LIST 0 XENO_VERSION_MAJOR)
    
    if(${XENO_VERSION_MAJOR} EQUAL 3)

      # Run these xeno-config commands to retrieve the necessary compile and link content
      #execute_process(COMMAND ${XENO_CONFIG} --alchemy --cflags OUTPUT_VARIABLE XENO_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
      #execute_process(COMMAND ${XENO_CONFIG} --alchemy --ldflags OUTPUT_VARIABLE XENO_LDFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
      #MESSAGE(STATUS "XENO_CFLAGS: \"${XENO_CFLAGS}\"")
      #MESSAGE(STATUS "XENO_LDFLAGS: \"${XENO_LDFLAGS}\"")

      # add compile/preprocess options as indicated by xeno-config
      set( XENOMAI_DEFINITIONS "-D_GNU_SOURCE -D_REENTRANT -fasynchronous-unwind-tables -D__COBALT__" )

      # add directories for xenomai include files
      set( CISST_XENOMAI_INCLUDE_DIRECTORIES ${XENOMAI_INCLUDE_DIR}/cobalt ${XENOMAI_INCLUDE_DIR} ${XENOMAI_INCLUDE_DIR}/alchemy )

      # find the xenomai libraries as indicated by xeno-config
      find_library( XENOMAI_LIBRARY_ALCHEMY     alchemy     ${XENOMAI_DIR}/lib )
      find_library( XENOMAI_LIBRARY_COPPERPLATE copperplate ${XENOMAI_DIR}/lib )
      find_library( XENOMAI_LIBRARY_COBALT      cobalt      ${XENOMAI_DIR}/lib )
      find_library( XENOMAI_LIBRARY_MODECHK     modechk     ${XENOMAI_DIR}/lib )
      set (CISST_XENOMAI_LIBRARIES
        ${XENOMAI_LIBRARY_ALCHEMY}
        ${XENOMAI_LIBRARY_COPPERPLATE}
        ${XENOMAI_LIBRARY_COBALT}
        ${XENOMAI_LIBRARY_MODECHK})

      # additional linker content as indicated by xeno-config
      # Note: using these in hardcoded form requires xenomai be present in /usr/xenomai
      set( XENOMAI_LDFLAGS_ADDITIONAL "-Wl,--no-as-needed")
      set( XENOMAI_LDFLAGS_ADDITIONAL "${XENOMAI_LDFLAGS_ADDITIONAL} -Wl,@/usr/xenomai/lib/modechk.wrappers")
      set( XENOMAI_LDFLAGS_ADDITIONAL "${XENOMAI_LDFLAGS_ADDITIONAL} /usr/xenomai/lib/xenomai/bootstrap.o")
      set( XENOMAI_LDFLAGS_ADDITIONAL "${XENOMAI_LDFLAGS_ADDITIONAL} -Wl,--wrap=main")
      set( XENOMAI_LDFLAGS_ADDITIONAL "${XENOMAI_LDFLAGS_ADDITIONAL} -Wl,--dynamic-list=/usr/xenomai/lib/dynlist.ld")
      set( XENOMAI_LDFLAGS_ADDITIONAL "${XENOMAI_LDFLAGS_ADDITIONAL} -lpthread -lrt")
      set( CISST_XENOMAI_LIBRARIES "${CISST_XENOMAI_LIBRARIES} ${XENOMAI_LDFLAGS_ADDITIONAL}")

      #MESSAGE(STATUS "XENOMAI_DEFINITIONS: \"${XENOMAI_DEFINITIONS}\"")
      #MESSAGE(STATUS "CISST_XENOMAI_INCLUDE_DIRECTORIES: \"${CISST_XENOMAI_INCLUDE_DIRECTORIES}\"")
      #MESSAGE(STATUS "CISST_XENOMAI_LIBRARIES: \"${CISST_XENOMAI_LIBRARIES}\"")

    else(${XENO_VERSION_MAJOR} EQUAL 3)
    
      # find the xenomai library
      find_library( XENOMAI_LIBRARY_NATIVE  native  ${XENOMAI_DIR}/lib )
      find_library( XENOMAI_LIBRARY_XENOMAI xenomai ${XENOMAI_DIR}/lib )
      
      # add compile/preprocess options
      set(XENOMAI_DEFINITIONS "-D_GNU_SOURCE -D_REENTRANT -Wall -pipe -D__XENO__")

      # add /usr/xenomai/include/posix. This is a *must* since we want to
      # to use xenomai/include/posix/pthread.h
      set (CISST_XENOMAI_INCLUDE_DIRECTORIES ${XENOMAI_INCLUDE_DIR} )#${XENOMAI_INCLUDE_POSIX_DIR})

      # add libnative libxenomai librtdm libpthread_rt
      set (CISST_XENOMAI_LIBRARIES ${XENOMAI_LIBRARY_NATIVE}
#                                ${XENOMAI_LIBRARY_PTHREAD_RT}
                                 ${XENOMAI_LIBRARY_RTDM}
                                 ${XENOMAI_LIBRARY_XENOMAI})

      # add Xenomai wrappers for pthread
      set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${XENOMAI_EXE_LINKER_FLAGS}")
      set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${XENOMAI_EXE_LINKER_FLAGS}")

    endif(${XENO_VERSION_MAJOR} EQUAL 3)

  else( XENOMAI_DIR )
    MESSAGE(STATUS "xenomai NOT found. (${XENOMAI_SEARCH_PATH})")
  endif( XENOMAI_DIR )

endif( UNIX )

