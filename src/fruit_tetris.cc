//////////////////////////////////////////////////////////////////////////////
//
//  fruit_tetris.cc
//
//  CMPT 361 Assignment 1 - FruitTetris implementation Sample Skeleton Code
//
//  projMatrixect         : FruitTetris
//  Name            : Chong Guo
//  Student ID      : 301295753
//  SFU username    : armourg
//  Instructor      : Thomas Shermer
//  TA              : Luwei Yang
//
//  This code is extracted from Connor MacLeod's (crmacleo@sfu.ca) assignment submission
//  by Rui Ma (ruim@sfu.ca) on 2014-03-04.
//
//  Modified in Sep 2014 by Honghua Li (honghual@sfu.ca).
//
//////////////////////////////////////////////////////////////////////////////

#include "game_manager.h"
#include "init_shader.h"
#include "include/Angel.h"
#include <ctime>
#include <cstdlib>
#include <math.h>
#include <string.h>
using namespace std;

// The global game manager
GameManager manager;

// Represent the window size - updated if window is reshaped to prevent stretching of the game
int size_x = 400;
int size_y = 720;

// An array containing the color of each of the 10*20*2*3 vertices that make up the board
glm::vec4 board_colors[1200*6]; // 6 faces of the cube

// Location of vertex attributes in the shader program
GLuint v_position;
GLuint v_color;

// Locations of uniform variables in shader program
GLuint loc_size_x;
GLuint loc_size_y;

// VAO and VBO
GLuint vao_IDs[3]; // One VAO for each object: the grid, the board, the current piece
GLuint vbo_IDs[6]; // Two Vertex Buffer Objects for each VAO (specifying vertex positions and colours, respectively)


GLuint gridMVP; // model view projection - a multiplication of 3 matrices below
mat4 projMatrix;
mat4 viewMatrix;
mat4 modelMatrix;


// robot stuff
// much of this was heavily influenced by chp 8 code from class textboox
#define ROBOT_BASE_HEIGHT 10.0
#define ROBOT_BASE_WIDTH 50.0
#define ROBOT_BOTARM_HEIGHT 125.0
#define ROBOT_BOTARM_WIDTH 10.0 
#define ROBOT_TOPARM_HEIGHT 125.0
#define ROBOT_TOPARM_WIDTH 10.0

typedef Angel::vec4 point4;
typedef Angel::vec4 color4;

const int NumVertices = 36;
point4 points[NumVertices];
color4 colors[NumVertices];

point4 vertices[8] = {
    point4( -0.5, -0.5,  0.5, 1.0 ),
    point4( -0.5,  0.5,  0.5, 1.0 ),
    point4(  0.5,  0.5,  0.5, 1.0 ),
    point4(  0.5, -0.5,  0.5, 1.0 ),
    point4( -0.5, -0.5, -0.5, 1.0 ),
    point4( -0.5,  0.5, -0.5, 1.0 ),
    point4(  0.5,  0.5, -0.5, 1.0 ),
    point4(  0.5, -0.5, -0.5, 1.0 )
};

// RGBA olors
color4 vertex_colors[8] = {
    color4( 0.0, 0.0, 0.0, 1.0 ),  // black
    color4( 1.0, 0.0, 0.0, 1.0 ),  // red
    color4( 1.0, 1.0, 0.0, 1.0 ),  // yellow
    color4( 0.0, 1.0, 0.0, 1.0 ),  // green
    color4( 0.0, 0.0, 1.0, 1.0 ),  // blue
    color4( 1.0, 0.0, 1.0, 1.0 ),  // magenta
    color4( 0.4, 0.5, 0.2, 1.0 ),  // white
    color4( 0.0, 1.0, 1.0, 1.0 )   // cyan
};

int Index = 0;

void
quad( int a, int b, int c, int d )
{
    colors[Index] = vertex_colors[a]; points[Index] = vertices[a]; Index++;
    colors[Index] = vertex_colors[a]; points[Index] = vertices[b]; Index++;
    colors[Index] = vertex_colors[a]; points[Index] = vertices[c]; Index++;
    colors[Index] = vertex_colors[a]; points[Index] = vertices[a]; Index++;
    colors[Index] = vertex_colors[a]; points[Index] = vertices[c]; Index++;
    colors[Index] = vertex_colors[a]; points[Index] = vertices[d]; Index++;
}

