# This script controls the version string that is passed at compile time.
# To override the version, use cmake -DRELEASE_STRING="you_version"
# If RELEASE_STRING is not explicitly set we attempt to use the latest git
# commit.  If cmake isn't being run from within a git repository we use the
# LATEST_RELEASE variable, which developers should set before a release is
# tagged

if(NOT DEFINED RELEASE_STRING)
	set(LATEST_RELEASE "2017-03-R2")

	execute_process(
		COMMAND git log -n 1 --format=%h
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
		RESULT_VARIABLE GIT_EXIT_ERROR
		ERROR_QUIET
		OUTPUT_VARIABLE GIT_VERSION
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if (GIT_EXIT_ERROR)
		# We're probably not in a git repo
		set(RELEASE_STRING ${LATEST_RELEASE})
	else (GIT_EXIT_ERROR)
		# We're in a git repo
		execute_process(
			COMMAND git status -s --untracked-files=no
			OUTPUT_VARIABLE DIRTY
		)
		if ( NOT "${DIRTY}" STREQUAL "" )
			set(DIRTY_FLAG "*")
		else()
			set(DIRTY_FLAG "")
		endif()
		set(RELEASE_STRING "git-${GIT_VERSION}${DIRTY_FLAG}")
	endif (GIT_EXIT_ERROR)
endif(NOT DEFINED RELEASE_STRING)