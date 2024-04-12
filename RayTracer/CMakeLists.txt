project(RayTracer)

### ---------------------- Static Library  ---------------------  ###
file(GLOB SOURCES "./private/*.cpp" "./private/*/*.cpp")
file(GLOB HEADERS "./public/*.hpp" "./public/*/*.hpp" "./applications/*.hpp")
add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PUBLIC ./public/ ../Externals/)
target_link_libraries(${PROJECT_NAME} PUBLIC Math Connection)

### -------------------------- Applications  --------------------------  ###
add_executable(rayt ./applications/Rayt.cpp applications/SceneLoad.cpp applications/Rayt.hpp)
target_link_libraries(rayt ${PROJECT_NAME} Lua ImageIO)
file(COPY "applications/rsc" DESTINATION "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/")

### -------------------------- Tests  --------------------------  ###
file(GLOB TEST_SOURCES "./tests/*.cpp" "./tests/*/*.cpp")
add_executable(Test${PROJECT_NAME} ${TEST_SOURCES})
target_include_directories(Test${PROJECT_NAME} PUBLIC ./applications/)
target_link_libraries(Test${PROJECT_NAME} ${PROJECT_NAME} UnitTest++ ImageIO)
add_test(NAME Test${PROJECT_NAME} COMMAND Test${PROJECT_NAME})