void
colorcube( void )
{
    quad( 1, 0, 3, 2 );
    quad( 2, 3, 7, 6 );
    quad( 3, 0, 4, 7 );
    quad( 6, 5, 1, 2 );
    quad( 4, 5, 6, 7 );
    quad( 5, 4, 0, 1 );
}

enum {
  Base,
  BottomArm,
  TopArm
};

GLfloat theta[3] = { 0.0 };

mat4 robotMVP; //MVP matrix for our robot
GLuint robotVAO;
GLuint robotBuffer;

vec3 robot_position;

float getRadians(float degrees) {
    return degrees * M_PI/180.0;
}
void init_robot() {
    robot_position = vec3(-100, -100, 0);

    // setup the boxes for the robot
    colorcube();

    glGenVertexArrays(1, &robotVAO);
    glBindVertexArray(robotVAO);

    glGenBuffers(1, &robotBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, robotBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points) + sizeof(colors), NULL, GL_DYNAMIC_DRAW);
    glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(points), points );
    glBufferSubData( GL_ARRAY_BUFFER, sizeof(points), sizeof(colors),
                     colors );

    glEnableVertexAttribArray( v_position );
    glVertexAttribPointer( v_position, 4, GL_FLOAT, GL_FALSE, 0,
               BUFFER_OFFSET(0) );

    glEnableVertexAttribArray( v_color );
    glVertexAttribPointer( v_color, 4, GL_FLOAT, GL_FALSE, 0,
               BUFFER_OFFSET(sizeof(points)) );
    //position arm
    theta[BottomArm] = libconsts::kBottomArmAngle;
    theta[TopArm] = libconsts::kTopArmAngle;

}

void setup_robot_base(mat4 &m) {
    //instance transformation as described in text book
    mat4 instance = ( Translate( 0.0, 0.5 * ROBOT_BASE_HEIGHT, 0.0 ) *
              Scale( ROBOT_BASE_WIDTH,
                 ROBOT_BASE_HEIGHT,
                 ROBOT_BASE_WIDTH ) );
    
    glUniformMatrix4fv( gridMVP, 1, GL_TRUE, m * robotMVP * instance);
    glDrawArrays( GL_TRIANGLES, 0, NumVertices );
}

void setup_robot_botarm(mat4& m) {
    //instance transformation as described in text book
    mat4 instance = ( Translate( 0.0, 0.5 * ROBOT_BOTARM_HEIGHT, 0.0 ) *
              Scale( ROBOT_BOTARM_WIDTH,
                 ROBOT_BOTARM_HEIGHT,
                 ROBOT_BOTARM_WIDTH ) );
    
    glUniformMatrix4fv( gridMVP, 1, GL_TRUE, m * robotMVP * instance);
    glDrawArrays( GL_TRIANGLES, 0, NumVertices );
}

void setup_robot_toparm(mat4& m) {
    //instance transformation as described in text book
    mat4 instance = ( Translate( 0.0, 0.5 * ROBOT_TOPARM_HEIGHT, 0.0 ) *
              Scale( ROBOT_TOPARM_WIDTH,
                 ROBOT_TOPARM_HEIGHT,
                 ROBOT_TOPARM_WIDTH ) );
    
    glUniformMatrix4fv( gridMVP, 1, GL_TRUE, m * robotMVP * instance);
    glDrawArrays( GL_TRIANGLES, 0, NumVertices );
}
void setup_robot() {
    glBindVertexArray(robotVAO);

    mat4 m = projMatrix * viewMatrix * Translate(robot_position);
    robotMVP = RotateY(theta[Base]);
    setup_robot_base(m);

    robotMVP *= Translate(0.0, ROBOT_BASE_HEIGHT, 0.0);
    robotMVP *= RotateZ(theta[BottomArm]);
    setup_robot_botarm(m);

    robotMVP *= Translate(0.0, ROBOT_BOTARM_HEIGHT, 0.0);
    robotMVP *= RotateZ(theta[TopArm]);
    setup_robot_toparm(m);
    robotMVP *= Translate(0.0, ROBOT_TOPARM_HEIGHT, 0.0);
}

