project(vcmiserver)
cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/include ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_HOME_DIRECTORY}/lib)
include_directories(${Boost_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIR})

set(server_SRCS
		StdInc.cpp

		CGameHandler.cpp
		CQuery.cpp
		CVCMIServer.cpp
		NetPacksServer.cpp
)

set(server_HEADERS
		StdInc.h

		CGameHandler.h
		CQuery.h
		CVCMIServer.h
)
if(ANDROID) # android needs client/server to be libraries, not executables, so we can't reuse the build part of this script
	return()
endif()

add_executable(vcmiserver ${server_SRCS} ${server_HEADERS})

target_link_libraries(vcmiserver vcmi ${Boost_LIBRARIES} ${SYSTEM_LIBS})

if(WIN32)
	set_target_properties(vcmiserver PROPERTIES OUTPUT_NAME VCMI_server)
endif()

vcmi_set_output_dir(vcmiserver "")

set_target_properties(vcmiserver PROPERTIES ${PCH_PROPERTIES})
set_target_properties(vcmiserver PROPERTIES XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks @executable_path/")
cotire(vcmiserver)

if (NOT APPLE) # Already inside vcmiclient bundle
	install(TARGETS vcmiserver DESTINATION ${BIN_DIR})
endif()
