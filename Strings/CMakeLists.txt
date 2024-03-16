project(Strings)

### ---------------------- Static Library  ---------------------  ###
file(GLOB SOURCES "./private/*.cpp")
file(GLOB HEADERS "./public/*.hpp")
add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PUBLIC ./public/)
target_link_libraries(${PROJECT_NAME} PUBLIC Allocators Containers)

### -------------------------- Tests  --------------------------  ###
enable_testing()
file(GLOB TEST_SOURCES "./tests/*.cpp")
add_executable(Test${PROJECT_NAME} ${TEST_SOURCES})
target_link_libraries(Test${PROJECT_NAME} ${PROJECT_NAME} UnitTest++)
add_test(NAME Test${PROJECT_NAME} COMMAND Test${PROJECT_NAME})