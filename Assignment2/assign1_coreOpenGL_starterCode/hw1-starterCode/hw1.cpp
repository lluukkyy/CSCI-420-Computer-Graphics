/*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields with Shaders.
  C++ starter code

  Student username: <haoyunzh>
*/

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "basicPipelineProgram.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "openGLHeader.h"
#include "glutHeader.h"

#include <iostream>
#include <cstring>
#include <math.h>
#include <time.h>

#if defined(WIN32) || defined(_WIN32)
  #ifdef _DEBUG
    #pragma comment(lib, "glew32d.lib")
  #else
    #pragma comment(lib, "glew32.lib")
  #endif
#endif

#if defined(WIN32) || defined(_WIN32)
  char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
  char mainShaderBasePath[1024] = "../openGLHelper-starterCode/mainShader";
  char texShaderBasePath[1024] = "../openGLHelper-starterCode/textureShader";
#endif

using namespace std;

time_t startTime;

int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework II";

//stores vbo vertices for triangular rendering of the rail
glm::vec3* railPosition;
//corresponding color info
glm::vec4* railColor;
int railIsVisible = 1;

unsigned int frame = 0; // frame for camera animation
float cameraHeightonRail = 0.2; //height of the camera above the camera spline
glm::vec3* cameraPosition; //catMull spline of camera
glm::vec3* cameraDirection; //direction of the camera
glm::vec3* cameraNormal; //normal vector of the camera view
glm::vec3* cameraBinormal; //binormal vector of the camera view
int cameraPositionSize; //number of points used in the catMull camera spline.

int curveSize;//size of catMull curve
float CM_s = 0.5; //variable s in CatMull-Rom basis
glm::mat4 CatMullBasis = glm::mat4(-CM_s,2*CM_s,-CM_s,0,
                                    2-CM_s,CM_s-3,0,1,
                                    CM_s-2,3-2*CM_s,CM_s,0,
                                    CM_s,-CM_s,0,0);

GLuint vertexArray; //vao of the rail
GLuint vertexBuffer; //vbo of the rail in triangular order
GLuint colorVertexBuffer;//correcponding color info

unsigned int texSize; //the size of vbo vertices in texture rendering
GLuint texHandle; //texture handle 
GLuint groundVertexArray; //vao for texture rendering
GLuint texVertexBuffer; //vbo for 2d vertices in texture rendering
GLuint groundVertexBuffer; //vbo for 3d vertices in texture rendering

// initialize some public variables
unsigned int numDrawingPoints;

//following variables are used to record view movement.
// The reason why I don't control drawing functions directly through key inputs is to
// split the model and controller part of the program. Drawing functions below are 
// for models and detecting key inputs is the job for the controller.
unsigned char drawingMode = 0; // drawing mode in glDrawArray
//toggle for TRANSLATE movement due to the malfunction of detecting CTRL input on macOS
int translateMode = 0; 

OpenGLMatrix matrix;
BasicPipelineProgram * mainPipelineProgram;//main pipeline for the railway shader
BasicPipelineProgram * texPipelineProgram;//texture pipeline for texture shader


// represents one control point along the spline 
struct Point 
{
  double x;
  double y;
  double z;
};

// spline struct 
// contains how many control points the spline has, and an array of control points 
struct Spline 
{
  int numControlPoints;
  Point * points;
};

// the spline array 
Spline * splines;
int numSplines; // total number of splines
//position range of the current spline
float xMin, xMax; 
float yMin, yMax;
float zMin, zMax;
float xLength, yLength, zLength;

// write a screenshot to the specified filename
void saveScreenshot()
{
  unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);
  char  filename[100];
  sprintf(filename,"./%03d.jpg",frame/10);
  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;

  delete [] screenshotData;
}

