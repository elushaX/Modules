project(Taskboard)

### -------------------------- Board Model And View  --------------------------  ###
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "./private/*.cpp")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "./public/*.hpp")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PUBLIC ./public)
target_link_libraries(${PROJECT_NAME} PUBLIC Widgets Picojson)
# The Python MCP task server ships in ./server; the app and tests launch it from this source checkout.
# Skills are versioned here too (./skills); the board's Skills tab browses and edits them in place.
target_compile_definitions(${PROJECT_NAME} PUBLIC TASKBOARD_SERVER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/server")
target_compile_definitions(${PROJECT_NAME} PUBLIC TASKBOARD_SKILLS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/skills")

### -------------------------- Application  --------------------------  ###
if (MODULES_BUILD_DESKTOP)
	add_executable(taskboardApp ./apps/Entry.cpp)
	target_link_libraries(taskboardApp ${PROJECT_NAME} WidgetsApp)
	modules_add_desktop_entry(taskboardApp
		ID online.ilushabasic.taskboard
		NAME Taskboard
		COMMENT "Agent task board"
		ICON ./rsc/desktop/taskboard.svg
		CATEGORIES Development ProjectManagement)
	file(COPY "../Graphics/rsc" DESTINATION "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/")

	add_executable(taskboardTextBench ./bench/TextBench.cpp)
	target_link_libraries(taskboardTextBench ${PROJECT_NAME} WidgetsApp)
endif ()

### -------------------------- Tests  --------------------------  ###
if (MODULES_BUILD_TESTS)
	add_executable(Test${PROJECT_NAME} ./tests/TaskBoardTests.cpp ./tests/TaskServerSyncTests.cpp ../Widgets/tests/Main.cpp ../Widgets/tests/support/UiDriver.cpp ../Widgets/tests/support/RecordingPainter.cpp)
	target_include_directories(Test${PROJECT_NAME} PRIVATE ../Widgets/tests/support)
	target_link_libraries(Test${PROJECT_NAME} ${PROJECT_NAME} UnitTest++)
	add_test(NAME Test${PROJECT_NAME} COMMAND Test${PROJECT_NAME})
endif ()
