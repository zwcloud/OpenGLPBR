//#define SingleModel
#include <windows.h>
#include <cstdio>
#include <cassert>
#include <chrono>
//for GLEW
#define GLEW_STATIC
#include "../GLEW/include/GL/glew.h"
#include <GL/GL.h>
//for math
#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp" // translate, rotate, scale, perspective
#include "../glm/ext.hpp" // perspective, translate, rotate
#include "../glm/gtc/type_ptr.hpp" // value_ptr
//for hdr image
#include "../HDRLoader/hdrloader.h"
#include <stdexcept>
#include "../Shared/textures/Texture.h"

#pragma comment (lib, "glew32s.lib")
#pragma comment (lib, "opengl32.lib")

//Global
HDC hDC = nullptr;
HGLRC openGLRC = nullptr;
RECT clientRect;

//OpenGL
GLuint vao;
GLuint vertexBuf = 0;
GLuint indexBuf = 0;
GLuint vShader = 0;
GLuint pShader = 0;
GLuint program = 0;
GLint attributePos = 0;
GLint attributeTexCoord = 0;
GLint attributeNormal = 0;
GLint uniformModel = -1;
GLint uniformView = -1;
GLint uniformProjection = -1;
GLint uniformCamPos = -1;
GLint uniformMetallic = -1;
GLint uniformRoughness = -1;
GLint uniformLightPositions = -1;
GLint uniformLightColors = -1;
GLint uniformIrradianceMap = -1;
GLint uniformPrefilterMap = -1;
GLint uniformBRDFLUT = -1;
GLenum err = GL_NO_ERROR;

GLuint environmentFrameBuffer = 0, environmentRenderBuffer = 0;
GLsizei environmentRenderBufferWidth = 512, environmentRenderBufferHeight = 512;

const char* hdrImagePath[] = { "Brooklyn_Bridge_Planks_2k.hdr", "Bryant_Park_2k.hdr","Factory_Catwalk_2k.hdr" };
int currentHDRImageIndex = 2;

//input
float mouseX = 0, mouseY = 0;

//misc
int clientWidth;
int clientHeight;
std::chrono::high_resolution_clock::time_point startTime;
BOOL InitGLEW();
BOOL InitOpenGL(HWND hWnd);
void DestroyOpenGL(HWND hWnd);
void SwitchHDRImage();
void fnCheckGLError(const char* szFile, int nLine);
#define _CheckGLError_ fnCheckGLError(__FILE__,__LINE__);

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        DestroyOpenGL(hWnd);
        PostQuitMessage(0);
        break;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            DestroyOpenGL(hWnd);
            PostQuitMessage(0);
            break;
        }
        else if (wParam == VK_RETURN)
        {
            SwitchHDRImage();
            return 0;
        }
        else
        {
            return 0;
        }
	case WM_MOUSEMOVE:
		mouseX = LOWORD(lParam);
		mouseY = HIWORD(lParam);
		if (mouseY < 0)
			mouseY = 0;
		if (mouseY > clientHeight)
			mouseY = clientHeight;
		return 0;
		break;
    case WM_SIZE:
        if(openGLRC != nullptr)
        {
            clientWidth = LOWORD(lParam);
            clientHeight = HIWORD(lParam);
            glViewport(0, 0, clientWidth, clientHeight);
        }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

BOOL InitGLEW()
{
    char buffer[128];
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        sprintf(buffer, "Error: %s\n", (char*)glewGetErrorString(err));
        OutputDebugStringA(buffer);
        return FALSE;
    }
    sprintf(buffer, "Status: Using GLEW %s\n", (char*)glewGetString(GLEW_VERSION));
    OutputDebugStringA(buffer);
    return TRUE;
}

void fnCheckGLError(const char* szFile, int nLine)
{
    GLenum ErrCode = glGetError();
    if (GL_NO_ERROR != ErrCode)
    {
        const char* szErr = "GL_UNKNOWN ERROR";
        switch (ErrCode)
        {
        case GL_INVALID_ENUM:		szErr = "GL_INVALID_ENUM		";		break;
        case GL_INVALID_VALUE:		szErr = "GL_INVALID_VALUE		";		break;
        case GL_INVALID_OPERATION:	szErr = "GL_INVALID_OPERATION	";		break;
        case GL_OUT_OF_MEMORY:		szErr = "GL_OUT_OF_MEMORY		";		break;
        }
        char buffer[512];
        sprintf(buffer, "%s(%d):glError %s\n", szFile, nLine, szErr);
        OutputDebugStringA(buffer);
        DebugBreak();
    }
}

