if ( NOT DEFINED ENV{ROOTSYS} )
  cmessage (FATAL_ERROR "$ROOTSYS is not defined, please set up root first.")
else()
  cmessage(STATUS "Using ROOT installed at $ENV{ROOTSYS}")
  set(CMAKE_ROOTSYS $ENV{ROOTSYS})
endif()

execute_process (COMMAND root-config
  --cflags OUTPUT_VARIABLE ROOT_CXX_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process (COMMAND root-config
  --libdir OUTPUT_VARIABLE ROOT_LIBDIR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process (COMMAND root-config
  --version OUTPUT_VARIABLE ROOT_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process (COMMAND root-config
  --features OUTPUT_VARIABLE ROOT_FEATURES OUTPUT_STRIP_TRAILING_WHITESPACE)

set(ROOT_LD_FLAGS "-L${ROOT_LIBDIR}")

set(ROOT_LIBS Core;Cint;RIO;Net;Hist;Graf;Graf3d;Gpad;Tree;Rint;Postscript;Matrix;Physics;MathCore;Thread;EG;EGPythia6;Geom;Pythia6)

if(USE_GENIE)
  cmessage(STATUS "GENIE requires eve generation libraries")
  set(ROOT_LIBS Eve;EG;TreePlayer;Geom;Ged;Gui;${ROOT_LIBS})
endif()

if(NOT DEFINED USE_MINIMIZER)
  if("${ROOT_FEATURES}" MATCHES "minuit2")
    cmessage(STATUS "ROOT built with MINUIT2 support")
    set(USE_MINIMIZER 1)
  else()
    cmessage(STATUS "ROOT built without MINUIT2 support, minimizer functionality will be disabled.")
    set(USE_MINIMIZER 0)
  endif()
endif()

if("${ROOT_FEATURES}" MATCHES "opengl")
  cmessage(STATUS "ROOT built with OpenGL support")
  set(ROOT_LIBS ${ROOT_LIBS};RGL)
endif()

if(DEFINED NEED_ROOTPYTHIA6 AND NEED_ROOTPYTHIA6)
  set(ROOT_LIBS ${ROOT_LIBS};EGPythia6)
endif()

cmessage ( STATUS "[ROOT]: root-config --version: " ${ROOT_VERSION})
cmessage ( STATUS "[ROOT]: root-config --cflags: " ${ROOT_CXX_FLAGS} )
cmessage ( STATUS "[ROOT]: root-config --libs: " ${ROOT_LD_FLAGS} )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ROOT_CXX_FLAGS}")
