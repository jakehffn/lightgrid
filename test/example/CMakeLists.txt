set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/CMake/)

find_package(OpenGL REQUIRED)

set(GLEW_VERBOSE true)
set(GLEW_ROOT "C:/mingw_dev_lib/glew-2.1.0")
find_package(GLEW REQUIRED)

set(SDL2_PATH "C:/mingw_dev_lib/SDL2")
find_package(SDL2 REQUIRED)
message(STATUS GLEW_LIBRARIES=${GLEW_LIBRARIES})

target_include_directories(${PROJECT_NAME}_example PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(${PROJECT_NAME}_example PUBLIC ${OPENGL_INCLUDE_DIRS} ${SDL2_INCLUDE_DIR} ${GLEW_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}_example PUBLIC -mconsole ${OPENGL_LIBRARY} ${SDL2_LIBRARY} GLEW::glew)