glm::vec2 robotTip() {
    glm::vec2 ret;
    ret.x += robot_position.x/20;
    ret.y += ROBOT_BASE_HEIGHT/10; //move up to underneath bottom

    ret.x += ROBOT_BOTARM_HEIGHT/10 * -sin(getRadians(theta[BottomArm]));
    ret.y += ROBOT_BOTARM_HEIGHT/10 * cos(getRadians(theta[BottomArm]));

    float gamma = 270.0;
    gamma -= theta[BottomArm];
    gamma -= theta[TopArm];

    ret.x += ROBOT_TOPARM_HEIGHT/10 * cos(getRadians(gamma));
    ret.y += ROBOT_TOPARM_HEIGHT/10 * -sin(getRadians(gamma));

    ret.x = (int) ret.x;
    ret.y = (int) ret.y;
    return ret;
}
//
// Function: UpdateTilePosition
// ---------------------------
//
//   When the current tile is moved or rotated (or created), update the VBO containing its vertex position data
//
//   Parameters:
//       void
//
//   Returns:
//       void
//

void UpdateTilePosition() {
    // Bind the VBO containing current tile vertex positions
    glBindBuffer(GL_ARRAY_BUFFER, vbo_IDs[4]);

    // For each of the 4 'cells' of the tile,
    for (int i = 0; i < libconsts::kCountCells; i++) {

        // Calculate the grid coordinates of the cell
        GLfloat x = manager.get_tile_current_position().x + libconsts::kShapeCategory[manager.get_tile_current_shape()][manager.get_tile_current_orient()][i].x;
        GLfloat y = manager.get_tile_current_position().y + libconsts::kShapeCategory[manager.get_tile_current_shape()][manager.get_tile_current_orient()][i].y;

        // 8 corners in a cube - front and back
        glm::vec4 p1 = glm::vec4(33.0 + (x * 33.0), 33.0 + (y * 33.0), 16.50, 1); 
        glm::vec4 p2 = glm::vec4(33.0 + (x * 33.0), 66.0 + (y * 33.0), 16.50, 1); 
        glm::vec4 p3 = glm::vec4(66.0 + (x * 33.0), 33.0 + (y * 33.0), 16.50, 1); 
        glm::vec4 p4 = glm::vec4(66.0 + (x * 33.0), 66.0 + (y * 33.0), 16.50, 1); 
        glm::vec4 p5 = glm::vec4(33.0 + (x * 33.0), 33.0 + (y * 33.0), -16.50, 1); 
        glm::vec4 p6 = glm::vec4(33.0 + (x * 33.0), 66.0 + (y * 33.0), -16.50, 1); 
        glm::vec4 p7 = glm::vec4(66.0 + (x * 33.0), 33.0 + (y * 33.0), -16.50, 1); 
        glm::vec4 p8 = glm::vec4(66.0 + (x * 33.0), 66.0 + (y * 33.0), -16.50, 1); 

        // Two points are used by two triangles each
        glm::vec4 newpoints[36] = {  p1, p2, p3, p2, p3, p4,
                                p5, p6, p7, p6, p7, p8,
                                p1, p2, p5, p2, p5, p6,
                                p3, p4, p7, p4, p7, p8,
                                p2, p4, p6, p4, p6, p8,
                                p1, p3, p5, p3, p5, p7};

        // Put new data in the VBO
        glBufferSubData(GL_ARRAY_BUFFER, i * sizeof(newpoints), sizeof(newpoints), newpoints);

    }
}

//
// Function: UpdateTileColor
// ---------------------------
//
//   When the current tile is moved or rotated (or created), update the VBO containing its vertex color data
//
//   Parameters:
//       void
//
//   Returns:
//       void
//

