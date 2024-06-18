project(3DScene)

### ---------------------- Static Library  ---------------------  ###
file(GLOB SOURCES "./private/*.cpp")
file(GLOB HEADERS "./public/*.hpp")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ./public/)
target_include_directories(${PROJECT_NAME} PRIVATE ../Externals/)

target_link_libraries(${PROJECT_NAME} PUBLIC Math)
target_link_libraries(${PROJECT_NAME} PUBLIC ${BINDINGS_LIBS})