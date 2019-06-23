#pragma once

// a unit cube centered at (0,0,0)
// position: 3 floats
// texcoord: 2 floats
// normal:   3 floats
float cube_vertices[] = {
    // back face
    -1.0f, -1.0f, -1.0f, 0.0f, 0.0f,  0.0f,  0.0f, -1.0f, // bottom-left
     1.0f,  1.0f, -1.0f, 1.0f, 1.0f,  0.0f,  0.0f, -1.0f, // top-right
     1.0f, -1.0f, -1.0f, 1.0f, 0.0f,  0.0f,  0.0f, -1.0f, // bottom-right
     1.0f,  1.0f, -1.0f, 1.0f, 1.0f,  0.0f,  0.0f, -1.0f, // top-right
    -1.0f, -1.0f, -1.0f, 0.0f, 0.0f,  0.0f,  0.0f, -1.0f, // bottom-left
    -1.0f,  1.0f, -1.0f, 0.0f, 1.0f,  0.0f,  0.0f, -1.0f, // top-left
    // front face
    -1.0f, -1.0f,  1.0f, 0.0f, 0.0f,  0.0f,  0.0f,  1.0f, // bottom-left
     1.0f, -1.0f,  1.0f, 1.0f, 0.0f,  0.0f,  0.0f,  1.0f, // bottom-right
     1.0f,  1.0f,  1.0f, 1.0f, 1.0f,  0.0f,  0.0f,  1.0f, // top-right
     1.0f,  1.0f,  1.0f, 1.0f, 1.0f,  0.0f,  0.0f,  1.0f, // top-right
    -1.0f,  1.0f,  1.0f, 0.0f, 1.0f,  0.0f,  0.0f,  1.0f, // top-left
    -1.0f, -1.0f,  1.0f, 0.0f, 0.0f,  0.0f,  0.0f,  1.0f, // bottom-left
    // left face
    -1.0f,  1.0f,  1.0f, 1.0f, 0.0f, -1.0f,  0.0f,  0.0f, // top-right
    -1.0f,  1.0f, -1.0f, 1.0f, 1.0f, -1.0f,  0.0f,  0.0f, // top-left
    -1.0f, -1.0f, -1.0f, 0.0f, 1.0f, -1.0f,  0.0f,  0.0f, // bottom-left
    -1.0f, -1.0f, -1.0f, 0.0f, 1.0f, -1.0f,  0.0f,  0.0f, // bottom-left
    -1.0f, -1.0f,  1.0f, 0.0f, 0.0f, -1.0f,  0.0f,  0.0f, // bottom-right
    -1.0f,  1.0f,  1.0f, 1.0f, 0.0f, -1.0f,  0.0f,  0.0f, // top-right
    // right face
     1.0f,  1.0f,  1.0f, 1.0f, 0.0f,  1.0f,  0.0f,  0.0f, // top-left
     1.0f, -1.0f, -1.0f, 0.0f, 1.0f,  1.0f,  0.0f,  0.0f, // bottom-right
     1.0f,  1.0f, -1.0f, 1.0f, 1.0f,  1.0f,  0.0f,  0.0f, // top-right
     1.0f, -1.0f, -1.0f, 0.0f, 1.0f,  1.0f,  0.0f,  0.0f, // bottom-right
     1.0f,  1.0f,  1.0f, 1.0f, 0.0f,  1.0f,  0.0f,  0.0f, // top-left
     1.0f, -1.0f,  1.0f, 0.0f, 0.0f,  1.0f,  0.0f,  0.0f, // bottom-left
    // bottom face
    -1.0f, -1.0f, -1.0f, 0.0f, 1.0f,  0.0f, -1.0f,  0.0f, // top-right
     1.0f, -1.0f, -1.0f, 1.0f, 1.0f,  0.0f, -1.0f,  0.0f, // top-left
     1.0f, -1.0f,  1.0f, 1.0f, 0.0f,  0.0f, -1.0f,  0.0f, // bottom-left
     1.0f, -1.0f,  1.0f, 1.0f, 0.0f,  0.0f, -1.0f,  0.0f, // bottom-left
    -1.0f, -1.0f,  1.0f, 0.0f, 0.0f,  0.0f, -1.0f,  0.0f, // bottom-right
    -1.0f, -1.0f, -1.0f, 0.0f, 1.0f,  0.0f, -1.0f,  0.0f, // top-right
    // top face
    -1.0f,  1.0f, -1.0f, 0.0f, 1.0f,  0.0f,  1.0f,  0.0f, // top-left
     1.0f,  1.0f , 1.0f, 1.0f, 0.0f,  0.0f,  1.0f,  0.0f, // bottom-right
     1.0f,  1.0f, -1.0f, 1.0f, 1.0f,  0.0f,  1.0f,  0.0f, // top-right
     1.0f,  1.0f,  1.0f, 1.0f, 0.0f,  0.0f,  1.0f,  0.0f, // bottom-right
    -1.0f,  1.0f, -1.0f, 0.0f, 1.0f,  0.0f,  1.0f,  0.0f, // top-left
    -1.0f,  1.0f,  1.0f, 0.0f, 0.0f,  0.0f,  1.0f,  0.0f  // bottom-left
};