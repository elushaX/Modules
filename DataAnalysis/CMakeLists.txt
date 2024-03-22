project(DataAnalysis)

### ---------------------- Externals  ---------------------  ###
set(BINDINGS_INCLUDE ${EXTERNALS}/glfw/include ${EXTERNALS}/glew/include)
set(BINDINGS_LIBS glfw glew_s Imgui Nanovg)

### ---------------------- Static Library  ---------------------  ###
file(GLOB SOURCES "./private/*.cpp" "./private/*/*.cpp")
file(GLOB HEADERS "./public/*.hpp" "./public/*/*.hpp" "./applications/*.hpp")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ./public/)
target_link_libraries(${PROJECT_NAME} PUBLIC Math Containers)

### -------------------------- Applications  --------------------------  ###
add_executable(NumRecTrain applications/NumRecTraining.cpp)
target_link_libraries(NumRecTrain ${PROJECT_NAME} Connection ImageIO)
file(COPY "applications/rsc" DESTINATION "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/")

add_executable(NumRecApp applications/NumRecApp.cpp)
target_link_libraries(NumRecApp ${PROJECT_NAME} Connection ImageIO)

### -------------------------- Tests  --------------------------  ###
file(GLOB TEST_SOURCES "./tests/*.cpp" "./tests/*/*.cpp")
add_executable(Test${PROJECT_NAME} ${TEST_SOURCES})
target_include_directories(Test${PROJECT_NAME} PUBLIC ./applications/)
target_link_libraries(Test${PROJECT_NAME} ${PROJECT_NAME} UnitTest++)
add_test(NAME Test${PROJECT_NAME} COMMAND Test${PROJECT_NAME})
