cmake_minimum_required(VERSION 3.0)

set(PROJECT_NAME renderer)
project(${PROJECT_NAME})

set(CMAKE_CXX_FLAGS "-std=c++11 -g -O3")

# GLFW
set(GLFW_ROOT_PATH /opt/homebrew/Cellar/glfw/3.4)
set(GLFW_INCLUDE_PATH ${GLFW_ROOT_PATH}/include)
set(GLFW_LIB_PATH ${GLFW_ROOT_PATH}/lib)
set(GLFW_LIBS ${GLFW_LIB_PATH}/libglfw.dylib)


# GLEW
set(GLEW_ROOT_PATH /opt/homebrew/Cellar/glew/2.2.0_1)
set(GLEW_INCLUDE_PATH ${GLEW_ROOT_PATH}/include)
set(GLEW_LIB_PATH ${GLEW_ROOT_PATH}/lib)
set(GLEW_LIBS ${GLEW_LIB_PATH}/libGLEW.2.2.0.dylib)

# FFMPEG
set(FFMPEG_ROOT_PATH /usr/local)
set(FFMPEG_INCLUDE_PATH ${FFMPEG_ROOT_PATH}/include)
set(FFMPEG_LIB_PATH ${FFMPEG_ROOT_PATH}/lib)
set(FFMPEG_LIBS 
	${FFMPEG_LIB_PATH}/libswresample.dylib
	${FFMPEG_LIB_PATH}/libavformat.dylib
	${FFMPEG_LIB_PATH}/libavutil.dylib
	${FFMPEG_LIB_PATH}/libswscale.dylib
)

include_directories(
	${PROJECT_SOURCE_DIR}
	${GLFW_INCLUDE_PATH}
	${GLEW_INCLUDE_PATH}
	${FFMPEG_INCLUDE_PATH}
)

set(SRC "")
aux_source_directory(. SRC)

add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(
    ${PROJECT_NAME}
	${GLFW_LIBS}
	${GLEW_LIBS}
	${FFMPEG_LIBS}
)

if (APPLE)
	target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()