void displayFunc()
{

  // render some stuff...
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.LoadIdentity();

  //camera of frame i, stays at cameraPosition[i] and center of the view should be
  // cameraPosition[i] + cameraDirection[i]
  if(frame<cameraPositionSize)
  matrix.LookAt(cameraPosition[frame].x, cameraPosition[frame].y, cameraPosition[frame].z, 
                  cameraPosition[frame].x+cameraDirection[frame].x, 
                  cameraPosition[frame].y+cameraDirection[frame].y, 
                  cameraPosition[frame].z+cameraDirection[frame].z, 
                cameraNormal[frame].x, cameraNormal[frame].y, cameraNormal[frame].z);
  else  matrix.LookAt(cameraPosition[0].x, cameraPosition[0].y, cameraPosition[0].z, 
                  cameraPosition[0].x+cameraDirection[0].x, 
                  cameraPosition[0].y+cameraDirection[0].y, 
                  cameraPosition[0].z+cameraDirection[0].z, 
                cameraNormal[0].x, cameraNormal[0].y, cameraNormal[0].z);

  //get model view matrix
  float m[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.Translate(landTranslate[0],landTranslate[1],landTranslate[2]); //translate movement
  //rotation movement, each for an axis
  matrix.Rotate(landRotate[0],1,0,0);
  matrix.Rotate(landRotate[1],0,1,0);
  matrix.Rotate(landRotate[2],0,0,1);
  //scaling transformation
  matrix.Scale(landScale[0],landScale[1],landScale[2]);
  matrix.GetMatrix(m);

  //get projection matrix
  float p[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(p);

  // bind ground texture shader
  texPipelineProgram->Bind();
  // set variable
  texPipelineProgram->SetModelViewMatrix(m);
  texPipelineProgram->SetProjectionMatrix(p);
  //bind texture vao
  glBindVertexArray(groundVertexArray);
  glActiveTexture(GL_TEXTURE0); 
  GLint h_textureImage = glGetUniformLocation(texPipelineProgram->GetProgramHandle(), "texCoord");
  glUniform1i(h_textureImage,0);
  glBindTexture(GL_TEXTURE_2D, texHandle); 
  //draw ground and sky texture
  glDrawArrays(GL_TRIANGLES, 0, texSize);


  //if the rail is set to be visible then render
  if (railIsVisible == 1){
  // bind rail vertex shader
  mainPipelineProgram->Bind();
  // set variable
  GLuint program = mainPipelineProgram->GetProgramHandle();//get program handle
  
  //set viewLightDirection
  GLint h_viewLightDirection = glGetUniformLocation(program, "viewLightDirection");
  float view[16];
  matrix.GetMatrix(view); // read the view matrix
  glm::vec4 lightDirection = glm::vec4(0,1,0,0); // the “Sun” at noon
  float viewLightDirection[3]; // light direction in the view space
  // the following line is pseudo-code:
  viewLightDirection[0] = (glm::make_mat4(view)* lightDirection).x;
  viewLightDirection[1] = (glm::make_mat4(view)* lightDirection).y;
  viewLightDirection[2] = (glm::make_mat4(view)* lightDirection).z;
  // upload viewLightDirection to the GPU
  glUniform3fv(h_viewLightDirection, 1, viewLightDirection);

  //set normalMatrix
  GLint h_normalMatrix = glGetUniformLocation(program, "normalMatrix");
  float n[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetNormalMatrix(n); // get normal matrix
  // upload n to the GPU
  GLboolean isRowMajor = GL_FALSE;
  glUniformMatrix4fv(h_normalMatrix, 1, isRowMajor, n);
   
  //draw the sky-box and the ground
  mainPipelineProgram->SetModelViewMatrix(m);
  mainPipelineProgram->SetProjectionMatrix(p);
  glBindVertexArray(vertexArray);
  for(int i =0; i<4; i++){
      glDrawArrays(GL_TRIANGLE_STRIP, i*curveSize*2, curveSize*2);
  }
  }
  //take a screen shot every 10 frames, until frame 2000.
  // if(frame < 2000 && frame%10==0) saveScreenshot();

  frame++;
  glutSwapBuffers();
}

void idleFunc()
{
  // do some stuff... 

  // for example, here, you can save the screenshots to disk (to make the animation)

  // make the screen update 
  glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
  glViewport(0, 0, w, h);

  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.LoadIdentity();
  matrix.Perspective(54.0f, (float)w / (float)h, 0.01f, 100.0f);
}

void mouseMotionDragFunc(int x, int y)
{
  // mouse has moved and one of the mouse buttons is pressed (dragging)

  // the change in mouse position since the last invocation of this function
  int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };
   
   
  switch (controlState)
  {
    // translate the landscape
    case TRANSLATE:
      if (leftMouseButton)
      {
        // control x,y translation via the left mouse button
        landTranslate[0] += mousePosDelta[0] * 0.01f;
        landTranslate[1] -= mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z translation via the middle mouse button
        landTranslate[2] += mousePosDelta[1] * 0.01f;
      }
      break;

    // rotate the landscape
    case ROTATE:
      if (leftMouseButton)
      {
        // control x,y rotation via the left mouse button
        landRotate[0] += mousePosDelta[1];
        landRotate[1] += mousePosDelta[0];
      }
      if (middleMouseButton)
      {
        // control z rotation via the middle mouse button
        landRotate[2] += mousePosDelta[1];
      }
      break;

    // scale the landscape
    case SCALE:
      if (leftMouseButton)
      {
        // control x,y scaling via the left mouse button
        landScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
        landScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z scaling via the middle mouse button
        landScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
  // mouse has moved
  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
  // a mouse button has has been pressed or depressed

  // keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables
  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      leftMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_MIDDLE_BUTTON:
      middleMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_RIGHT_BUTTON:
      rightMouseButton = (state == GLUT_DOWN);
    break;
  }

  // keep track of whether CTRL and SHIFT keys are pressed
  switch (glutGetModifiers())
  {


    case GLUT_ACTIVE_SHIFT:
      translateMode = 0;
      controlState = SCALE;
    break;

    // if CTRL and SHIFT are not pressed, we are in rotate mode
    //since CTRL is not working, I use 't' instead for toggling tranlation mode.
    default:
      if (translateMode == 0) controlState = ROTATE;
      else if (translateMode == 1) controlState = TRANSLATE;
    break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
  switch (key)
  {
    case 27: // ESC key
      exit(0); // exit the program
    break;

    case ' ':
      cout << "You pressed the spacebar." << endl;
    break;

    case 'x':
      // take a screenshot
      saveScreenshot();
    break;

    case '1':
      printf("drawingMode is 1, draw dots.\n");
      drawingMode = '1';
    break;

    case '2':
      printf("drawingMode is 2, draw grids.\n");
      drawingMode = '2';
    break;

    case '3':
      printf("drawingMode is 3, draw triangels.\n");
      drawingMode = '3';
    break;

    case 't':
      translateMode = translateMode == 0 ? 1 : 0;
    break;

    case 'v':
      railIsVisible = railIsVisible == 0 ? 1 : 0;
    break;
  }
}

//load rail splines from track.txt file
int loadSplines(char * argv) 
{
  char * cName = (char *) malloc(128 * sizeof(char));
  FILE * fileList;
  FILE * fileSpline;
  int iType, i = 0, j, iLength;

  // load the track file 
  fileList = fopen(argv, "r");
  if (fileList == NULL) 
  {
    printf ("can't open file\n");
    exit(1);
  }
  
  // stores the number of splines in a global variable 
  fscanf(fileList, "%d", &numSplines);

  splines = (Spline*) malloc(numSplines * sizeof(Spline));

  // reads through the spline files 
  for (j = 0; j < numSplines; j++) 
  {
    i = 0;
    fscanf(fileList, "%s", cName);
    fileSpline = fopen(cName, "r");

    if (fileSpline == NULL) 
    {
      printf ("can't open file\n");
      exit(1);
    }

    // gets length for spline file
    fscanf(fileSpline, "%d %d", &iLength, &iType);

    // allocate memory for all the points
    splines[j].points = (Point *)malloc(iLength * sizeof(Point));
    splines[j].numControlPoints = iLength;

    // saves the data to the struct
    while (fscanf(fileSpline, "%lf %lf %lf", 
	   &splines[j].points[i].x, 
	   &splines[j].points[i].y, 
	   &splines[j].points[i].z) != EOF) 
    {
      i++;
    }
  }

  free(cName);

  //get the position range of the spline
  xMin = 99999; yMin = 99999; zMin = 99999;
  xMax = -99999; yMax = -99999; zMax = -99999;
  for(int i=0; i<splines[0].numControlPoints; i++){
    Point p = splines[0].points[i];
    if(p.x > xMax) xMax = p.x;
    if (p.x < xMin) xMin = p.x;
    if(p.y > yMax) yMax = p.y;
    if (p.y < yMin) yMin = p.y;
    if(p.z > zMax) zMax = p.z;
    if (p.z < zMin) zMin = p.z;
  }
  xMin-=cameraHeightonRail*5; xMax+=cameraHeightonRail*5;
  yMin-=cameraHeightonRail*5; yMax+=cameraHeightonRail*5;
  zMin-=cameraHeightonRail*5; zMax+=cameraHeightonRail*5;
  xLength = xMax - xMin;
  yLength = yMax - yMin;
  zLength = zMax - zMin;
  return 0;
}

//construct ground and sky-box vertices and the texture shader
int initGroundTexture(const char * imageFilename, GLuint textureHandle)
{

  // read the texture image
  ImageIO img;
  ImageIO::fileFormatType imgFormat;
  ImageIO::errorType err = img.load(imageFilename, &imgFormat);
  if (err != ImageIO::OK) 
  {
    printf("Loading texture from %s failed.\n", imageFilename);
    return -1;
  }

  // check that the number of bytes is a multiple of 4
  if (img.getWidth() * img.getBytesPerPixel() % 4) 
  {
    printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
    return -1;
  }

  // allocate space for an array of pixels
  int width = img.getWidth();
  int height = img.getHeight();
  unsigned char * pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

  // fill the pixelsRGBA array with the image pixels
  memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
  for (int h = 0; h < height; h++)
    for (int w = 0; w < width; w++) 
    {
      // assign some default byte values (for the case where img.getBytesPerPixel() < 4)
      pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
      pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
      pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
      pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque

      // set the RGBA channels, based on the loaded image
      int numChannels = img.getBytesPerPixel();
      for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
        pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
    }

  // bind the texture
  glBindTexture(GL_TEXTURE_2D, textureHandle);

  // initialize the texture
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

  // generate the mipmaps for this texture
  glGenerateMipmap(GL_TEXTURE_2D);

  // set the texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // query support for anisotropic texture filtering
  GLfloat fLargest;
  glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
  printf("Max available anisotropic samples: %f\n", fLargest);
  // set anisotropic texture filtering
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

  // query for any errors
  GLenum errCode = glGetError();
  if (errCode != 0) 
  {
    printf("Texture initialization error. Error code: %d.\n", errCode);
    return -1;
  }
  // de-allocate the pixel array -- it is no longer needed
  delete [] pixelsRGBA;
  
  //generate position vertices and texture coordinates for the ground and the skybox
  texSize = 36; //vbo consists of 36 points
  glm::vec3* position = (glm::vec3*) malloc(texSize * sizeof(glm::vec3));
  glm::vec2* texCoord = (glm::vec2*) malloc(texSize * sizeof(glm::vec2));
  //back and front surfaces of the skybox
  position[0] = glm::vec3(xMin,yMin,zMin); position[1] = glm::vec3(xMin,yMax,zMin); position[2] = glm::vec3(xMax,yMax,zMin);
  position[3] = glm::vec3(xMax,yMax,zMin); position[4] = glm::vec3(xMax,yMin,zMin); position[5] = glm::vec3(xMin,yMin,zMin);
  position[6] = glm::vec3(xMin,yMin,zMax); position[7] = glm::vec3(xMin,yMax,zMax); position[8] = glm::vec3(xMax,yMax,zMax);
  position[9] = glm::vec3(xMax,yMax,zMax); position[10] = glm::vec3(xMax,yMin,zMax); position[11] = glm::vec3(xMin,yMin,zMax);
  //left and right surfaces of the box
  position[12] = glm::vec3(xMin,yMin,zMin); position[13] = glm::vec3(xMin,yMax,zMin); position[14] = glm::vec3(xMin,yMax,zMax);
  position[15] = glm::vec3(xMin,yMax,zMax); position[16] = glm::vec3(xMin,yMin,zMax); position[17] = glm::vec3(xMin,yMin,zMin);
  position[18] = glm::vec3(xMax,yMin,zMin); position[19] = glm::vec3(xMax,yMax,zMin); position[20] = glm::vec3(xMax,yMax,zMax);
  position[21] = glm::vec3(xMax,yMax,zMax); position[22] = glm::vec3(xMax,yMin,zMax); position[23] = glm::vec3(xMax,yMin,zMin);
  //up and down surfaces of the box
  position[24] = glm::vec3(xMin,yMin,zMin); position[25] = glm::vec3(xMax,yMin,zMin); position[26] = glm::vec3(xMax,yMin,zMax);
  position[27] = glm::vec3(xMax,yMin,zMax); position[28] = glm::vec3(xMin,yMin,zMax); position[29] = glm::vec3(xMin,yMin,zMin);
  position[30] = glm::vec3(xMin,yMax,zMin); position[31] = glm::vec3(xMax,yMax,zMin); position[32] = glm::vec3(xMax,yMax,zMax);
  position[33] = glm::vec3(xMax,yMax,zMax); position[34] = glm::vec3(xMin,yMax,zMax); position[35] = glm::vec3(xMin,yMax,zMin);
  //use the same texture picture for all six surfaces
  for (int i = 0; i<6; i++){
    texCoord[i*6]=glm::vec2(0,0); texCoord[i*6+1]=glm::vec2(0,1); texCoord[i*6+2]=glm::vec2(1,1);
    texCoord[i*6+3]=glm::vec2(1,1); texCoord[i*6+4]=glm::vec2(1,0); texCoord[i*6+5]=glm::vec2(0,0);
  }
  
  //start a pipeline for texture shader
  texPipelineProgram = new BasicPipelineProgram;
  int ret = texPipelineProgram->Init(texShaderBasePath);
  if (ret != 0) abort();
  texPipelineProgram->Bind();

  //initialization of handles
  glGenBuffers(1, &texVertexBuffer);
  glGenBuffers(1, &groundVertexBuffer);
  glGenVertexArrays(1, &groundVertexArray);

  glBindBuffer(GL_ARRAY_BUFFER, texVertexBuffer); //upload texture coordinates to texture vbo
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * texSize, texCoord, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, groundVertexBuffer); //upload real world positions to vertex vbo
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * texSize, position, GL_STATIC_DRAW);

  glBindVertexArray(groundVertexArray); //bind the vao

    glBindBuffer(GL_ARRAY_BUFFER, texVertexBuffer); 
      GLuint loc = glGetAttribLocation(texPipelineProgram->GetProgramHandle(), "texCoord");
      glEnableVertexAttribArray(loc);
      glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, (const void *)0);

    glBindBuffer(GL_ARRAY_BUFFER, groundVertexBuffer); 
      loc = glGetAttribLocation(texPipelineProgram->GetProgramHandle(), "position");
      glEnableVertexAttribArray(loc);
      glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
    glEnable(GL_DEPTH_TEST);
  glBindVertexArray(0);

  free(position);
  free(texCoord);
  return 0;
}

//generate a CatMull curve given the control points
//pointPerLines indicates number of curve points between two control points
//position_ptr a pointer to the CatMull curve, is a pointer to a list of vec3
//similarly, tengnet_ptr stores tengent info for each curve point, so does normal_ptr and binormal_ptr
//iniVec indicates the very first normal vector
int generateCurveInfo(int pointPerLine, glm::vec3** position_ptr, glm::vec3** tangent_ptr,
                           glm::vec3** normal_ptr, glm::vec3** binormal_ptr, glm::vec3 initVec){
  int numControlPoints = splines[0].numControlPoints;
  int size = pointPerLine*(numControlPoints-3);
  *position_ptr = (glm::vec3*) malloc(size * sizeof(glm::vec3)); 
  *tangent_ptr = (glm::vec3*) malloc(size * sizeof(glm::vec3)); 
  *normal_ptr = (glm::vec3*) malloc(size * sizeof(glm::vec3)); 
  *binormal_ptr = (glm::vec3*) malloc(size * sizeof(glm::vec3)); 
  for(int i=0; i<numControlPoints-3; i++){
    Point p1 = splines[0].points[i];
    Point p2 = splines[0].points[i+1];
    Point p3 = splines[0].points[i+2];
    Point p4 = splines[0].points[i+3];
    for(int j=0; j<pointPerLine; j++){
      float u = 1 / (float) pointPerLine * j;
      glm::vec4 uVec = glm::vec4(pow(u,3), pow(u,2), u, 1);
      glm::vec4 duVec = glm::vec4(3*pow(u,2), 2*u,1,0);
      glm::mat3x4 controlMat = glm::mat3x4( p1.x,p2.x,p3.x,p4.x,
                                            p1.y,p2.y,p3.y,p4.y,
                                            p1.z,p2.z,p3.z,p4.z);

      int index= i*pointPerLine+j;
      (*position_ptr)[index] = uVec * CatMullBasis * controlMat;
      (*tangent_ptr)[index]= normalize(duVec * CatMullBasis * controlMat);
      if(index == 0) {
        (*normal_ptr)[0] = normalize(  cross( (*tangent_ptr)[0], initVec )  );
        (*binormal_ptr)[0] = normalize(  cross( (*tangent_ptr)[0], (*normal_ptr)[0] )   );
      } else {
        (*normal_ptr)[index] = normalize(  cross( (*binormal_ptr)[index-1], (*tangent_ptr)[index] )  );
        (*binormal_ptr)[index] = normalize(  cross( (*tangent_ptr)[index], (*normal_ptr)[index] )   );
      }
      
    }
  }
  return size;
}

//generate a rectangular shaped rail
//stores triangular vbo vertices in *railPosition_ptr, colors in *railColor_ptr
int generateRail(int pointPerLine, float radius, glm::vec3** railPosition_ptr, glm::vec4** railColor_ptr){
  glm::vec3* curvePosition;
  glm::vec3* curveTangent;
  glm::vec3* curveNormal;
  glm::vec3* curveBinormal;
  //generate a CatMull curve according to the current spline
  int curveSize = generateCurveInfo(20, &curvePosition, &curveTangent, &curveNormal, &curveBinormal,
                                    glm::vec3(1,0,0));
  //the logic is to render 4 trangle strip
  //Therefore, 5 times as many of the points are needed
  int size = curveSize * 5;
  (*railPosition_ptr) = (glm::vec3*) malloc(size * sizeof(glm::vec3)); 
  (*railColor_ptr) = (glm::vec4*) malloc(curveSize * 4 * sizeof(glm::vec4)); 
  for(int i=0; i<curveSize; i++){
    //first 4 arrays for each edge of the rail
    (*railPosition_ptr)[i] = curvePosition[i] + radius * normalize( curveNormal[i] + curveBinormal[i]) 
                             + cameraHeightonRail*curveNormal[i];
    (*railPosition_ptr)[i+curveSize] = curvePosition[i] + radius * normalize( curveNormal[i] - curveBinormal[i] )
                                       + cameraHeightonRail*curveNormal[i];
    (*railPosition_ptr)[i+2*curveSize] = curvePosition[i] + radius * normalize( - curveNormal[i] - curveBinormal[i] )
                                         + cameraHeightonRail*curveNormal[i];
    (*railPosition_ptr)[i+3*curveSize] = curvePosition[i] + radius * normalize( - curveNormal[i] + curveBinormal[i] )
                                         + cameraHeightonRail*curveNormal[i];
    //the last array to make closure of the strips
    (*railPosition_ptr)[i+4*curveSize] = (*railPosition_ptr)[i];

    //assign each surface its normal vector
    (*railColor_ptr)[i] = glm::vec4(curveNormal[i],1);
    (*railColor_ptr)[i+curveSize] = glm::vec4(-curveBinormal[i],1);
    (*railColor_ptr)[i+curveSize*2] = glm::vec4(-curveNormal[i],1);
    (*railColor_ptr)[i+curveSize*3] = glm::vec4(curveBinormal[i],1);
  }
    
  free(curvePosition);
  free(curveNormal);
  free(curveBinormal);
  free(curveTangent);

  return curveSize;
}


void initScene(int argc, char *argv[])
{

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  
  //generate a rectangular shaped rail
  curveSize = generateRail(5, 0.1, &railPosition, &railColor);
  //generate the curve of camera movement
  cameraPositionSize = generateCurveInfo(250, &cameraPosition, &cameraDirection,
                                        &cameraNormal, &cameraBinormal, glm::vec3(-1,0,0));

 //generate triangular order of position and color arrays
  numDrawingPoints = 4*curveSize*2; //all pixels are used twice expect the first and last row.
  glm::vec3* position = (glm::vec3*) malloc(numDrawingPoints * sizeof(glm::vec3)); 
  glm::vec4* color = (glm::vec4*) malloc(numDrawingPoints * sizeof(glm::vec4)); 
  //the logic is to render the img with height-1 strips of GL_TRIANGLE_STRIP
  for(int i=0; i<4; i++){
    for(int j=0; j<curveSize; j++){
      int x = curveSize*2*i+j*2;
      position[x] = railPosition[i*curveSize+j]; 
      position[x+1] = railPosition[i*curveSize+j+curveSize];
      color[x] = railColor[i*curveSize+j];
      color[x+1] = color[x];
    }
  }

  //start a pipeline to render the rail
  mainPipelineProgram = new BasicPipelineProgram;
  int ret = mainPipelineProgram->Init(mainShaderBasePath);
  if (ret != 0) abort();

  mainPipelineProgram->Bind();
  //initialize the handles
  glGenBuffers(1, &vertexBuffer);
  glGenBuffers(1, &colorVertexBuffer);
  glGenVertexArrays(1, &vertexArray);

  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);//upload positions to vbo
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * numDrawingPoints, position, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, colorVertexBuffer);//upload colors to vbo
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * numDrawingPoints, color, GL_STATIC_DRAW);

  //create a vao
  glBindVertexArray(vertexArray);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
      GLuint loc =
          glGetAttribLocation(mainPipelineProgram->GetProgramHandle(), "position");
      glEnableVertexAttribArray(loc);
      glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

    glBindBuffer(GL_ARRAY_BUFFER, colorVertexBuffer);
      loc = glGetAttribLocation(mainPipelineProgram->GetProgramHandle(), "normal");
      glEnableVertexAttribArray(loc);
      glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void *)0);

    glEnable(GL_DEPTH_TEST);

    std::cout << "GL error: " << glGetError() << std::endl;

  glBindVertexArray(0);


  free(position);
  free(color);
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    cout << "The arguments are incorrect." << endl;
    cout << "usage: ./hw1 <heightmap file>" << endl;
    exit(EXIT_FAILURE);
  }

  cout << "Initializing GLUT..." << endl;
  glutInit(&argc,argv);

  cout << "Initializing OpenGL..." << endl;

  #ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);  
  glutCreateWindow(windowTitle);

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

  #ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(windowWidth - 1, windowHeight - 1);
  #endif

  // tells glut to use a particular display function to redraw 
  glutDisplayFunc(displayFunc);
  // perform animation inside idleFunc
  glutIdleFunc(idleFunc);
  // callback for mouse drags
  glutMotionFunc(mouseMotionDragFunc);
  // callback for idle mouse movement
  glutPassiveMotionFunc(mouseMotionFunc);
  // callback for mouse button changes
  glutMouseFunc(mouseButtonFunc);
  // callback for resizing the window
  glutReshapeFunc(reshapeFunc);
  // callback for pressing the keys on the keyboard
  glutKeyboardFunc(keyboardFunc);

  // init glew
  #ifdef __APPLE__
    // nothing is needed on Apple
  #else
    // Windows, Linux
    GLint result = glewInit();
    if (result != GLEW_OK)
    {
      cout << "error: " << glewGetErrorString(result) << endl;
      exit(EXIT_FAILURE);
    }
  #endif

  
  // load the splines from the provided filename
  loadSplines(argv[1]);
  printf("Loaded %d spline(s).\n", numSplines);
  for(int i=0; i<numSplines; i++)
    printf("Num control points in spline %d: %d.\n", i, splines[i].numControlPoints);

  //generate texture for the ground and the skybox
  glGenTextures(1, &texHandle);
  int rc = initGroundTexture("tex.jpg",texHandle);
  if (rc != 0)
  {
    printf("Error loading the texture image.\n");
    exit(EXIT_FAILURE);
  }

  // initialization of the rail
  initScene(argc, argv);
  free(splines);

  // sink forever into the glut loop
  glutMainLoop();
}


