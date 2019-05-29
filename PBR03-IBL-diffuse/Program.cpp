#include <windows.h>
#include <cstdio>
#include <cassert>
#include <chrono>
//for GLEW
#include "../GLEW/include/GL/glew.h"
#include <GL/GL.h>
//for math
#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp" // translate, rotate, scale, perspective
#include "../glm/ext.hpp" // perspective, translate, rotate
#include "../glm/gtc/type_ptr.hpp" // value_ptr
//for hdr image
#include "../HDRLoader/hdrloader.h"

#pragma comment (lib, "../../GLEW/glew32.lib")
#pragma comment (lib, "opengl32.lib")

//Global
HDC hDC;
HGLRC openGLRC;
RECT clientRect;

//OpenGL
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
GLint uniformMetallic = -1;
GLint uniformRoughness = -1;
GLint uniformLightPositions = -1;
GLint uniformLightColors = -1;
GLint uniformIrradianceMap = -1;
GLenum err = GL_NO_ERROR;

//misc
int clientWidth;
int clientHeight;
std::chrono::high_resolution_clock::time_point startTime;
BOOL CreateOpenGLRenderContext(HWND hWnd);
BOOL InitGLEW();
BOOL InitOpenGL(HWND hWnd);
void DestroyOpenGL(HWND hWnd);
void Render();
void fnCheckGLError(const char* szFile, int nLine);
#define _CheckGLError_ fnCheckGLError(__FILE__,__LINE__);

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int __stdcall WinMain(__in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in_opt LPSTR lpCmdLine, __in int nShowCmd)
{
    BOOL bResult = FALSE;

    MSG msg = { 0 };
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
    wc.lpszClassName = L"PBR03-IBL-diffuseWindowClass";
    wc.style = CS_OWNDC;
    if (!RegisterClass(&wc))
        return 1;
    HWND hWnd = CreateWindowW(wc.lpszClassName, L"PBR03-IBL-diffuse", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 800, 600, 0, 0, hInstance, 0);

    ShowWindow(hWnd, nShowCmd);
    UpdateWindow(hWnd);

    bResult = CreateOpenGLRenderContext(hWnd);
    if (bResult == FALSE)
    {
        OutputDebugStringA("CreateOpenGLRenderContext failed!\n");
        return 1;
    }
    InitGLEW();

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

    startTime = std::chrono::high_resolution_clock::now();
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
            Render();
        }
    }

    return 0;
}

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
        else
        {
            return 0;
        }
    case WM_SIZE:
        clientWidth = LOWORD(lParam);
        clientHeight = HIWORD(lParam);
        glViewport(0, 0, clientWidth, clientHeight);
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
        return -1;
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
        return -1;
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
        return -1;
    }
    return program;
}

GLuint CreateHDRTexture(const char* imagePath)
{
    //load hdr image file
    HDRLoaderResult result = {0};
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
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return cubemap;
}

#include "PBRShader.hpp"
#include "Sphere.hpp"

