find_package(OpenGL REQUIRED)

set(GLEW_VERBOSE true)
set(GLEW_ROOT "C:/msys64/mingw64/include/GL")
find_package(GLEW REQUIRED)

find_package(SDL2 REQUIRED)

find_package(SDL2TTF REQUIRED)

target_include_directories(${PROJECT_NAME}_example PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(${PROJECT_NAME}_example PUBLIC ${OPENGL_INCLUDE_DIRS} ${SDL2_INCLUDE_DIR} ${SDL2TTF_INCLUDE_DIR} ${GLEW_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}_example PUBLIC -mconsole ${OPENGL_LIBRARY} ${SDL2_LIBRARY} ${SDL2TTF_LIBRARY} GLEW::glew )