GLuint CreateShaderProgram(const char* vShaderStr, const char* fShaderStr)
{
    GLint compiled;
    //Vertex shader
    vShader = glCreateShader(GL_VERTEX_SHADER);
    if (vShader == 0)
    {
        OutputDebugString(L"glCreateShader Failed!\n");
        return -1;
    }
    glShaderSource(vShader, 1, &vShaderStr, nullptr);
    glCompileShader(vShader);
    glGetShaderiv(vShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        GLint infoLength = 0;
        glGetShaderiv(vShader, GL_INFO_LOG_LENGTH, &infoLength);
        if (infoLength > 1)
        {
            char* infoLog = (char*)malloc(sizeof(char)*infoLength);
            glGetShaderInfoLog(vShader, infoLength, nullptr, infoLog);
            OutputDebugString(L"Error compiling vertex shader: \n");
            OutputDebugStringA(infoLog);
            OutputDebugStringA("\n");
            OutputDebugStringA(vShaderStr);
            free(infoLog);
        }
        glDeleteShader(vShader);
        throw std::runtime_error("Failed to compile the vertex shader.");
    }

    //Fragment shader
    pShader = glCreateShader(GL_FRAGMENT_SHADER);
    if (vShader == 0)
    {
        OutputDebugString(L"glCreateShader Failed!\n");
        return -1;
    }
    glShaderSource(pShader, 1, &fShaderStr, nullptr);
    glCompileShader(pShader);
    glGetShaderiv(pShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        GLint infoLength = 0;
        glGetShaderiv(pShader, GL_INFO_LOG_LENGTH, &infoLength);
        if (infoLength > 1)
        {
            char* infoLog = (char*)malloc(sizeof(char)*infoLength);
            glGetShaderInfoLog(pShader, infoLength, NULL, infoLog);
            OutputDebugString(L"Error compiling fragment shader: \n");
            OutputDebugStringA(infoLog);
            OutputDebugStringA("\n");
            OutputDebugStringA(fShaderStr);
            free(infoLog);
        }
        glDeleteShader(pShader);
        throw std::runtime_error("Failed to compile the fragment shader.");
    }

    //Program
    GLint linked;
    GLuint program = glCreateProgram();
    if (program == 0)
    {
        OutputDebugString(L"glCreateProgram Failed!\n");
        return -1;
    }
    glAttachShader(program, vShader);
    glAttachShader(program, pShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLint infoLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLength);
        if (infoLength > 1)
        {
            char* infoLog = (char*)malloc(sizeof(char) * infoLength);
            glGetProgramInfoLog(program, infoLength, nullptr, infoLog);
            OutputDebugString(L"Error Linking program: \n");
            OutputDebugStringA(infoLog);
            OutputDebugStringA("\n");
            free(infoLog);
        }
        glDeleteProgram(program);
        throw std::runtime_error("Failed to link the shader program.");
    }
    return program;
}

GLuint CreateHDRTexture(const char* imagePath)
{
    //load hdr image file

    HDRLoaderResult result = { 0 };
    if (!HDRLoader::load(imagePath, result))
    {
        char buf[128];
        sprintf(buf, "error: Failed to load HDR image<%s>\n", imagePath);
        OutputDebugStringA(buf);
        return -1;
    }

    float* data = result.cols;
    unsigned int width = result.width;
    unsigned int height = result.height;

    //create hdr texture object
    GLuint texture = -1;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
    delete[] result.cols;

    //sampler settings
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return texture;
}

GLuint CreateCubeMap(GLsizei width, GLsizei height)
{
    GLuint cubemap;
    glGenTextures(1, &cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
    for (int i = 0; i < 6; ++i)
    {
        // note that we store each face with 16 bit floating point values
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
            width, height, 0, GL_RGB, GL_FLOAT, nullptr);
        // texture data will be paint to each face of this cubemap later
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);//enable trilinear filtering
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return cubemap;
}

#include "PBRShader.hpp"
#include "Sphere.hpp"

glm::vec3 lightPositions[] = {
    glm::vec3(-10.0f,  10.0f, 10.0f),
    glm::vec3(10.0f,  10.0f, 10.0f),
    glm::vec3(-10.0f, -10.0f, 10.0f),
    glm::vec3(10.0f, -10.0f, 10.0f),
};
glm::vec3 lightColors[] = {
    glm::vec3(300.0f, 300.0f, 300.0f),
    glm::vec3(300.0f, 300.0f, 300.0f),
    glm::vec3(300.0f, 300.0f, 300.0f),
    glm::vec3(300.0f, 300.0f, 300.0f)
};

glm::mat4 projection;
glm::vec3 camPos;
glm::mat4 view;

glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
glm::mat4 captureViews[6] =
{
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
};

