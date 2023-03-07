find_package(OpenGL REQUIRED)

set(GLEW_VERBOSE true)
set(GLEW_ROOT "C:/mingw_dev_lib/glew-2.1.0")
find_package(GLEW REQUIRED)

set(SDL2_PATH "C:/mingw_dev_lib/SDL2")
find_package(SDL2 REQUIRED)

set(SDL2TTF_PATH "C:/mingw_dev_lib/SDL2_ttf/")
find_package(SDL2TTF REQUIRED)
message(STATUS GLEW_LIBRARIES=${GLEW_LIBRARIES})

target_include_directories(${PROJECT_NAME}_example PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# message(SDL2TTF_LIBRARY="${SDL2TTF_LIBRARY}")
# message(SDL2TTF_INCLUDE="${SDL2TTF_INCLUDE_DIR}")

target_include_directories(${PROJECT_NAME}_example PUBLIC ${OPENGL_INCLUDE_DIRS} ${SDL2_INCLUDE_DIR} ${SDL2TTF_INCLUDE_DIR} ${GLEW_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}_example PUBLIC -mconsole ${OPENGL_LIBRARY} ${SDL2_LIBRARY} ${SDL2TTF_LIBRARY} GLEW::glew )