project(Widgets)

### ---------------------- Headless Widget Library  ---------------------  ###
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "./private/core/*.cpp" "./private/layouts/*.cpp" "./private/widgets/*.cpp" "./private/nodes/*.cpp")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "./public/core/*.hpp" "./public/layouts/*.hpp" "./public/widgets/*.hpp" "./public/nodes/*.hpp")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PUBLIC ./public/core ./public/layouts ./public/widgets ./public/nodes)
target_include_directories(${PROJECT_NAME} PRIVATE ./private/core)
target_link_libraries(${PROJECT_NAME} PUBLIC Math GraphicsInput PRIVATE Md4c)

### ---------------------- Window Application Adapter  ---------------------  ###
if (MODULES_BUILD_DESKTOP OR ANDROID)
	file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "./private/app/*.cpp")
	file(GLOB_RECURSE APP_HEADERS CONFIGURE_DEPENDS "./public/app/*.hpp")

	add_library(${PROJECT_NAME}App STATIC ${APP_SOURCES} ${APP_HEADERS})
	target_include_directories(${PROJECT_NAME}App PUBLIC ./public/app)
	target_link_libraries(${PROJECT_NAME}App PUBLIC ${PROJECT_NAME} Graphics Imgui)
endif ()

### -------------------------- Gallery  --------------------------  ###
if (MODULES_BUILD_TESTS OR MODULES_BUILD_EXAMPLES OR ANDROID)
	add_library(${PROJECT_NAME}Gallery STATIC ./examples/Gallery.cpp ./examples/Gallery.hpp ./examples/ThemeEditor.cpp ./examples/ThemeEditor.hpp)
	file(COPY "./examples/rsc/markdown" DESTINATION "${CMAKE_BINARY_DIR}/Graphics/rsc/")
	target_include_directories(${PROJECT_NAME}Gallery PUBLIC ./examples)
	target_link_libraries(${PROJECT_NAME}Gallery PUBLIC ${PROJECT_NAME})

	if (MODULES_BUILD_DESKTOP AND MODULES_BUILD_EXAMPLES)
		add_executable(${PROJECT_NAME}Example ./examples/GalleryApplication.cpp)
		target_link_libraries(${PROJECT_NAME}Example ${PROJECT_NAME}Gallery ${PROJECT_NAME}App)
		modules_add_desktop_entry(${PROJECT_NAME}Example
			ID ${MODULES_GENERIC_APP_ID}
			NAME ${MODULES_GENERIC_APP_NAME}
			COMMENT "Widget gallery of the Modules UI library"
			ICON ${MODULES_GENERIC_APP_ICON}
			CATEGORIES Development)
		file(COPY "../Graphics/rsc" DESTINATION "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/")
		file(COPY "./examples/rsc/markdown" DESTINATION "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/rsc/")
	endif ()

	if (ANDROID)
		add_library(widgetGallery SHARED ./examples/GalleryAndroid.cpp)
		target_link_libraries(widgetGallery PRIVATE ${PROJECT_NAME}Gallery ${PROJECT_NAME}App)
		target_link_options(widgetGallery PRIVATE "-Wl,-u,ANativeActivity_onCreate")
	endif ()
endif ()

### -------------------------- Tests  --------------------------  ###
if (MODULES_BUILD_TESTS)
	file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "./tests/*.cpp" "./tests/support/*.cpp")
	add_executable(Test${PROJECT_NAME} ${TEST_SOURCES})
	target_include_directories(Test${PROJECT_NAME} PRIVATE ./tests/support)
	target_link_libraries(Test${PROJECT_NAME} ${PROJECT_NAME} ${PROJECT_NAME}Gallery UnitTest++)
	add_test(NAME Test${PROJECT_NAME} COMMAND Test${PROJECT_NAME})

	if (MODULES_BUILD_DESKTOP)
		add_executable(Test${PROJECT_NAME}App ./tests/app/DebugInspectorTests.cpp ./tests/app/DebugToolsTests.cpp ./tests/app/InputTranslatorTests.cpp ./tests/Main.cpp ./tests/support/UiDriver.cpp ./tests/support/RecordingPainter.cpp)
		target_include_directories(Test${PROJECT_NAME}App PRIVATE ./tests/support)
		target_link_libraries(Test${PROJECT_NAME}App ${PROJECT_NAME}App UnitTest++)
		add_test(NAME Test${PROJECT_NAME}App COMMAND Test${PROJECT_NAME}App)
	endif ()
endif ()