BOOL InitOpenGL(HWND hWnd)
{
    //create program
    program = CreateShaderProgram(vShaderStr, fShaderStr);
    _CheckGLError_

    //attribute
    attributePos = 0;
    attributeTexCoord = 1;
    attributeNormal = 2;
    //mvp
    uniformModel = glGetUniformLocation(program, "model");
    uniformView = glGetUniformLocation(program, "view");
    uniformProjection = glGetUniformLocation(program, "projection");
    //camera position
    uniformCamPos = glGetUniformLocation(program, "camPos");

    // material parameters
    GLint uniformAlbedo = glGetUniformLocation(program, "albedo");
    uniformMetallic = glGetUniformLocation(program, "metallic");
    uniformRoughness = glGetUniformLocation(program, "roughness");
    GLint uniformAo = glGetUniformLocation(program, "ao");

    // lights
    uniformLightPositions = glGetUniformLocation(program, "lightPositions");
    uniformLightColors = glGetUniformLocation(program, "lightColors");

    // textures
    uniformIrradianceMap = glGetUniformLocation(program, "irradianceMap");
    uniformPrefilterMap = glGetUniformLocation(program, "prefilterMap");
    uniformBRDFLUT = glGetUniformLocation(program, "brdfLUT");

    _CheckGLError_

    glUseProgram(program);

    projection = glm::perspective(glm::radians(45.0f), (float)clientWidth / (float)clientHeight, 0.1f, 100.0f);
    camPos = glm::vec3(6.5f, 6.5f, 6.5f);
    view = glm::lookAt(camPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    //static albedo and AO
    glUniform3f(uniformAlbedo, 0.2f, 0.2f, 0.2f);
    glUniform1f(uniformAo, 1.0f);

    glUseProgram(0);

    _CheckGLError_;

    //
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    //set up vertex and index buffer
    glGenBuffers(1, &vertexBuf);
    assert(vertexBuf != 0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), (GLvoid*)vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &indexBuf);
    assert(indexBuf != 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), (GLvoid*)indices, GL_STATIC_DRAW);

    _CheckGLError_

    //set up attribute for positon, texcoord and normal
    glEnableVertexAttribArray(attributePos);
    glVertexAttribPointer(attributePos, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
    glEnableVertexAttribArray(attributeTexCoord);
    glVertexAttribPointer(attributeTexCoord, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(attributeNormal);
    glVertexAttribPointer(attributeNormal, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));

    _CheckGLError_

    //create framebuffer and renderbuffer
    glGenFramebuffers(1, &environmentFrameBuffer);
    glGenRenderbuffers(1, &environmentRenderBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, environmentFrameBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, environmentRenderBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, environmentRenderBuffer);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        char buffer[512];
        sprintf(buffer, "Error: Framebuffer is not complete!\n");
        OutputDebugStringA(buffer);
        DebugBreak();
    }

    //other settings
    glViewport(0, 0, GLsizei(clientWidth), GLsizei(clientHeight));
    glClearColor(1, 1, 1, 1);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    _CheckGLError_

    return TRUE;
}

void DestroyOpenGL(HWND hWnd)
{
    //Destroy OpenGL objects
    glDeleteShader(vShader);
    glDeleteShader(pShader);
    glDeleteProgram(program);

    //Destroy OpenGL Context
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(openGLRC);

    //Release the device context
    ReleaseDC(hWnd, hDC);
}


#include "CubeShader.h"
#include "Cube.h"
GLuint cubeProgram = 0,
cube_vertexArray = 0,
cube_vertexBuf = 0,
cube_indexBuf = 0,
equirectangularMap_texture = 0;
GLint cube_uniformProjection = -1,
cube_uniformView = -1,
cube_uniformEquirectangularMap = -1;

