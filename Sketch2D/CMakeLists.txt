project(Sketch2D)

add_subdirectory(src/libs)

if (MODULES_BUILD_DESKTOP OR ANDROID)
	if (ANDROID)
		add_library(sketch2dAndroid SHARED ./src/apps/AndroidEntry.cpp ./src/apps/AndroidUpdater.cpp)
		target_include_directories(sketch2dAndroid PRIVATE ${ANDROID_NDK}/sources/android/native_app_glue)
		target_link_libraries(sketch2dAndroid PRIVATE ${PROJECT_NAME}Application)
		target_link_options(sketch2dAndroid PRIVATE "-Wl,-u,ANativeActivity_onCreate")
	else ()
		add_executable(sketch2dApp ./src/apps/Entry.cpp)
		target_link_libraries(sketch2dApp ${PROJECT_NAME}Application)
		modules_add_desktop_entry(sketch2dApp
			ID online.ilushabasic.sketch2d
			NAME Sketch2D
			COMMENT "Raster sketching and painting"
			ICON ${CMAKE_SOURCE_DIR}/android/app/src/main/res/mipmap-xxxhdpi/ic_launcher.png
			CATEGORIES Graphics 2DGraphics RasterGraphics)
	endif ()
endif ()

if (MODULES_BUILD_DESKTOP)
	### -------------------------- Benchmark --------------------------  ###
	add_library(${PROJECT_NAME}BenchCore STATIC
		./bench/BenchInput.cpp ./bench/BenchReport.cpp
		./bench/BenchInput.hpp ./bench/BenchReport.hpp ./bench/BenchScenario.hpp
	)
	target_include_directories(${PROJECT_NAME}BenchCore PUBLIC ./bench/)
	target_link_libraries(${PROJECT_NAME}BenchCore PUBLIC ${PROJECT_NAME}UI Graphics)

	add_executable(sketch2dBench ./bench/BenchMain.cpp ./bench/BenchRunner.cpp ./bench/BenchScenario.cpp ./bench/BenchRunner.hpp)
	target_link_libraries(sketch2dBench ${PROJECT_NAME}BenchCore ${PROJECT_NAME}Application)
	if (MODULES_BUILD_BENCH_TESTS)
		add_test(NAME sketch2dBenchSmoke COMMAND ${CMAKE_SOURCE_DIR}/Sketch2D/temp/run-headless-wayland.sh $<TARGET_FILE:sketch2dBench> --quick --repeats 2 --filter "size 20" --out ${CMAKE_BINARY_DIR}/Sketch2D/bench-smoke WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME})
	endif ()
endif ()

### -------------------------- Headless UI Tests  --------------------------  ###
if (MODULES_BUILD_TESTS AND MODULES_BUILD_DESKTOP)
	add_executable(Test${PROJECT_NAME} ./tests/Sketch2DUiTests.cpp ./tests/TouchUiTests.cpp ./tests/StudioUiTests.cpp ./tests/SketchLibraryTests.cpp ./tests/UpdateTests.cpp ./tests/BrushTests.cpp ./tests/StabilizerTests.cpp ./tests/CommandTests.cpp ./tests/HistoryTests.cpp ./tests/LayerTests.cpp ./tests/CompositorTests.cpp ./tests/MipTests.cpp ./tests/CanvasTests.cpp ./tests/BenchTests.cpp ./tests/SessionTests.cpp ./tests/StrokePainterTests.cpp ./tests/BrushCursorTests.cpp ./tests/BrushToolsTests.cpp ./tests/ColorCommandTests.cpp ./tests/ColorPanelTests.cpp ./tests/ColorHistoryTests.cpp ./tests/PickerToolTests.cpp ./tests/ViewToolTests.cpp ./tests/BucketToolTests.cpp ./tests/GradientToolTests.cpp ./tests/MoveToolTests.cpp ./tests/TransformToolTests.cpp ./tests/CropToolTests.cpp ./tests/SelectionTests.cpp ./tests/MagicWandTests.cpp ./tests/SelectionEditTests.cpp ./tests/FilterTests.cpp ./tests/ClipboardTests.cpp ./tests/AdjustmentTests.cpp ./tests/LineTests.cpp ./tests/ViewLockTests.cpp ./tests/EngineScenarioTests.cpp ./tests/EngineTests.cpp ./tests/BrushWorkflowTests.cpp ./tests/StrokePathTests.cpp ./tests/DabMaskTests.cpp ./tests/DabMaskBenchTests.cpp ./bench/BenchScenario.cpp ../Widgets/tests/Main.cpp ../Widgets/tests/support/UiDriver.cpp ../Widgets/tests/support/RecordingPainter.cpp)
	target_include_directories(Test${PROJECT_NAME} PRIVATE ../Widgets/tests/support ./tests/support)
	target_link_libraries(Test${PROJECT_NAME} ${PROJECT_NAME}BenchCore UnitTest++)
	add_test(NAME Test${PROJECT_NAME} COMMAND Test${PROJECT_NAME})
endif ()

### -------------------------- Application Integration Tests  --------------------------  ###
if (MODULES_BUILD_TESTS AND MODULES_BUILD_DESKTOP)
	add_executable(Test${PROJECT_NAME}App ./tests/integration/Sketch2DAppTests.cpp ../Widgets/tests/Main.cpp)
	target_link_libraries(Test${PROJECT_NAME}App ${PROJECT_NAME}Application UnitTest++)
	add_test(NAME Test${PROJECT_NAME}App COMMAND ${CMAKE_SOURCE_DIR}/Sketch2D/temp/run-headless-wayland.sh $<TARGET_FILE:Test${PROJECT_NAME}App> WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME})
endif ()

if (MODULES_BUILD_DESKTOP)
	file(COPY "../Graphics/rsc" DESTINATION "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/")
	file(COPY "rsc/icons" DESTINATION "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/rsc/")
endif ()

