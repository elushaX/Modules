project(Containers)

### ---------------------- Static Library  ---------------------  ###
file(GLOB SOURCES "./private/*.cpp")
file(GLOB HEADERS "./public/*.hpp")
add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PUBLIC ./public/)
target_include_directories(${PROJECT_NAME} PUBLIC ./private/)
target_link_libraries(${PROJECT_NAME} PUBLIC Modules)

### -------------------------- Tests  --------------------------  ###
file(GLOB TEST_SOURCES
        ./tests/Buffer2DTest.cpp
        ./tests/BufferTest.cpp
        ./tests/IntervalTreeTests.cpp
        ./tests/ListTest.cpp
        ./tests/MapTest.cpp
        ./tests/TreeTest.cpp
        ./tests/Tests.cpp
)

add_executable(Tests${PROJECT_NAME} ${TEST_SOURCES})
target_link_libraries(Tests${PROJECT_NAME} ${PROJECT_NAME} UnitTest++)
add_test(NAME Tests${PROJECT_NAME} COMMAND Tests${PROJECT_NAME})


add_executable(AVLTreeSpeedTest ./tests/AVLTreeProfiling.cpp)
target_link_libraries(AVLTreeSpeedTest ${PROJECT_NAME})