project(Modules)

### ---------------------- Static Library  ---------------------  ###
file(GLOB SOURCES "./private/*.cpp")
add_library(${PROJECT_NAME} STATIC ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ./public/)

### -------------------------- Tests  --------------------------  ###
enable_testing()
file(GLOB SOURCES_TEST "./tests/*.cpp")
add_executable(${PROJECT_NAME}Tests ${SOURCES_TEST})
target_link_libraries(${PROJECT_NAME}Tests ${PROJECT_NAME})
add_test(NAME ${PROJECT_NAME}Tests COMMAND ${PROJECT_NAME}Tests)