void RenderCube()
{
    //create vertex array object
    if (cube_vertexArray == 0)
    {
        glGenVertexArrays(1, &cube_vertexArray);

        glGenBuffers(1, &cube_vertexBuf);
        assert(cube_vertexBuf != 0);
        glBindBuffer(GL_ARRAY_BUFFER, cube_vertexBuf);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), (GLvoid*)cube_vertices, GL_STATIC_DRAW);

        glBindVertexArray(cube_vertexArray);
        //set up attribute for positon, texcoord and normal
        glVertexAttribPointer(attributePos, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
        glVertexAttribPointer(attributeTexCoord, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glVertexAttribPointer(attributeNormal, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
        //enable attribute arrays
        glEnableVertexAttribArray(attributePos);
        glEnableVertexAttribArray(attributeTexCoord);
        glEnableVertexAttribArray(attributeNormal);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    glBindVertexArray(cube_vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vertexBuf);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

GLuint quadVAO, quadVBO;
void RenderQuad()
{
    if(quadVAO == 0)
    {
        // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        float quadVertices[] = {
            // positions   // texCoords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f
        };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    }

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

//test only: render equirectangular map on a unit cube
void RenderEquirectangularMapToCube()
{
    //create program object
    if (cubeProgram == 0)
    {
        cubeProgram = CreateShaderProgram(cubeVertexShader, equirectangularToCubemapFragmentShader);
        cube_uniformProjection = glGetUniformLocation(cubeProgram, "projection");
        cube_uniformView = glGetUniformLocation(cubeProgram, "view");
        cube_uniformEquirectangularMap = glGetUniformLocation(cubeProgram, "equirectangularMap");
    }

    //create vertex array object
    if (cube_vertexArray == 0)
    {
        glGenVertexArrays(1, &cube_vertexArray);

        glGenBuffers(1, &cube_vertexBuf);
        assert(cube_vertexBuf != 0);
        glBindBuffer(GL_ARRAY_BUFFER, cube_vertexBuf);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), (GLvoid*)cube_vertices, GL_STATIC_DRAW);

        glBindVertexArray(cube_vertexArray);
        //set up attribute for positon, texcoord and normal
        glVertexAttribPointer(attributePos, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
        glVertexAttribPointer(attributeTexCoord, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glVertexAttribPointer(attributeNormal, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
        //enable attribute arrays
        glEnableVertexAttribArray(attributePos);
        glEnableVertexAttribArray(attributeTexCoord);
        glEnableVertexAttribArray(attributeNormal);
    }

    //create texture object
    if (equirectangularMap_texture == 0)
    {
        const char* imagePath = hdrImagePath[currentHDRImageIndex];
        equirectangularMap_texture = CreateHDRTexture(imagePath);
    }

    //setup
    glViewport(0, 0, clientWidth, clientHeight);
    glUseProgram(cubeProgram);
    glBindVertexArray(cube_vertexArray);
    glUniformMatrix4fv(cube_uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(cube_uniformView, 1, GL_FALSE, glm::value_ptr(view));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, equirectangularMap_texture);
    glUniform1i(cube_uniformEquirectangularMap, 0);

    //draw
    glDrawArrays(GL_TRIANGLES, 0, 36);

    //restore
    glUseProgram(0);
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

#include "SkyBoxShader.h"
GLuint environmentCubeMap = 0;
GLuint skyBoxProgram = 0;
GLint skyBoxUniformProjection = -1,
skyBoxUniformView = -1,
skyBoxUniformEnvironmentMap = -1;

//render environment equirectangular map to a cube map
void RenderEquirectangularToCubeMap()
{
    //create program object
    if (cubeProgram == 0)
    {
        cubeProgram = CreateShaderProgram(cubeVertexShader, equirectangularToCubemapFragmentShader);
        cube_uniformProjection = glGetUniformLocation(cubeProgram, "projection");
        cube_uniformView = glGetUniformLocation(cubeProgram, "view");
        cube_uniformEquirectangularMap = glGetUniformLocation(cubeProgram, "equirectangularMap");
    }

    //create texture object
    if (equirectangularMap_texture == 0)
    {
        const char* imagePath = hdrImagePath[currentHDRImageIndex];
        equirectangularMap_texture = CreateHDRTexture(imagePath);
    }

    //create cubemap texture
    if (environmentCubeMap == 0)
    {
        environmentCubeMap = CreateCubeMap(environmentRenderBufferWidth, environmentRenderBufferHeight);
    }

    //setup
    glViewport(0, 0, environmentRenderBufferWidth, environmentRenderBufferHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, environmentFrameBuffer);
    glUseProgram(cubeProgram);
    glUniformMatrix4fv(cube_uniformProjection, 1, GL_FALSE, glm::value_ptr(captureProjection));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, equirectangularMap_texture);
    glUniform1i(cube_uniformEquirectangularMap, 0);

    //draw
    for (int i = 0; i < 6; i++)
    {
        glUniformMatrix4fv(cube_uniformView, 1, GL_FALSE, glm::value_ptr(captureViews[i]));
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            environmentCubeMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        RenderCube();
    }

    //restore
    glUseProgram(0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindTexture(GL_TEXTURE_CUBE_MAP, environmentCubeMap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}

//render environmentCubeMap as the background skybox to default framebuffer
void RenderEnvironment()
{
    if (environmentCubeMap <= 0)
    {
        throw std::runtime_error("environmentCubeMap not ready");
    }

    //create vertex array object
    if (cube_vertexArray == 0)
    {
        glGenVertexArrays(1, &cube_vertexArray);

        glGenBuffers(1, &cube_vertexBuf);
        assert(cube_vertexBuf != 0);
        glBindBuffer(GL_ARRAY_BUFFER, cube_vertexBuf);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), (GLvoid*)cube_vertices, GL_STATIC_DRAW);

        glBindVertexArray(cube_vertexArray);
        //set up attribute for positon, texcoord and normal
        glVertexAttribPointer(attributePos, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
        glVertexAttribPointer(attributeTexCoord, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glVertexAttribPointer(attributeNormal, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
        //enable attribute arrays
        glEnableVertexAttribArray(attributePos);
        glEnableVertexAttribArray(attributeTexCoord);
        glEnableVertexAttribArray(attributeNormal);
    }

    //create skybox program object
    if (skyBoxProgram == 0)
    {
        skyBoxProgram = CreateShaderProgram(SkyBoxVertexShader, SkyBoxFragmentShader);
        skyBoxUniformProjection = glGetUniformLocation(skyBoxProgram, "projection");
        skyBoxUniformView = glGetUniformLocation(skyBoxProgram, "view");
        skyBoxUniformEnvironmentMap = glGetUniformLocation(skyBoxProgram, "environmentMap");
    }

    //setup
    glViewport(0, 0, clientWidth, clientHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(skyBoxProgram);
    glUniformMatrix4fv(skyBoxUniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(skyBoxUniformView, 1, GL_FALSE, glm::value_ptr(view));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, environmentCubeMap);
    glUniform1i(skyBoxUniformEnvironmentMap, 0);

    //draw
    RenderCube();

    //restore
    glUseProgram(0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

#include "IrradianceShader.h"
GLuint irradianceCubeMap = 0, irradianceProgram = 0;
GLint irradianceUniformEnvironmentMap = -1,
irradianceUniformProjection = -1,
irradianceUniformView = -1;
void RenderEnvironmentCubeMapToIrradianceCubeMap()
{
    //check cubemap texture
    if (environmentCubeMap <= 0)
    {
        throw std::runtime_error("environmentCubeMap not ready");
    }

    //create irradiance cubemap
    if (irradianceCubeMap == 0)
    {
        irradianceCubeMap = CreateCubeMap(32, 32);
    }

    //create irradiance program object
    if (irradianceProgram == 0)
    {
        irradianceProgram = CreateShaderProgram(cubeVertexShader, irradianceFragmentShader);
        irradianceUniformEnvironmentMap = glGetUniformLocation(irradianceProgram, "environmentMap");
        irradianceUniformProjection = glGetUniformLocation(irradianceProgram, "projection");
        irradianceUniformView = glGetUniformLocation(irradianceProgram, "view");
    }

    //set up
    glBindFramebuffer(GL_FRAMEBUFFER, environmentFrameBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, environmentRenderBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

    glViewport(0, 0, 32, 32);
    glBindFramebuffer(GL_FRAMEBUFFER, environmentFrameBuffer);
    glUseProgram(irradianceProgram);
    glUniformMatrix4fv(irradianceUniformProjection, 1, false, glm::value_ptr(captureProjection));
    glUniform1i(irradianceUniformEnvironmentMap, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, environmentCubeMap);

    //draw
    for (unsigned int i = 0; i < 6; ++i)
    {
        glUniformMatrix4fv(irradianceUniformView, 1, false, glm::value_ptr(captureViews[i]));
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceCubeMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        RenderCube();
    }

    //restore
    glUseProgram(0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//render irradianceCubeMap as the background skybox to default framebuffer
void RenderIrradianceEnvironment()
{
    if (irradianceCubeMap <= 0)
    {
        throw std::runtime_error("irradianceCubeMap not ready");
    }

    //create vertex array object
    if (cube_vertexArray == 0)
    {
        glGenVertexArrays(1, &cube_vertexArray);

        glGenBuffers(1, &cube_vertexBuf);
        assert(cube_vertexBuf != 0);
        glBindBuffer(GL_ARRAY_BUFFER, cube_vertexBuf);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), (GLvoid*)cube_vertices, GL_STATIC_DRAW);

        glBindVertexArray(cube_vertexArray);
        //set up attribute for position, texcoord and normal
        glVertexAttribPointer(attributePos, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
        glVertexAttribPointer(attributeTexCoord, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glVertexAttribPointer(attributeNormal, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
        //enable attribute arrays
        glEnableVertexAttribArray(attributePos);
        glEnableVertexAttribArray(attributeTexCoord);
        glEnableVertexAttribArray(attributeNormal);
    }

    //create skybox program object
    if (skyBoxProgram == 0)
    {
        skyBoxProgram = CreateShaderProgram(SkyBoxVertexShader, SkyBoxFragmentShader);
        skyBoxUniformProjection = glGetUniformLocation(skyBoxProgram, "projection");
        skyBoxUniformView = glGetUniformLocation(skyBoxProgram, "view");
        skyBoxUniformEnvironmentMap = glGetUniformLocation(skyBoxProgram, "environmentMap");
    }

    //setup
    glViewport(0, 0, clientWidth, clientHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(skyBoxProgram);
    glBindVertexArray(cube_vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vertexBuf);
    glUniformMatrix4fv(skyBoxUniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(skyBoxUniformView, 1, GL_FALSE, glm::value_ptr(view));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceCubeMap);
    glUniform1i(skyBoxUniformEnvironmentMap, 0);

    //draw
    glDrawArrays(GL_TRIANGLES, 0, 36);

    //restore
    glUseProgram(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SwitchHDRImage()
{
    currentHDRImageIndex = (currentHDRImageIndex + 1) % 3;

    //reset
    if (equirectangularMap_texture > 0)
    {
        glDeleteTextures(1, &equirectangularMap_texture);
        equirectangularMap_texture = 0;
    }
    if (environmentCubeMap > 0)
    {
        glDeleteTextures(1, &environmentCubeMap);
        environmentCubeMap = 0;
    }
    if (irradianceCubeMap > 0)
    {
        glDeleteTextures(1, &irradianceCubeMap);
        irradianceCubeMap = 0;
    }
}

#include "PreFilteredEnvironmentShader.h"
GLuint prefilteredCubeMap = 0;
GLuint preFilteredEnvironmentProgram = 0;
GLint preFilteredEnvironmentUniformProjection = -1,
preFilteredEnvironmentUniformView = -1,
preFilteredEnvironmentUniformEnvironmentMap = -1,
preFilteredEnvironmentUniformRoughness = -1;
const uint32_t prefilteredEnvironmentCubeMapSize = 128;
void RenderPrefilteredEnvironmentMapToCubeMap()
{
    //create skybox cubemap that will be rendered onto
    if (environmentCubeMap == 0)
    {
        throw std::runtime_error("environmentCubeMap not ready");
    }

    //create pre-filtered environment cubemap
    glGenTextures(1, &prefilteredCubeMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilteredCubeMap);
    for (int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);//enable trilinear filtering
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    //create shader program
    preFilteredEnvironmentProgram = CreateShaderProgram(PreFilteredEnvironmentVertexShader, PreFilteredEnvironmentFragmentShader);
    preFilteredEnvironmentUniformProjection = glGetUniformLocation(preFilteredEnvironmentProgram, "projection");
    preFilteredEnvironmentUniformView = glGetUniformLocation(preFilteredEnvironmentProgram, "view");
    preFilteredEnvironmentUniformEnvironmentMap = glGetUniformLocation(preFilteredEnvironmentProgram, "environmentMap");
    preFilteredEnvironmentUniformRoughness = glGetUniformLocation(preFilteredEnvironmentProgram, "roughness");

    glUseProgram(preFilteredEnvironmentProgram);
    glUniform1i(preFilteredEnvironmentUniformEnvironmentMap, 0);
    glUniformMatrix4fv(preFilteredEnvironmentUniformProjection, 1, false, glm::value_ptr(captureProjection));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, environmentCubeMap);

    glBindFramebuffer(GL_FRAMEBUFFER, environmentFrameBuffer);
    uint32_t maxMipLevel = 5;
    for (uint32_t mipLevel = 0; mipLevel < maxMipLevel; mipLevel++)
    {
        uint32_t mipSize = prefilteredEnvironmentCubeMapSize * std::pow(0.5, mipLevel);
        glBindRenderbuffer(GL_RENDERBUFFER, environmentRenderBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipSize, mipSize);
        glViewport(0, 0, mipSize, mipSize);
        float roughtness = (float)mipLevel / (float)(maxMipLevel - 1);
        glUniform1f(preFilteredEnvironmentUniformRoughness, roughtness);
        for (uint32_t i = 0; i < 6; i++)
        {
            glUniformMatrix4fv(preFilteredEnvironmentUniformView, 1, GL_FALSE, glm::value_ptr(captureViews[i]));
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilteredCubeMap, mipLevel);
            if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                char buffer[512];
                sprintf(buffer, "Error: Framebuffer is not complete!\n");
                OutputDebugStringA(buffer);
                DebugBreak();
            }
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            RenderCube();
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    _CheckGLError_
}

//test only: render pre-filtered map on a unit cube
namespace _
{
    GLuint program = 0;
    GLint uniformProjection = -1,
        uniformView = -1,
        uniformCubeMap = -1;
}
void RenderPrefilteredMapAsACube()
{
    //check prefiltered environment cubeMap
    if (prefilteredCubeMap == 0)
    {
        throw new std::runtime_error("prefilteredCubeMap is not ready!");
    }

    //create program object
    if (_::program == 0)
    {
        _::program = CreateShaderProgram(cubeVertexShader, cubeFragmentShader);
        _::uniformProjection = glGetUniformLocation(cubeProgram, "projection");
        _::uniformView = glGetUniformLocation(cubeProgram, "view");
        _::uniformCubeMap = glGetUniformLocation(cubeProgram, "cubeMap");
    }

    //setup
    glViewport(0, 0, clientWidth, clientHeight);
    glUseProgram(_::program);
    glUniformMatrix4fv(_::uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(_::uniformView, 1, GL_FALSE, glm::value_ptr(view));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilteredCubeMap);
    glUniform1i(_::uniformCubeMap, 0);

    //draw
    RenderCube();

    //restore
    glUseProgram(0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}


#include "SkyBoxLODShader.h"
GLuint skyBoxLODProgram = 0;
GLint skyBoxLODUniformProjection = -1,
skyBoxLODUniformView = -1,
skyBoxLODUniformEnvironmentMap = -1;
void RenderPrefilteredEnvironmentSkyBox()
{
    if (prefilteredCubeMap == 0)
    {
        throw std::runtime_error("prefilteredCubeMap not ready");
    }

    //create skybox program object
    if (skyBoxLODProgram == 0)
    {
        skyBoxLODProgram = CreateShaderProgram(SkyBoxLODVertexShader, SkyBoxLODFragmentShader);
        skyBoxLODUniformProjection = glGetUniformLocation(skyBoxLODProgram, "projection");
        skyBoxLODUniformView = glGetUniformLocation(skyBoxLODProgram, "view");
        skyBoxLODUniformEnvironmentMap = glGetUniformLocation(skyBoxLODProgram, "environmentMap");
    }

    //setup
    glViewport(0, 0, clientWidth, clientHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(skyBoxLODProgram);
    glUniformMatrix4fv(skyBoxLODUniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(skyBoxLODUniformView, 1, GL_FALSE, glm::value_ptr(view));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilteredCubeMap);

    //draw
    RenderCube();

    //restore
    glUseProgram(0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    _CheckGLError_
}

#include "ConvolutedBRDFShader.h"
GLuint brdfLUTTexture = 0;
GLuint ConvolutedBRDFProgram = 0;
void RenderConvolutedBRDFToTexture()
{
    ConvolutedBRDFProgram = CreateShaderProgram(ConvolutedBRDFVertexShader, ConvolutedBRDFFragmentShader);

    glGenTextures(1, &brdfLUTTexture);

    // pre-allocate enough memory for the LUT texture.
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, environmentFrameBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, environmentRenderBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        OutputDebugStringA("Error: Framebuffer is not complete!\n");
        DebugBreak();
    }
    glViewport(0, 0, 512, 512);
    glUseProgram(ConvolutedBRDFProgram);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    RenderQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderConvolutedBRDFTextureToScreen()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(ConvolutedBRDFProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    RenderQuad();
}

void RenderModel()
{
    int nrRows = 7;
    int nrColumns = 7;
    float spacing = 1.2;

    auto now = std::chrono::high_resolution_clock::now();
    auto time_span = now - startTime;
    auto passedTime = time_span.count() / 1000;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf);
    glUseProgram(program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceCubeMap);
    glUniform1i(uniformIrradianceMap, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilteredCubeMap);
    glUniform1i(uniformPrefilterMap, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    glUniform1i(uniformBRDFLUT, 2);

    glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(view));
    glUniform3f(uniformCamPos, camPos.x, camPos.y, camPos.z);

    // render rows*column number of spheres with varying metallic/roughness values scaled by rows and columns respectively
    // move light positions
    glm::vec3 newLightPositions[] = { glm::vec3(), glm::vec3(), glm::vec3(), glm::vec3() };
    for (unsigned int i = 0; i < sizeof(lightPositions) / sizeof(lightPositions[0]); ++i)
    {
        glm::vec4 p = glm::vec4(lightPositions[i], 0);
        auto rotation = glm::rotate(glm::mat4(1), glm::radians(0.0001f*passedTime), glm::vec3(1.0f, 1.0f, 0.0f));
        p = rotation * p;
        newLightPositions[i] = p;
    }
    glUniform3fv(uniformLightPositions, 4, (const GLfloat*)&newLightPositions[0]);
    glUniform3fv(uniformLightColors, 4, (const GLfloat*)&lightColors[0]);

    glm::mat4 model = glm::mat4(1.0f);

#ifdef SingleModel
	int row = 0, col = 0;
#else
    for (int row = 0; row < nrRows; ++row)
#endif
    {
        glUniform1f(uniformMetallic, (float)row / (float)nrRows);
#ifndef SingleModel
        for (int col = 0; col < nrColumns; ++col)
#endif
        {
            // we clamp the roughness to 0.025 - 1.0 as perfectly smooth surfaces (roughness of 0.0) tend to look a bit off
            // on direct lighting.
            glUniform1f(uniformRoughness, glm::clamp((float)col / (float)nrColumns, 0.05f, 1.0f));

#ifdef SingleModel
			model = glm::mat4(4.0f);
#else
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(
                (col - (nrColumns / 2)) * spacing,
                (row - (nrRows / 2)) * spacing,
                0.0f
            ));
#endif
            glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));

            glDrawElements(GL_TRIANGLES, sizeof(indices) / 3, GL_UNSIGNED_INT, 0);
        }
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glUseProgram(0);
    _CheckGLError_
}

BOOL CreateOpenGLRenderContext(HWND hWnd)
{
    BOOL bResult;
    char buffer[128];
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),  //  size of this pfd
        1,                     // version number
        PFD_DRAW_TO_WINDOW |   // support window
        PFD_SUPPORT_OPENGL |   // support OpenGL
        PFD_DOUBLEBUFFER,      // double buffered
        PFD_TYPE_RGBA,         // RGBA type
        32,                    // 32-bit color depth
        0, 0, 0, 0, 0, 0,      // color bits ignored
        0,                     // no alpha buffer
        0,                     // shift bit ignored
        0,                     // no accumulation buffer
        0, 0, 0, 0,            // accum bits ignored
        24,                    // 24-bit z-buffer
        8,                     // 8-bit stencil buffer
        0,                     // no auxiliary buffer
        PFD_MAIN_PLANE,        // main layer
        0,                     // reserved
        0, 0, 0                // layer masks ignored
    };

    hDC = GetDC(hWnd);
    if (hDC == NULL)
    {
        OutputDebugStringA("Error: GetDC Failed!\n");
        return FALSE;
    }

    int pixelFormatIndex;
    pixelFormatIndex = ChoosePixelFormat(hDC, &pfd);
    if (pixelFormatIndex == 0)
    {
        sprintf(buffer, "Error %d: ChoosePixelFormat Failed!\n", GetLastError());
        OutputDebugStringA(buffer);
        return FALSE;
    }
    bResult = SetPixelFormat(hDC, pixelFormatIndex, &pfd);
    if (bResult == FALSE)
    {
        OutputDebugStringA("SetPixelFormat Failed!\n");
        return FALSE;
    }

    openGLRC = wglCreateContext(hDC);
    if (openGLRC == NULL)
    {
        sprintf(buffer, "Error %d: wglCreateContext Failed!\n", GetLastError());
        OutputDebugStringA(buffer);
        return FALSE;
    }
    bResult = wglMakeCurrent(hDC, openGLRC);
    if (bResult == FALSE)
    {
        sprintf(buffer, "Error %d: wglMakeCurrent Failed!\n", GetLastError());
        OutputDebugStringA(buffer);
        return FALSE;
    }

    sprintf(buffer, "OpenGL version info: %s\n", (char*)glGetString(GL_VERSION));
    OutputDebugStringA(buffer);
    return TRUE;
}

int __stdcall WinMain(__in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in_opt LPSTR lpCmdLine, __in int nShowCmd)
{
    BOOL bResult = FALSE;

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
    wc.lpszClassName = L"PBR04-IBL-specularWindowClass";
    wc.style = CS_OWNDC;
    if (!RegisterClass(&wc))
        return 1;
    HWND hWnd = CreateWindowW(wc.lpszClassName, L"PBR04-IBL-specular", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 800, 600, 0, 0, hInstance, 0);

    ExtractHDRTextures(hInstance);

    bResult = CreateOpenGLRenderContext(hWnd);
    if (bResult == FALSE)
    {
        OutputDebugStringA("CreateOpenGLRenderContext failed!\n");
        return 1;
    }
    InitGLEW();

    ShowWindow(hWnd, nShowCmd);
    UpdateWindow(hWnd);

    // get client size
    if (!GetClientRect(hWnd, &clientRect))
    {
        OutputDebugString(L"GetClientRect Failed!\n");
        return FALSE;
    }
    clientWidth = clientRect.right - clientRect.left;
    clientHeight = clientRect.bottom - clientRect.top;

    //set up OpenGL
    InitOpenGL(hWnd);

    RenderEquirectangularToCubeMap();
    RenderEnvironmentCubeMapToIrradianceCubeMap();
    RenderPrefilteredEnvironmentMapToCubeMap();
    RenderConvolutedBRDFToTexture();

    startTime = std::chrono::high_resolution_clock::now();
    MSG msg = {};
    while (true)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0)
        {
            if (msg.message == WM_QUIT)
            {
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            //rotate camera around the view point according to mouse position
            auto rotation = glm::quat(glm::vec3(0.005f*(clientHeight*0.5f - mouseY), 0.005f*(clientWidth*0.5f - mouseX), 0.0f));
            camPos = glm::vec3(6.5f, 6.5f, 6.5f);
            glm::vec4 p = glm::vec4(camPos, 0);
            camPos = rotation * p;
            view = glm::lookAt(camPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

            glViewport(0, 0, clientWidth, clientHeight);
            // clear
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // draw model
            RenderModel();

            // draw skybox
            RenderEnvironment();

            //RenderPrefilteredMapAsACube();

            //RenderPrefilteredEnvironmentSkyBox();

            //RenderConvolutedBRDFTextureToScreen();

            _CheckGLError_

            SwapBuffers(hDC);
        }
    }

    return 0;
}