glm::vec3 lightPositions[] = {
    glm::vec3(-10.0f,  10.0f, 10.0f),
    glm::vec3( 10.0f,  10.0f, 10.0f),
    glm::vec3(-10.0f, -10.0f, 10.0f),
    glm::vec3( 10.0f, -10.0f, 10.0f),
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

    // material parameters
    GLint uniformAlbedo = glGetUniformLocation(program, "albedo");
    uniformMetallic = glGetUniformLocation(program, "metallic");
    uniformRoughness = glGetUniformLocation(program, "roughness");
    GLint uniformAo = glGetUniformLocation(program, "ao");

    // lights
    uniformLightPositions = glGetUniformLocation(program, "lightPositions");
    uniformLightColors = glGetUniformLocation(program, "lightColors");

    GLint uniformCamPos = glGetUniformLocation(program, "camPos");

    // environment map
    uniformIrradianceMap = glGetUniformLocation(program, "irradianceMap");

    _CheckGLError_

    glUseProgram(program);

    projection = glm::perspective(glm::radians(45.0f), (float)clientWidth / (float)clientHeight, 0.1f, 100.0f);
    camPos = glm::vec3(6.5f, 6.5f, 6.5f);
    view = glm::lookAt(camPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    //static projection and view
    glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(view));

    //static albedo and AO
    glUniform3f(uniformAlbedo, 0.5f, 0.0f, 0.0f);
    glUniform1f(uniformAo, 1.0f);

    //static camera position
    glUniform3f(uniformCamPos, camPos.x, camPos.y, camPos.z);
    glUseProgram(0);

    _CheckGLError_

    //set up vertex and index buffer
    glGenBuffers(1, &vertexBuf);
    assert(vertexBuf != 0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), (GLvoid*)vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &indexBuf);
    assert(indexBuf != 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), (GLvoid*)indices, GL_STATIC_DRAW);

    //set up attribute for positon, texcoord and normal
    glVertexAttribPointer(attributePos,      3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
    glVertexAttribPointer(attributeTexCoord, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glVertexAttribPointer(attributeNormal,   3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    //enable attribute arrays
    glEnableVertexAttribArray(attributePos);
    glEnableVertexAttribArray(attributeTexCoord);
    glEnableVertexAttribArray(attributeNormal);

    _CheckGLError_

    //other settings
    glViewport(0, 0, GLsizei(clientWidth), GLsizei(clientHeight));
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

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
//test only: render equirectangular map on a unit cube
void RenderEquirectangularMapToCube()
{
    //create program object
    if(cubeProgram == 0)
    {
        cubeProgram = CreateShaderProgram(cubeVertexShader, cubeFragmentShader);
        cube_uniformProjection = glGetUniformLocation(cubeProgram, "projection");
        cube_uniformView = glGetUniformLocation(cubeProgram, "view");
        cube_uniformEquirectangularMap = glGetUniformLocation(cubeProgram, "equirectangularMap");
    }

    //create vertex array object
    if(cube_vertexArray == 0)
    {
        glGenVertexArrays(1, &cube_vertexArray);

        glGenBuffers(1, &cube_vertexBuf);
        assert(cube_vertexBuf != 0);
        glBindBuffer(GL_ARRAY_BUFFER, cube_vertexBuf);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), (GLvoid*)cube_vertices, GL_STATIC_DRAW);

        glBindVertexArray(cube_vertexArray);
        //set up attribute for positon, texcoord and normal
        glVertexAttribPointer(attributePos,      3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
        glVertexAttribPointer(attributeTexCoord, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glVertexAttribPointer(attributeNormal,   3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
        //enable attribute arrays
        glEnableVertexAttribArray(attributePos);
        glEnableVertexAttribArray(attributeTexCoord);
        glEnableVertexAttribArray(attributeNormal);
    }

    //create texture object
    if(equirectangularMap_texture == 0)
    {
        equirectangularMap_texture = CreateHDRTexture("Brooklyn_Bridge_Planks_2k.hdr");
    }

    //setup
    glViewport(0,0, clientWidth, clientHeight);
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
GLuint environmentFrameBuffer = 0, environmentRenderBuffer = 0, environmentCubeMap = 0;
GLsizei environmentRenderBufferWidth = 512, environmentRenderBufferHeight = 512;
GLuint skyBoxProgram = 0;
GLint skyBoxUniformProjection = -1,
      skyBoxUniformView = -1,
      skyBoxUniformEnvironmentMap = -1;

//render environment equirectangular map to a cube map
void RenderEquirectangularToCubeMap()
{
    //create program object
    if(cubeProgram == 0)
    {
        cubeProgram = CreateShaderProgram(cubeVertexShader, cubeFragmentShader);
        cube_uniformProjection = glGetUniformLocation(cubeProgram, "projection");
        cube_uniformView = glGetUniformLocation(cubeProgram, "view");
        cube_uniformEquirectangularMap = glGetUniformLocation(cubeProgram, "equirectangularMap");
    }

    //create vertex array object
    if(cube_vertexArray == 0)
    {
        glGenVertexArrays(1, &cube_vertexArray);

        glGenBuffers(1, &cube_vertexBuf);
        assert(cube_vertexBuf != 0);
        glBindBuffer(GL_ARRAY_BUFFER, cube_vertexBuf);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), (GLvoid*)cube_vertices, GL_STATIC_DRAW);

        glBindVertexArray(cube_vertexArray);
        //set up attribute for positon, texcoord and normal
        glVertexAttribPointer(attributePos,      3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
        glVertexAttribPointer(attributeTexCoord, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glVertexAttribPointer(attributeNormal,   3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
        //enable attribute arrays
        glEnableVertexAttribArray(attributePos);
        glEnableVertexAttribArray(attributeTexCoord);
        glEnableVertexAttribArray(attributeNormal);
    }

    //create texture object
    if(equirectangularMap_texture == 0)
    {
        equirectangularMap_texture = CreateHDRTexture("Brooklyn_Bridge_Planks_2k.hdr");
    }

    //create cubemap texture
    if(environmentCubeMap == 0)
    {
        environmentCubeMap = CreateCubeMap(environmentRenderBufferWidth, environmentRenderBufferHeight);
    }

    //create framebuffer object
    if(environmentFrameBuffer == 0)
    {
        glGenFramebuffers(1, &environmentFrameBuffer);
        glGenRenderbuffers(1, &environmentRenderBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, environmentFrameBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, environmentRenderBuffer);

        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, environmentRenderBufferWidth, environmentRenderBufferHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, environmentRenderBuffer);
    }

    //setup
    glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 views[6] =
    {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };
    glViewport(0,0,environmentRenderBufferWidth, environmentRenderBufferHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, environmentFrameBuffer);
    glUseProgram(cubeProgram);
    glBindVertexArray(cube_vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vertexBuf);
    glUniformMatrix4fv(cube_uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, equirectangularMap_texture);
    glUniform1i(cube_uniformEquirectangularMap, 0);

    glDisable(GL_CULL_FACE);
    //draw
    for (int i = 0; i < 6; i++)
    {
        glUniformMatrix4fv(cube_uniformView, 1, GL_FALSE, glm::value_ptr(views[i]));
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, environmentCubeMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    glEnable(GL_CULL_FACE);

    //restore
    glUseProgram(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//render environmentCubeMap as the background skybox to default framebuffer
void RenderEnvironment()
{
    //check cubemap texture
    if(environmentCubeMap == 0)
    {
        RenderEquirectangularToCubeMap();
    }

    //create vertex array object
    if(cube_vertexArray == 0)
    {
        glGenVertexArrays(1, &cube_vertexArray);

        glGenBuffers(1, &cube_vertexBuf);
        assert(cube_vertexBuf != 0);
        glBindBuffer(GL_ARRAY_BUFFER, cube_vertexBuf);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), (GLvoid*)cube_vertices, GL_STATIC_DRAW);

        glBindVertexArray(cube_vertexArray);
        //set up attribute for positon, texcoord and normal
        glVertexAttribPointer(attributePos,      3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
        glVertexAttribPointer(attributeTexCoord, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glVertexAttribPointer(attributeNormal,   3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
        //enable attribute arrays
        glEnableVertexAttribArray(attributePos);
        glEnableVertexAttribArray(attributeTexCoord);
        glEnableVertexAttribArray(attributeNormal);
    }

    //create skybox program object
    if(skyBoxProgram == 0)
    {
        skyBoxProgram = CreateShaderProgram(SkyBoxVertexShader, SkyBoxFragmentShader);
        skyBoxUniformProjection = glGetUniformLocation(skyBoxProgram, "projection");
        skyBoxUniformView = glGetUniformLocation(skyBoxProgram, "view");
        skyBoxUniformEnvironmentMap = glGetUniformLocation(skyBoxProgram, "environmentMap");
    }

    //setup
    glViewport(0,0, clientWidth, clientHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(skyBoxProgram);
    glBindVertexArray(cube_vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vertexBuf);
    glUniformMatrix4fv(skyBoxUniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(skyBoxUniformView, 1, GL_FALSE, glm::value_ptr(view));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, environmentCubeMap);
    glUniform1i(skyBoxUniformEnvironmentMap, 0);

    //draw
    glDisable(GL_CULL_FACE);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glEnable(GL_CULL_FACE);

    //restore
    glUseProgram(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
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
    if (environmentCubeMap == 0)
    {
        RenderEquirectangularToCubeMap();
    }

    //create irradiance cubemap
    if (irradianceCubeMap == 0)
    {
        irradianceCubeMap = CreateCubeMap(32, 32);
    }

    //create irradiance program object
    if(irradianceProgram == 0)
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
    glBindVertexArray(cube_vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vertexBuf);
    glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glUniformMatrix4fv(irradianceUniformProjection, 1, false, glm::value_ptr(projection));
    glUniform1i(irradianceUniformEnvironmentMap, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, environmentCubeMap);

    //draw
    glm::mat4 views[6] =
    {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };
    glDisable(GL_CULL_FACE);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glUniformMatrix4fv(irradianceUniformView, 1, false, glm::value_ptr(views[i]));
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceCubeMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    glEnable(GL_CULL_FACE);

    //restore
    glUseProgram(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//render irradianceCubeMap as the background skybox to default framebuffer
void RenderIrradianceEnvironment()
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
    glDisable(GL_CULL_FACE);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glEnable(GL_CULL_FACE);

    //restore
    glUseProgram(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int nrRows    = 7;
int nrColumns = 7;
float spacing = 1.2;

void Render()
{
    auto now = std::chrono::high_resolution_clock::now();
    auto time_span = now - startTime;
    auto passedTime = time_span.count()/1000;

    if (irradianceCubeMap == 0)
    {
        RenderEnvironmentCubeMapToIrradianceCubeMap();
    }

    glViewport(0,0, clientWidth, clientHeight);
    // clear
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw model
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf);
    glUseProgram(program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceCubeMap);
    glUniform1i(uniformIrradianceMap, 0);

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
    for (int row = 0; row < nrRows; ++row)
    {
        glUniform1f(uniformMetallic, (float)row / (float)nrRows);
        for (int col = 0; col < nrColumns; ++col)
        {
            // we clamp the roughness to 0.025 - 1.0 as perfectly smooth surfaces (roughness of 0.0) tend to look a bit off
            // on direct lighting.
            glUniform1f(uniformRoughness, glm::clamp((float)col / (float)nrColumns, 0.05f, 1.0f));

            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(
                (col - (nrColumns / 2)) * spacing,
                (row - (nrRows / 2)) * spacing,
                0.0f
            ));
            glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));

            glDrawElements(GL_TRIANGLES, sizeof(indices)/3, GL_UNSIGNED_INT, 0);
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glUseProgram(0);
    _CheckGLError_

    //RenderEquirectangularMapToCube();

    RenderEnvironment();

    //RenderIrradianceEnvironment();

    _CheckGLError_

    SwapBuffers(hDC);
}