 ##############################################################################
 #                                                                            #
 # Copyright 2021 Jan Henrik Weinstock                                        #
 #                                                                            #
 # Licensed under the Apache License, Version 2.0 (the "License");            #
 # you may not use this file except in compliance with the License.           #
 # You may obtain a copy of the License at                                    #
 #                                                                            #
 #     http://www.apache.org/licenses/LICENSE-2.0                             #
 #                                                                            #
 # Unless required by applicable law or agreed to in writing, software        #
 # distributed under the License is distributed on an "AS IS" BASIS,          #
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   #
 # See the License for the specific language governing permissions and        #
 # limitations under the License.                                             #
 #                                                                            #
 ##############################################################################

find_path(LIBSLIRP_INCLUDE_DIRS NAMES "libslirp.h"
          HINTS $ENV{LIBSLIRP_HOME}/include/slirp /usr/include/slirp)

find_library(LIBSLIRP_LIBRARIES NAMES "libslirp.so"
             HINTS $ENV{LIBVNC_HOME}/lib /usr/lib /lib)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibSLIRP DEFAULT_MSG
                                  LIBSLIRP_LIBRARIES
                                  LIBSLIRP_INCLUDE_DIRS)

mark_as_advanced(LIBVNC_INCLUDE_DIRS LIBVNC_LIBRARIES)

#message(STATUS "LIBSLIRP_FOUND        " ${LIBSLIRP_FOUND})
#message(STATUS "LIBSLIRP_INCLUDE_DIRS " ${LIBSLIRP_INCLUDE_DIRS})
#message(STATUS "LIBSLIRP_LIBRARIES    " ${LIBSLIRP_LIBRARIES})
