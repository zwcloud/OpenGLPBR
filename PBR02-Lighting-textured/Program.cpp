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
//for png
#include "../LodePNG/lodepng.h"
#include "../Shared/textures/PBRTextures.h"

#pragma comment (lib, "glew32s.lib")
#pragma comment (lib, "opengl32.lib")

//Global
HINSTANCE globalHInstance;
HDC hDC;
HGLRC openGLRC;
RECT clientRect;

//OpenGL
GLuint vertexBuf = 0;
GLuint indexBuf = 0;
GLuint vShader = 0;
GLuint pShader = 0;
GLuint program = 0;

GLuint albedoTexture = 0;
GLuint normalTexture = 0;
GLuint metallicTexture = 0;
GLuint roughnessTexture = 0;
GLuint aoTexture = 0;

GLint attributePos = 0;
GLint attributeTexCoord = 0;
GLint attributeNormal = 0;

GLint uniformModel = 0;
GLint uniformView = 0;
GLint uniformProjection = 0;
GLint uniformLightPositions = 0;
GLint uniformLightColors = 0;

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

    globalHInstance = hInstance;

    MSG msg = { 0 };
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
    wc.lpszClassName = L"PBR02-Lighting-texturedWindowClass";
    wc.style = CS_OWNDC;
    if (!RegisterClass(&wc))
        return 1;
    HWND hWnd = CreateWindowW(wc.lpszClassName, L"PBR02-Lighting-textured", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 800, 600, 0, 0, hInstance, 0);

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
        throw std::exception();
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

GLuint CreateTexture(const char* imagePath)
{
    GLuint texture = -1;

    //get image png data pointer
    uint8_t* pngData = nullptr;
    uint32_t pngDataByteSize = 0;
    GetPBRTextureData(globalHInstance, imagePath, &pngData, &pngDataByteSize);

    //decode png to raw RGB buffer
    unsigned char* image = nullptr;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned error = lodepng_decode_memory(&image, &width, &height,
        pngData, pngDataByteSize, LodePNGColorType::LCT_RGB, 8);
    if (error)
    {
        char buf[128];
        sprintf(buf, "error %u: %s\n", error, lodepng_error_text(error));
        OutputDebugStringA(buf);
    }

    //generate and fill the texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    free(image);

    //sampler settings
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return texture;
}

#include "Sphere.hpp"
#include "VertexShader.hpp"
#include "FragmentShader.hpp"

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

BOOL InitOpenGL(HWND hWnd)
{
    //create program
    program = CreateShaderProgram(vShaderStr, fShaderStr);
    _CheckGLError_;

    //attribute
    attributePos = 0;
    attributeTexCoord = 1;
    attributeNormal = 2;
    //mvp
    uniformModel = glGetUniformLocation(program, "model");
    uniformView = glGetUniformLocation(program, "view");
    uniformProjection = glGetUniformLocation(program, "projection");

    // material parameters
    GLint uniformAlbedoMap = glGetUniformLocation(program, "albedoMap");
    GLint uniformNormalMap = glGetUniformLocation(program, "normalMap");
    GLint uniformMetallicMap = glGetUniformLocation(program, "metallicMap");
    GLint uniformRoughnessMap = glGetUniformLocation(program, "roughnessMap");
    GLint uniformAoMap = glGetUniformLocation(program, "aoMap");

    // lights
    uniformLightPositions = glGetUniformLocation(program, "lightPositions");
    uniformLightColors = glGetUniformLocation(program, "lightColors");

    GLint uniformCamPos = glGetUniformLocation(program, "camPos");

    _CheckGLError_

    glUseProgram(program);
    //static projection and view
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)clientWidth / (float)clientHeight, 0.1f, 100.0f);
    glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
    glm::vec3 camPos = glm::vec3(0.0f, 0.0f, 2.0f);
    glm::mat4 view = glm::lookAt(camPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(view));

    //set sampler uniforms
    glUniform1i(uniformAlbedoMap, 0);
    glUniform1i(uniformNormalMap, 1);
    glUniform1i(uniformMetallicMap, 2);
    glUniform1i(uniformRoughnessMap, 3);
    glUniform1i(uniformAoMap, 4);

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

    _CheckGLError_;

    //textures
    albedoTexture = CreateTexture("albedo.png");
    normalTexture = CreateTexture("normal.png");
    metallicTexture = CreateTexture("metallic.png");
    roughnessTexture = CreateTexture("roughness.png");
    aoTexture = CreateTexture("ao.png");

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, albedoTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, metallicTexture);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, roughnessTexture);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, aoTexture);

    //other settings
    glViewport(0, 0, GLsizei(clientWidth), GLsizei(clientHeight));
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glDisable(GL_CULL_FACE);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glDisable(GL_DEPTH_TEST);

    _CheckGLError_;

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

int nrRows    = 7;
int nrColumns = 7;
float spacing = 1.2;

void Render()
{
    auto now = std::chrono::high_resolution_clock::now();
    auto time_span = now - startTime;
    float passedTime = (float)(time_span.count()/1000);

    // clear
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw model
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf);
    glUseProgram(program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, albedoTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, metallicTexture);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, roughnessTexture);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, aoTexture);

    // move light positions
    glm::vec3 newLightPositions[] = { glm::vec3(), glm::vec3(), glm::vec3(), glm::vec3() };
    for (unsigned int i = 0; i < sizeof(lightPositions) / sizeof(lightPositions[0]); ++i)
    {
        glm::vec4 p = glm::vec4(lightPositions[i], 0);
        auto rotation = glm::rotate(glm::mat4(1.0f), -glm::radians(0.00001f*passedTime), glm::vec3(1.0f, 1.0f, 0.0f));
        p = rotation * p;
        newLightPositions[i] = p;
    }
    glUniform3fv(uniformLightPositions, 4, (const GLfloat*)&newLightPositions[0]);
    glUniform3fv(uniformLightColors, 4, (const GLfloat*)&lightColors[0]);

    // rotate the model
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(glm::mat4(1.0f), glm::radians(0.00001f*passedTime), glm::vec3(0.3f,1.0f,0.0f));
    glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
    glDrawElements(GL_TRIANGLES, sizeof(indices)/3, GL_UNSIGNED_INT, 0);

    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    _CheckGLError_

    SwapBuffers(hDC);
}