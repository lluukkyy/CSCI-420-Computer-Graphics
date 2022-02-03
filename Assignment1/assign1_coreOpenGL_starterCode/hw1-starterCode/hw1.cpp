/*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields with Shaders.
  C++ starter code

  Student username: <haoyunzh>
*/

#include "basicPipelineProgram.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "openGLHeader.h"
#include "glutHeader.h"

#include <iostream>
#include <cstring>
#include <math.h>

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
  char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

using namespace std;

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
char windowTitle[512] = "CSCI 420 homework I";

ImageIO * heightmapImage;

//three vertex buffer. Namely, the normal one, the transposed one, the triangular one.
GLuint triVertexBuffer[3]; 
//corresponding smoothened vertex buffer
GLuint smoothVertexBuffer[3];
//three color buffers and the smoothened ones
GLuint triColorVertexBuffer[3];
GLuint smoothColorVertexBuffer[3];

GLuint triVertexArray[3];

// initialize some public variables
unsigned int width; // pixel width of the img
unsigned int height; // pixel hieght of the img

//following variables are used to record view movement.
// The reason why I don't control drawing functions directly through key inputs is to
// split the model and controller part of the program. Drawing functions below are 
// for models and detecting key inputs is the job for the controller.
unsigned char drawingMode = 0; // drawing mode in glDrawArray
int smoothMode = 0; //toggle for smooth mode
//toggle for TRANSLATE movement due to the malfunction of detecting CTRL input on macOS
int translateMode = 0; 

OpenGLMatrix matrix;
BasicPipelineProgram * pipelineProgram;

// write a screenshot to the specified filename
void saveScreenshot(const char * filename)
{
  unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;

  delete [] screenshotData;
}

//a basic fuction to draw dots at specified positions
void drawDots(){
  glBindVertexArray(triVertexArray[0]);
  glDrawArrays(GL_POINTS, 0, width*height);
}

//draw grids connecting dots
//the logic is to draw twice: once for horizontal lines, another for vertical lines
void drawGrids(){
  //draw lines horizontally
  glBindVertexArray(triVertexArray[0]);
  for(int i =0; i<height; i++){
      glDrawArrays(GL_LINE_STRIP, i*width, width);
  }
 
  //draw lines
  glBindVertexArray(triVertexArray[1]);
  for(int i =0; i<width; i++){
      glDrawArrays(GL_LINE_STRIP, i*height, height);
  }
}

//draw height-1 number of triangle strips to generate a whole terrain
void drawTriangles(){
  glBindVertexArray(triVertexArray[2]);
  for(int i =0; i<height-1; i++){
      glDrawArrays(GL_TRIANGLE_STRIP, i*width*2, width*2);
  }
}

void displayFunc()
{
  // render some stuff...
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.LoadIdentity();
  matrix.LookAt(0.5, -0.5, 0.75, 0.5, 0.5, 0, 0, 1, 0);

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

  float p[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(p);

  //
  // bind shader
  pipelineProgram->Bind();

  // set variable
  pipelineProgram->SetModelViewMatrix(m);
  pipelineProgram->SetProjectionMatrix(p);

  GLuint program = pipelineProgram->GetProgramHandle();
  GLint h_mode = glGetUniformLocation(program,  "mode"); 

  //use different drawing functions according to drawingModes
  switch (drawingMode)
  {
    case '1':
      glUniform1i(h_mode,smoothMode);
      drawDots();
    break;
    
    case '2':
      glUniform1i(h_mode,smoothMode);
      drawGrids();
    break;

    case '3':
      glUniform1i(h_mode,smoothMode);
      drawTriangles();
    break;
  }

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
      saveScreenshot("screenshot.jpg");
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

    case '4':
      printf("Smooth switch.\n");
      smoothMode = smoothMode == 0 ? 1 : 0;
    break;

    case 't':
      translateMode = translateMode == 0 ? 1 : 0;
    break;
  }
}


//repeated binding process
//binding posiiton and color including the smoothened version to a vertex buffer, and submit it to a vertex array
void binding(glm::vec3* position, glm::vec3* position_s, glm::vec4* color, glm::vec4* color_s, int bufferIndex, int arraySize){
  glBindBuffer(GL_ARRAY_BUFFER, triVertexBuffer[bufferIndex]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * arraySize, position, GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, smoothVertexBuffer[bufferIndex]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * arraySize, position_s, GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, triColorVertexBuffer[bufferIndex]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * arraySize, color, GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, smoothColorVertexBuffer[bufferIndex]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * arraySize, color_s, GL_STATIC_DRAW);
 
  pipelineProgram = new BasicPipelineProgram;
  int ret = pipelineProgram->Init(shaderBasePath);
  if (ret != 0) abort();

  glBindVertexArray(triVertexArray[bufferIndex]);
  glBindBuffer(GL_ARRAY_BUFFER, triVertexBuffer[bufferIndex]);
  GLuint loc =
      glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

  glBindVertexArray(triVertexArray[bufferIndex]);
  glBindBuffer(GL_ARRAY_BUFFER, smoothVertexBuffer[bufferIndex]);
  loc =
      glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position_smooth");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

  glBindBuffer(GL_ARRAY_BUFFER, triColorVertexBuffer[bufferIndex]);
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void *)0);

  glBindBuffer(GL_ARRAY_BUFFER, smoothColorVertexBuffer[bufferIndex]);
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color_smooth");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void *)0);

  glEnable(GL_DEPTH_TEST);

  std::cout << "GL error: " << glGetError() << std::endl;
}

