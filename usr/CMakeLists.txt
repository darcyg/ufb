CMAKE_MINIMUM_REQUIRED( VERSION 2.6 )

PROJECT( usrfb_sdl )

SET( CMAKE_C_FLAGS "-g -Werror -Wall -Wextra" )

SET( EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin" )
SET( LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/lib" )

INCLUDE(FindPkgConfig)
PKG_SEARCH_MODULE( SDL2 REQUIRED sdl2 )
INCLUDE_DIRECTORIES( ${SDL2_INCLUDE_DIRS} )

INCLUDE_DIRECTORIES( "${PROJECT_SOURCE_DIR}/libufb/inc" )

ADD_SUBDIRECTORY( fbtest )
ADD_SUBDIRECTORY( libufb )
ADD_SUBDIRECTORY( sdlfb )

