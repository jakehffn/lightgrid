# find_package(OpenGL REQUIRED)
# find_package(GLEW REQUIRED)

if(MINGW)
    find_package(SDL2 REQUIRED)
elseif(WIN32)
    target_link_libraries(${PROJECT_NAME}_example PUBLIC shell32.lib)
else()
endif()

target_include_directories(${PROJECT_NAME}_example PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(${PROJECT_NAME}_example PUBLIC ${SDL2_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME}_example PUBLIC -mconsole ${SDL2_LIBRARY})