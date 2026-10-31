include_guard(GLOBAL)

set(MODULES_DESKTOP_STAGING_DIR "${CMAKE_BINARY_DIR}/desktop")
set(MODULES_DESKTOP_TEMPLATE "${CMAKE_CURRENT_LIST_DIR}/desktop/application.desktop.in")
set(MODULES_GENERIC_APP_ID "online.ilushabasic.modules")
set(MODULES_GENERIC_APP_NAME "Modules")
set(MODULES_GENERIC_APP_ICON "${CMAKE_CURRENT_LIST_DIR}/desktop/modules.svg")

if (DEFINED ENV{XDG_DATA_HOME} AND NOT "$ENV{XDG_DATA_HOME}" STREQUAL "")
	set(MODULES_DESKTOP_DATA_HOME_DEFAULT "$ENV{XDG_DATA_HOME}")
else ()
	set(MODULES_DESKTOP_DATA_HOME_DEFAULT "$ENV{HOME}/.local/share")
endif ()
set(MODULES_DESKTOP_DATA_HOME "${MODULES_DESKTOP_DATA_HOME_DEFAULT}" CACHE PATH "Data directory the install-desktop-entries target copies launchers and icons into")

function(modules_icon_theme_path ICON ID OUTPUT)
	get_filename_component(EXTENSION "${ICON}" LAST_EXT)
	if (EXTENSION STREQUAL ".svg")
		set(${OUTPUT} "icons/hicolor/scalable/apps/${ID}.svg" PARENT_SCOPE)
		return()
	endif ()

	execute_process(COMMAND file --brief "${ICON}" OUTPUT_VARIABLE DESCRIPTION)
	if (NOT DESCRIPTION MATCHES "PNG image data, ([0-9]+) x ([0-9]+)")
		message(FATAL_ERROR "Desktop icon ${ICON} must be an SVG or a square PNG")
	endif ()
	set(${OUTPUT} "icons/hicolor/${CMAKE_MATCH_1}x${CMAKE_MATCH_2}/apps/${ID}.png" PARENT_SCOPE)
endfunction()

function(modules_add_desktop_entry TARGET)
	cmake_parse_arguments(ENTRY "" "ID;NAME;COMMENT;ICON" "CATEGORIES;ARGUMENTS" ${ARGN})
	if (ENTRY_UNPARSED_ARGUMENTS)
		message(FATAL_ERROR "modules_add_desktop_entry(${TARGET}) got unexpected ${ENTRY_UNPARSED_ARGUMENTS}")
	endif ()
	foreach(REQUIRED ID NAME ICON)
		if (NOT ENTRY_${REQUIRED})
			message(FATAL_ERROR "modules_add_desktop_entry(${TARGET}) needs ${REQUIRED}")
		endif ()
	endforeach()

	target_compile_definitions(${TARGET} PRIVATE MODULES_APP_ID="${ENTRY_ID}" MODULES_APP_NAME="${ENTRY_NAME}")

	set(DESKTOP_ID "${ENTRY_ID}")
	set(DESKTOP_NAME "${ENTRY_NAME}")
	set(DESKTOP_COMMENT "${ENTRY_COMMENT}")
	string(JOIN ";" DESKTOP_CATEGORIES ${ENTRY_CATEGORIES} "")
	string(JOIN " " DESKTOP_EXEC "$<TARGET_FILE:${TARGET}>" ${ENTRY_ARGUMENTS})
	set(DESKTOP_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${TARGET}>")

	configure_file("${MODULES_DESKTOP_TEMPLATE}" "${CMAKE_CURRENT_BINARY_DIR}/${ENTRY_ID}.desktop.in" @ONLY)
	file(GENERATE
		OUTPUT "${MODULES_DESKTOP_STAGING_DIR}/applications/${ENTRY_ID}.desktop"
		INPUT "${CMAKE_CURRENT_BINARY_DIR}/${ENTRY_ID}.desktop.in")

	get_filename_component(ICON_SOURCE "${ENTRY_ICON}" ABSOLUTE)
	modules_icon_theme_path("${ICON_SOURCE}" "${ENTRY_ID}" ICON_PATH)
	configure_file("${ICON_SOURCE}" "${MODULES_DESKTOP_STAGING_DIR}/${ICON_PATH}" COPYONLY)

	add_dependencies(install-desktop-entries ${TARGET})
endfunction()

if (NOT MODULES_BUILD_DESKTOP)
	return()
endif ()

find_program(MODULES_UPDATE_DESKTOP_DATABASE update-desktop-database)
find_program(MODULES_GTK_UPDATE_ICON_CACHE gtk-update-icon-cache)

set(MODULES_DESKTOP_REFRESH_COMMANDS)
if (MODULES_UPDATE_DESKTOP_DATABASE)
	list(APPEND MODULES_DESKTOP_REFRESH_COMMANDS
		COMMAND ${MODULES_UPDATE_DESKTOP_DATABASE} "${MODULES_DESKTOP_DATA_HOME}/applications")
endif ()
if (MODULES_GTK_UPDATE_ICON_CACHE)
	list(APPEND MODULES_DESKTOP_REFRESH_COMMANDS
		COMMAND ${MODULES_GTK_UPDATE_ICON_CACHE} --force --ignore-theme-index "${MODULES_DESKTOP_DATA_HOME}/icons/hicolor")
endif ()

add_custom_target(install-desktop-entries
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${MODULES_DESKTOP_STAGING_DIR}" "${MODULES_DESKTOP_DATA_HOME}"
	${MODULES_DESKTOP_REFRESH_COMMANDS}
	COMMENT "Installing application launchers and icons into ${MODULES_DESKTOP_DATA_HOME}"
	VERBATIM)
