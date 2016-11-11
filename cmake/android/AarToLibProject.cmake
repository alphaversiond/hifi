# 
#  aar-to-libproject.cmake
# 
#  Created by Cristian Duarte on 11/10/16.
#  Copyright 2016 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
# Purpose:
# Converts a .aar library to a library project folder that can be used by android ant projects (without gradle)
# 

macro(aar_to_libproject _aar_file _target_id _output_dir)

if (NOT EXISTS "${_aar_file}")
	message( FATAL_ERROR "${_aar_file} not found")
endif()

if (NOT EXISTS "${_output_dir}")
	file(MAKE_DIRECTORY ${_output_dir})
endif()

set (_command jar xf ${_aar_file})
execute_process(COMMAND ${_command} WORKING_DIRECTORY ${_output_dir})

file(MAKE_DIRECTORY "${_output_dir}/libs")
file(RENAME "${_output_dir}/classes.jar" "${_output_dir}/libs/classes.jar")
file(MAKE_DIRECTORY "${_output_dir}/src")
file(REMOVE "${_output_dir}/R.txt")

set (_command $ENV{ANDROID_HOME}/tools/android update lib-project -p . -t ${_target_id})
execute_process(COMMAND ${_command} WORKING_DIRECTORY ${_output_dir})

file(APPEND "${_output_dir}/project.properties" "\nandroid.library=true")

endmacro(aar_to_libproject)