void UpdateTileColor() {
    // Get color from color category;
    // grey out if colliding
    glm::vec4 new_colors[24*6];
    if (!manager.getColliding()) {
        for (int i = 0; i < 24*6; i++) {
            new_colors[i] = libconsts::kColorCategory[manager.get_tile_current_color()[i / 36]];
        }
    } else {
        for (int i = 0; i < 24*6; i++) {
            new_colors[i] = glm::vec4(0.5, 0.5, 0.5, 1.0); //grey
        }
    }

    // Update the color VBO of current tile
    glBindBuffer(GL_ARRAY_BUFFER, vbo_IDs[5]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(new_colors), new_colors);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

//
// Function: UpdateTileDisplay
// ---------------------------
//
//   When the current tile is moved or rotated (or created), update all of its VBO
//
//   Parameters:
//       void
//
//   Returns:
//       void
//

void UpdateTileDisplay() {
    // Update the geometry VBO of current tile
    UpdateTilePosition();

    // Update the color VBO of current tile
    UpdateTileColor();

    // Post redisplay
    glutPostRedisplay();
}

//
// Function: UpdateBoard
// ---------------------------
//
//   Update the VBO containing current board information
//
//   Parameters:
//       void
//
//   Returns:
//       void
//

void UpdateBoard() {
    // Get current map color data
    std::vector<std::vector<int>> &map_data = manager.get_map_data();

    // Let the empty cells on the board be black
    for (int i = 0; i < 1200*6; i++) {
        int x = i / 36 / (int)manager.get_map_size().x;
        int y = i / 36 % (int)manager.get_map_size().x;
        board_colors[i] = libconsts::kColorCategory[map_data[y][x]];
    }

    // Update grid cell vertex colours
    glBindBuffer(GL_ARRAY_BUFFER, vbo_IDs[3]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, 1200 * 6 * sizeof(glm::vec4), board_colors);
}

//
// Function: NewTile
// ---------------------------
//
//   Called at the start of play and every time a tile is placed
//
//   Parameters:
//       void
//
//   Returns:
//       void
//

void NewTile() {
    // Add a new tile
    manager.AddNewTile(robotTip());

    // Update tile display
    UpdateTileDisplay();
}

//
// Function: InitGrid
// ---------------------------
//
//   Init grid line
//
//   Parameters:
//       void
//
//   Returns:
//       void
//

void InitGrid() {
    // 590 = 64 * 2 front and back + 462 connecting depth lines 
    

    glm::vec4 grid_points[590];
    glm::vec4 grid_colors[590];
    // Vertical lines 
    for (int i = 0; i < 11; i++){
        grid_points[2*i]      = glm::vec4((33.0 + (33.0 * i)), 33.0, 16.5, 1);
        grid_points[2*i + 1]  = glm::vec4((33.0 + (33.0 * i)), 693.0, 16.5, 1);
        grid_points[2*i + 64] = glm::vec4((33.0 + (33.0 * i)), 33.0, -16.5, 1);
        grid_points[2*i + 65] = glm::vec4((33.0 + (33.0 * i)), 693.0, -16.5, 1);
    }
    // Horizontal lines
    for (int i = 0; i < 21; i++){
        grid_points[22 + 2*i]        = glm::vec4(33.0, (33.0 + (33.0 * i)), 16.5, 1);
        grid_points[22 + 2*i + 1]    = glm::vec4(363.0, (33.0 + (33.0 * i)), 16.5, 1);
        grid_points[22 + 2*i + 64]   = glm::vec4(33.0, (33.0 + (33.0 * i)), -16.5, 1);
        grid_points[22 + 2*i + 65]   = glm::vec4(363.0, (33.0 + (33.0 * i)), -16.5, 1);
    }
    // depth lines
    for (int i = 0; i < 21; i++){
        for (int j = 0; j < 11; j++) {
            grid_points[128 + 22*i + 2*j]        = glm::vec4(33.0 + (j * 33.0), 33.0 + (i * 33.0), 16.5, 1); // front left bottom
            grid_points[128 + 22*i + 2*j + 1]    = glm::vec4(33.0 + (j * 33.0), 33.0 + (i * 33.0), -16.5, 1); // back left bottom
        }
    }
    // Make all grid lines white
    for (int i = 0; i < 590; i++)
        grid_colors[i] = libconsts::kColorCategory[1]; // white



    glBindVertexArray(vao_IDs[0]); // Bind the first VAO
    glGenBuffers(2, vbo_IDs); // Create two Vertex Buffer Objects for this VAO (positions, colours)

    // Grid vertex positions
    glBindBuffer(GL_ARRAY_BUFFER, vbo_IDs[0]); // Bind the first grid VBO (vertex positions)
    glBufferData(GL_ARRAY_BUFFER, 590*sizeof(glm::vec4), grid_points, GL_STATIC_DRAW); // Put the grid points in the VBO
    glVertexAttribPointer(v_position, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(v_position); // Enable the attribute
    
    // Grid vertex colours
    glBindBuffer(GL_ARRAY_BUFFER, vbo_IDs[1]); // Bind the second grid VBO (vertex colours)
    glBufferData(GL_ARRAY_BUFFER, 590*sizeof(glm::vec4), grid_colors, GL_DYNAMIC_DRAW); // Put the grid colours in the VBO
    glVertexAttribPointer(v_color, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(v_color); // Enable the attribute
}

//
// Function: InitBoard
// ---------------------------
//
//   Init board
//
//   Parameters:
//       void
//
//   Returns:
//       void
//
void InitBoard() {
    glm::vec4 board_points[1200*6];
    for (int i=0; i<1200*6; i++) {
        board_colors[i] = libconsts::kColorCategory[0];
    }
    // Each cell is a square (2 triangles with 6 vertices)
    // repeat for each face 
    for (int i = 0; i < 20; i++){
        for (int j = 0; j < 10; j++) {
            glm::vec4 p1 = glm::vec4(33.0 + (j * 33.0), 33.0 + (i * 33.0), 16.5, 1);
            glm::vec4 p2 = glm::vec4(33.0 + (j * 33.0), 66.0 + (i * 33.0), 16.5, 1);
            glm::vec4 p3 = glm::vec4(66.0 + (j * 33.0), 33.0 + (i * 33.0), 16.5, 1);
            glm::vec4 p4 = glm::vec4(66.0 + (j * 33.0), 66.0 + (i * 33.0), 16.5, 1);
            glm::vec4 p5 = glm::vec4(33.0 + (j * 33.0), 33.0 + (i * 33.0), -16.5, 1);
            glm::vec4 p6 = glm::vec4(33.0 + (j * 33.0), 66.0 + (i * 33.0), -16.5, 1);
            glm::vec4 p7 = glm::vec4(66.0 + (j * 33.0), 33.0 + (i * 33.0), -16.5, 1);
            glm::vec4 p8 = glm::vec4(66.0 + (j * 33.0), 66.0 + (i * 33.0), -16.5, 1); 

            board_points[36 * (10 * i + j)    ] = p1;
            board_points[36 * (10 * i + j) + 1] = p2;
            board_points[36 * (10 * i + j) + 2] = p3;
            board_points[36 * (10 * i + j) + 3] = p2;
            board_points[36 * (10 * i + j) + 4] = p3;
            board_points[36 * (10 * i + j) + 5] = p4;

            board_points[36 * (10 * i + j) + 6 ] = p5;
            board_points[36 * (10 * i + j) + 7 ] = p6;
            board_points[36 * (10 * i + j) + 8 ] = p7;
            board_points[36 * (10 * i + j) + 9 ] = p6;
            board_points[36 * (10 * i + j) + 10] = p7;
            board_points[36 * (10 * i + j) + 11] = p8;

            board_points[36 * (10 * i + j) + 12] = p1;
            board_points[36 * (10 * i + j) + 13] = p2;
            board_points[36 * (10 * i + j) + 14] = p5;
            board_points[36 * (10 * i + j) + 15] = p2;
            board_points[36 * (10 * i + j) + 16] = p5;
            board_points[36 * (10 * i + j) + 17] = p6;

            board_points[36 * (10 * i + j) + 18] = p3;
            board_points[36 * (10 * i + j) + 19] = p4;
            board_points[36 * (10 * i + j) + 20] = p7;
            board_points[36 * (10 * i + j) + 21] = p4;
            board_points[36 * (10 * i + j) + 22] = p7;
            board_points[36 * (10 * i + j) + 23] = p8;

            board_points[36 * (10 * i + j) + 24] = p2;
            board_points[36 * (10 * i + j) + 25] = p4;
            board_points[36 * (10 * i + j) + 26] = p6;
            board_points[36 * (10 * i + j) + 27] = p4;
            board_points[36 * (10 * i + j) + 28] = p6;
            board_points[36 * (10 * i + j) + 29] = p8;

            board_points[36 * (10 * i + j) + 30] = p1;
            board_points[36 * (10 * i + j) + 31] = p3;
            board_points[36 * (10 * i + j) + 32] = p5;
            board_points[36 * (10 * i + j) + 33] = p3;
            board_points[36 * (10 * i + j) + 34] = p5;
            board_points[36 * (10 * i + j) + 35] = p7;  
        }
    }

    // Set up buffer objects
    glBindVertexArray(vao_IDs[1]);
    glGenBuffers(2, &vbo_IDs[2]);

    // Grid cell vertex positions
    glBindBuffer(GL_ARRAY_BUFFER, vbo_IDs[2]);
    glBufferData(GL_ARRAY_BUFFER, 1200 * 6 * sizeof(glm::vec4), board_points, GL_STATIC_DRAW);
    glVertexAttribPointer(v_position, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(v_position);

    // Grid cell vertex colours
    glBindBuffer(GL_ARRAY_BUFFER, vbo_IDs[3]);
    glBufferData(GL_ARRAY_BUFFER, 1200 * 6 * sizeof(glm::vec4), board_colors, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(v_color, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(v_color);
}

//
// Function: InitCurrentTile
// ---------------------------
//
//   No geometry for current tile initially
//
//   Parameters:
//       void
//
//   Returns:
//       void
//

void InitCurrentTile() {
    // Bind to current tile buffer vertex array
    glBindVertexArray(vao_IDs[2]);
    glGenBuffers(2, &vbo_IDs[4]);

    // Current tile vertex positions
    glBindBuffer(GL_ARRAY_BUFFER, vbo_IDs[4]);
    glBufferData(GL_ARRAY_BUFFER, 24 * 6 * sizeof(glm::vec4), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(v_position, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(v_position);

    // Current tile vertex colours
    glBindBuffer(GL_ARRAY_BUFFER, vbo_IDs[5]);
    glBufferData(GL_ARRAY_BUFFER, 24 * 6 * sizeof(glm::vec4), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(v_color, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(v_color);
}

//
// Function: Init
// ---------------------------
//
//   Init all things, including shader
//
//   Parameters:
//       void
//
//   Returns:
//       void
//


void Init() {
    // Initialize the time seed
    srand(time(0));

    // Init game manager
    manager.Init(10, 20);

    // Load shaders and use the shader program
#ifdef __APPLE__
    GLuint program = angel::InitShader("vshader_mac.glsl", "fshader_mac.glsl");
#else
    GLuint program = angel::InitShader("vshader_unix.glsl", "fshader_unix.glsl");
#endif
    glUseProgram(program);

    // Get the location of the attributes (for glVertexAttribPointer() calls)
    v_position = glGetAttribLocation(program, "v_position");
    v_color = glGetAttribLocation(program, "v_color");

    // Create 3 Vertex Array Objects, each representing one 'object'. Store the names in array vao_IDs
    glGenVertexArrays(3, &vao_IDs[0]);

    // Initialize the grid, the board, and the current tile and the robot
    InitGrid();
    InitBoard();
    InitCurrentTile();
    init_robot();

    gridMVP = glGetUniformLocation(program, "MVP");
    viewMatrix = LookAt( 
            vec3(0, 70, 300),
            vec3(0, 10, 0),
            vec3(0, 1, 0));


    // Game initialization
    NewTile(); // create new next tile


    glEnable(GL_BLEND); 
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClearDepth(1.0);

   

}

//
// Function: Display
// ---------------------------
//
//   Display callback function
//
//   Parameters:
//       void
//
//   Returns:
//       void
//

void drawBitmapText(std::string string,float x,float y) 
{  
    glRasterPos2f(x, y);

    for (char& c : string) 
    {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, c);
    }
}
void drawText(int x, int y, char *string)
{
    int i, len; 
   
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    glLoadIdentity();
    glTranslatef(0.0f, 10.0f, 0.0f);
    glRasterPos2i(x, y);
    
    glDisable(GL_TEXTURE);
    glDisable(GL_TEXTURE_2D);
    for (i = 0, len = strlen(string); i < len; i++)
    {    
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, (int)string[i]);
    }
    glEnable(GL_TEXTURE);
    glEnable(GL_TEXTURE_2D);
}
void Display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // 42
    projMatrix = Perspective(48, 1*size_x/size_y, 1, 500);

    setup_robot();

    mat4 modelMatrix = mat4();
    modelMatrix *= Translate(0, 10, 0);
    modelMatrix *= Scale(0.33, 0.33, 0.33); 
    modelMatrix *= Translate(-200, -360, 0); 


    //model coords -> world coords -> camera coords -> homogenous
    mat4 MVP = projMatrix * viewMatrix * modelMatrix; //form the mvp transformation
    glUniformMatrix4fv(gridMVP, 1, GL_TRUE, MVP);

    glBindVertexArray(vao_IDs[1]);      // Bind the VAO representing the grid cells (to be drawn first)
    glDrawArrays(GL_TRIANGLES, 0, 1200*6);        // Draw the board (10 * 20 * 2 = 400 triangles)

    if (manager.get_game_state() != GameState::GameStateEnd) {
        glBindVertexArray(vao_IDs[2]);          // Bind the VAO representing the current tile (to be drawn on top of the board)
        glDrawArrays(GL_TRIANGLES, 0, 24*6);      // Draw the current tile (8 triangles)
    }

    glBindVertexArray(vao_IDs[0]);      // Bind the VAO representing the grid lines (to be drawn on top of everything else)
    glDrawArrays(GL_LINES, 0, 590);      // Draw the grid lines (21+11 = 32 lines)

    


    int a = rand() % 10000;
    int b = (rand() % 10000) ;
    //cout << a << "," << b << "\n";
    drawText(a,b,"rewrew");

 //   drawBitmapText("Rewrew", a , b);
    glutSwapBuffers();
}

//
// Function: Reshape
// ---------------------------
//
//   Reshape callback function
//
//   Parameters:
//       w: the width of the window
//       h: the height of the window
//
//   Returns:
//       void
//

void Reshape(GLsizei w, GLsizei h) {
    size_x = w;
    size_y = h;
    glViewport(0, 0, w, h);
}

//
// Function: Tick
// ---------------------------
//
//   Timer callback function
//
//   Parameters:
//       w: the width of the window
//       h: the height of the window
//
//   Returns:
//       void
//

void Tick(int value) {
    manager.Tick();
    UpdateBoard();
    UpdateTileDisplay();
    glutTimerFunc(manager.get_tick_interval(), Tick, 0);
}

//
// Function: Special
// ---------------------------
//
//   Special input callback function
//
//   Parameters:
//       key: the key that user pressed
//
//   Returns:
//       void
//

void Special(int key, int, int) {
    switch (key) {
        case GLUT_KEY_LEFT:         // Move left
            if (glutGetModifiers() == GLUT_ACTIVE_CTRL) {
                viewMatrix *= RotateY(5); 
            } else {
              //  manager.MoveTile(libconsts::kMoveLeft);
              //  UpdateTileDisplay();
            }
            
            break;
        case GLUT_KEY_RIGHT:        // Move right
            if (glutGetModifiers() == GLUT_ACTIVE_CTRL) {
                viewMatrix *= RotateY(-5); 
            } else {
              //  manager.MoveTile(libconsts::kMoveRight);
              //  UpdateTileDisplay();
            }
            break;
        case GLUT_KEY_UP:           // Rotate
            if (glutGetModifiers() == GLUT_ACTIVE_CTRL) {
                viewMatrix *= RotateZ(5); 
            } else {
                manager.RotateTile(libconsts::kClockWise);
                UpdateTileDisplay();
            }
            break;
        case GLUT_KEY_DOWN:         // Drop down
           // manager.MoveTile(libconsts::kMoveDown);
          //  UpdateTileDisplay();
            break;
        default:
            break;
    }
}

//
// Function: Keyboard
// ---------------------------
//
//   Keyboard input callback function
//
//   Parameters:
//       key: the key that user pressed
//
//   Returns:
//       void
//

void Keyboard(unsigned char key, int, int) {
    switch (key) {
        case 033:   // Both escape key and 'q' cause the game to exit
            exit(EXIT_SUCCESS);
        case 'q':   // Both escape key and 'q' cause the game to exit
            exit (EXIT_SUCCESS);
        case '1':   // '1' key enter easy mode
            manager.Easy();
            break;
        case '2':   // '2' key enter normal mode
            manager.Normal();
            break;
        case '3':   // '3' key enter hard mode
            manager.Hard();
            break;
        case '4':   // '4' key enter insane mode
            manager.Insane();
            break;
        case 'r':   // 'r' key restarts the game
            manager.Restart();
            theta[BottomArm] = libconsts::kBottomArmAngle;
            theta[TopArm] = libconsts::kTopArmAngle;
            manager.AddNewTile(robotTip());

            UpdateBoard();
            UpdateTileDisplay();
            break;
        case 'p':   // 'p' key pause or resume the game
            if (manager.get_game_state() == GameState::GameStatePause)
                manager.Resume();
            else
                manager.Pause();
            break;
        case 'a':
            theta[BottomArm] += 3;
            std::cout << robotTip().x << "," << robotTip().y <<","<<theta[BottomArm]<<"\n";
            manager.MoveTile(robotTip());
            UpdateTileDisplay();


            break;
        case 'd':
            theta[BottomArm] -= 3;
            std::cout << robotTip().x << "," << robotTip().y <<","<<theta[BottomArm]<<"\n";
            manager.MoveTile(robotTip());
            UpdateTileDisplay();
            break;
        case 'w':
            theta[TopArm] += 3;
            std::cout << robotTip().x << "," << robotTip().y <<","<<theta[TopArm]<<"\n";
            manager.MoveTile(robotTip());
            UpdateTileDisplay();

            break;
        case 's':
            theta[TopArm] -= 3;
            std::cout << robotTip().x << "," << robotTip().y <<","<<theta[TopArm]<<"\n";
            manager.MoveTile(robotTip());
            UpdateTileDisplay();
            break;
        case ' ':
            manager.setTileFalling(true);
            manager.setTimeTilDrop(libconsts::kTimeTilDrop);

            break;

    }
    glutPostRedisplay();
}

//
// Function: Idle
// ---------------------------
//
//   Idle callback function
//
//   Parameters:
//       void
//
//   Returns:
//       void
//

void Idle(void) {
    glutPostRedisplay();
}

//
// Function: Main
// ---------------------------
//
//   The main function
//
//   Parameters:
//       argc: the number of parameters in main function
//       argv[]: the array of parameters in main function
//
//   Returns:
//       void
//

int main(int argc, char **argv) {
    glutInit(&argc, argv);
#ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE);
#else
    glutInitDisplayMode(GLUT_MULTISAMPLE | GLUT_DEPTH | GLUT_RGBA | GLUT_DOUBLE);
#endif

    // Init glut window
   // glutInitWindowSize(size_x, size_y);
    glutInitWindowSize(800, 720);

    glutInitWindowPosition(libconsts::kWindowPositionX, libconsts::kWindowPositionY);
    glutCreateWindow("Fruit Tetris");

    // Init
#ifndef __APPLE__
    glewInit();
#endif
    glewInit();
    Init();

    // Setup Callback functions
    glutDisplayFunc(Display);
    glutReshapeFunc(Reshape);
    glutSpecialFunc(Special);
    glutKeyboardFunc(Keyboard);
    glutIdleFunc(Idle);
    glutTimerFunc(libconsts::kTickNormalMode, Tick, 0);

    // Start main loop
    glutMainLoop();
    return 0;
}
