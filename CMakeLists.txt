cmake_minimum_required (VERSION 2.6)
project (LambdaCore)

# Sources
set(BITLINKER_COMMONS_SOURCES
    #Common
    BitlinkerCommons/Common/StringUtils.h
    BitlinkerCommons/Common/StringUtils.cpp
    
    #Streams
    BitlinkerCommons/Streams/IOStream.h
    BitlinkerCommons/Streams/FileStream.h
    BitlinkerCommons/Streams/FileStream.cpp
    BitlinkerCommons/Streams/MemoryStream.h
    BitlinkerCommons/Streams/MemoryStream.cpp

    #Logger
    BitlinkerCommons/Logger/Log.h
    BitlinkerCommons/Logger/Log.cpp
    
    #Render
    BitlinkerCommons/Render/Init.cpp
    BitlinkerCommons/Render/Init.h
    BitlinkerCommons/Render/BufferObject.h
    BitlinkerCommons/Render/Camera.h
    BitlinkerCommons/Render/Camera.cpp
    BitlinkerCommons/Render/Frustum.h
    BitlinkerCommons/Render/Frustum.cpp
    BitlinkerCommons/Render/IBindable.h    
    BitlinkerCommons/Render/RenderCommon.h
    BitlinkerCommons/Render/GLContext.h
    BitlinkerCommons/Render/GLContext.cpp
    BitlinkerCommons/Render/ScopeBind.h
    BitlinkerCommons/Render/Shader.h
    BitlinkerCommons/Render/Shader.cpp
    BitlinkerCommons/Render/ShaderProgram.h
    BitlinkerCommons/Render/ShaderProgram.cpp
    BitlinkerCommons/Render/Texture.h
    BitlinkerCommons/Render/Texture.cpp
    BitlinkerCommons/Render/ITextureLoader.h
    BitlinkerCommons/Render/SharedTextureMgr.h
    BitlinkerCommons/Render/SharedTextureMgr.cpp
    BitlinkerCommons/Render/VertexArrayObject.h
    BitlinkerCommons/Render/RenderWindow.h
    BitlinkerCommons/Render/RenderWindow.cpp
    BitlinkerCommons/Render/RenderNode.h
    BitlinkerCommons/Render/RenderNode.cpp
)

set(LAMBDA_CORE_SOURCES
    src/main.cpp
    src/WAD.cpp
    src/WAD.h
    src/BSPMap.cpp
    src/BSPMap.h
    src/BSPRender.cpp
    src/BSPRender.h
    src/MipTexLoader.cpp
    src/MipTexLoader.h
    src/LightmapMgr.cpp
    src/LightmapMgr.h    
    src/BSPShaderProgram.cpp
    src/BSPShaderProgram.h
    src/FreeFlyCameraController.cpp
    src/FreeFlyCameraController.h
)


# Includes
set(INCLUDE_DIRS
	BitlinkerCommons
    src
)
INCLUDE_DIRECTORIES(${INCLUDE_DIRS})

SOURCE_GROUP("BitlinkerCommons" FILES ${BITLINKER_COMMONS_SOURCES})
SOURCE_GROUP("LambdaCore" FILES ${LAMBDA_CORE_SOURCES})


# Platform-specific configs:
ADD_DEFINITIONS(/D _CRT_SECURE_NO_WARNINGS)


# External libs:
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/Modules/")

FIND_PACKAGE (GLFW3 REQUIRED)
FIND_PACKAGE (GLEW REQUIRED)
FIND_PACKAGE (GLM REQUIRED)

INCLUDE_DIRECTORIES (${GLFW3_INCLUDE_DIR} ${GLEW_INCLUDE_DIR} ${GLM_INCLUDE_DIR})

# Link
add_executable(LambdaCore ${BITLINKER_COMMONS_SOURCES} ${LAMBDA_CORE_SOURCES})
target_link_libraries(LambdaCore opengl32.lib ${GLFW3_LIBRARY} ${GLEW_LIBRARY})