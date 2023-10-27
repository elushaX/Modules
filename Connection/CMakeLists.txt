project(Connection)

set(BINDINGS_INCLUDE ./../Externals/asio/asio/include)

### ---------------------- Static Library  ---------------------  ###
file(GLOB SOURCES "./private/*.cpp" "./private/*/*.cpp")
file(GLOB HEADERS "./public/*.hpp")
add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PUBLIC ./public/)
target_include_directories(${PROJECT_NAME} PRIVATE ${BINDINGS_INCLUDE})
target_link_libraries(${PROJECT_NAME} PUBLIC Strings)

### -------------------------- Tests  --------------------------  ###
enable_testing()
file(GLOB TEST_SOURCES "./tests/*.cpp")
add_executable(${PROJECT_NAME}Tests ${TEST_SOURCES})
target_link_libraries(${PROJECT_NAME}Tests ${PROJECT_NAME} Utils)
add_test(NAME ${PROJECT_NAME}Tests COMMAND ${PROJECT_NAME}Tests)