project(Allocators)

### ---------------------- Static Library  ---------------------  ###
file(GLOB SOURCES "./private/*.cpp")
file(GLOB HEADERS "./public/*.hpp")
add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PUBLIC ./public/)
target_link_libraries(${PROJECT_NAME} PUBLIC Callstack)

### -------------------------- Tests  --------------------------  ###
enable_testing()
file(GLOB TEST_SOURCES "./tests/*.cpp")
add_executable(Tests${PROJECT_NAME} ${TEST_SOURCES})
target_link_libraries(Tests${PROJECT_NAME} ${PROJECT_NAME} UnitTest++)
add_test(NAME Tests${PROJECT_NAME} COMMAND Tests${PROJECT_NAME})