void initScene(int argc, char *argv[])
{
  // load the image from a jpeg disk file to main memory
  heightmapImage = new ImageIO();
  if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
  {
    cout << "Error reading image " << argv[1] << "." << endl;
    exit(EXIT_FAILURE);
  }

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  // modify the following code accordingly

  //get size information form the img
  height = heightmapImage->getHeight();
  width = heightmapImage->getWidth();  
  

  //generate two 2d-arrays, storing position and color information
  unsigned int size = height*width;
  glm::vec3* position = (glm::vec3*) malloc(size * sizeof(glm::vec3)); 
  glm::vec4* color = (glm::vec4*) malloc(size * sizeof(glm::vec4)); 
  for(int i=0;i<height;i++){
    for(int j=0;j<width;j++){
      float gs = (float) heightmapImage->getPixel(i,j,0) / 255.0f *2; // get greyscale of a pixel
      color[i*width+j] = glm::vec4(gs,gs,gs,1); //set color of the pixel
      position[i*width+j] = glm::vec3( ((float) i)/width,((float) j)/height,gs/5); //assign z value as the greyscale of the pixel
    }
  }

  float eps = 0.00001;
  glm::vec3* position_smooth = (glm::vec3*) malloc(size * sizeof(glm::vec3)); 
  glm::vec4* color_smooth = (glm::vec4*) malloc(size * sizeof(glm::vec4)); 
  for(int i=0;i<size;i++){
    int row_head = floor(i/width)*width;
    int row_tail = row_head+width-1;
    int index_0 = i-width > 0 ? i-width : i+width;
    int index_3 = i+1 < row_tail ? i+1 : i-1;
    int index_6 = i+width < size-1 ? i+width : i-width;
    int index_9 = i-1 > row_head ? i-1 : i+1;
    position_smooth[i] = (position[index_0]+position[index_3]+position[index_6]+position[index_9])/4.0f;
    float z = position[i].z;
    color_smooth[i] = color[i] / (z > eps ? z : eps) * position_smooth[i].z;
  }
  

  //generate transposed position and color
  glm::vec3* position_t = (glm::vec3*) malloc(size * sizeof(glm::vec3)); 
  glm::vec4* color_t = (glm::vec4*) malloc(size * sizeof(glm::vec4)); 
  glm::vec3* position_t_smooth = (glm::vec3*) malloc(size * sizeof(glm::vec3)); 
  glm::vec4* color_t_smooth = (glm::vec4*) malloc(size * sizeof(glm::vec4)); 
  for(int i=0; i<height; i++){
    for(int j=0; j<width; j++){
      position_t[height*j+i] = position[i*width+j];
      color_t[height*j+i] = color[i*width+j];
      position_t_smooth[height*j+i] = position_smooth[i*width+j];
      color_t_smooth[height*j+i] = color_smooth[i*width+j];
    }
  }

  //generate triangular order of position and color arrays
  size = (height-1)*width*2; //all pixels are used twice expect the first and last row.
  glm::vec3* position_tri = (glm::vec3*) malloc(size * sizeof(glm::vec3)); 
  glm::vec4* color_tri = (glm::vec4*) malloc(size * sizeof(glm::vec4)); 
   glm::vec3* position_tri_smooth = (glm::vec3*) malloc(size * sizeof(glm::vec3)); 
  glm::vec4* color_tri_smooth = (glm::vec4*) malloc(size * sizeof(glm::vec4)); 
  //the logic is to render the img with height-1 strips of GL_TRIANGLE_STRIP
  for(int i=0; i<height-1; i++){
    for(int j=0; j<width; j++){
      int x = width*2*i+j*2;
      position_tri[x] = position[i*width+j]; position_tri[x+1] = position[i*width+j+width];
      color_tri[x] = color[i*width+j]; color_tri[x+1] = color[i*width+j+width];
      position_tri_smooth[x] = position_smooth[i*width+j]; position_tri_smooth[x+1] = position_smooth[i*width+j+width];
      color_tri_smooth[x] = color_smooth[i*width+j]; color_tri_smooth[x+1] = color_smooth[i*width+j+width];
    }
  }

  //generate three buffers, each one for normal, transposed and triangular order of the array.
  glGenBuffers(3, triVertexBuffer);
  glGenBuffers(3, smoothVertexBuffer);
  glGenBuffers(3, triColorVertexBuffer);
  glGenBuffers(3, smoothColorVertexBuffer);
  glGenVertexArrays(3, triVertexArray);

  //bind those position-color pairs each to a vertex buffer, and submit them to a vertex array. 
  binding(position, position_smooth, color, color_smooth, 0, height*width);
  binding(position_t, position_t_smooth, color_t,color_t_smooth, 1, height*width);
  binding(position_tri, position_tri_smooth, color_tri, color_tri_smooth, 2, (height-1)*width*2);

  free(position); free(position_smooth);
  free(color); free(color_smooth);

  free(position_t); free(position_t_smooth);
  free(color_t); free(color_t_smooth);

  free(position_tri); free(position_tri_smooth);
  free(color_tri); free(color_tri_smooth);

  
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

  // do initialization
  initScene(argc, argv);

  // sink forever into the glut loop
  glutMainLoop();
}


