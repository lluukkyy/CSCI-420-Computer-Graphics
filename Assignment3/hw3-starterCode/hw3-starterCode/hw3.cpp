/* **************************
 * CSCI 420
 * Assignment 3 Raytracer
 * Name: <Your name here>
 * *************************
*/

#ifdef WIN32
  #include <windows.h>
#endif

#if defined(WIN32) || defined(linux)
  #include <GL/gl.h>
  #include <GL/glut.h>
#elif defined(__APPLE__)
  #include <OpenGL/gl.h>
  #include <GLUT/glut.h>
#endif

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <typeinfo>
#ifdef WIN32
  #define strcasecmp _stricmp
#endif

#include <imageIO.h>

#define MAX_TRIANGLES 20000
#define MAX_SPHERES 100
#define MAX_LIGHTS 100

char * filename = NULL;

//different display modes
#define MODE_DISPLAY 1
#define MODE_JPEG 2

int mode = MODE_DISPLAY;

//you may want to make these smaller for debugging purposes
#define WIDTH 800
#define HEIGHT 600

//the field of view of the camera
#define fov 60.0

#define EPSILON 0.001
#define LARGE_NUM 999999


double aspectRatio = (double) WIDTH / (double) HEIGHT; //compute the aspect ratio
double tanHalfFov = tan( fov * ( M_PI / 180.0 ) / 2.0); //compute the tangent of 1/2*fov
glm::vec3* rays; //array containing directions of rays from the camera
float buffer[HEIGHT][WIDTH][3];
enum geoType {TRIANGLE, SPHERE, NONE}; //tags for geometry types

//structs-----------------------------------------------------------------------------------------------
struct Vertex
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double normal[3];
  double shininess;
};

struct Triangle
{
  Vertex v[3];
};

struct Sphere
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double shininess;
  double radius;
};

struct Light
{
  double position[3];
  double color[3];
};

//structure for a hitting event
struct Hit{
  //records if the light hits a sphere or a triangle, set a pointer to the object
  Sphere* sphere_ptr;
  Triangle* triangle_ptr;
  enum geoType objectType;
  glm::vec3 hittingPoint;//the exact position of the hitting point
  //If the light hits a triangle, also stores the interpolation coordinate for the hitting point.
  glm::vec3 interpolation;
  //direction of incoming ray
  glm::vec3 incomingRay;
};


//starter codes for IO (parsing input file and plotting)------------------------------------------------------
Triangle triangles[MAX_TRIANGLES];
Sphere spheres[MAX_SPHERES];
Light lights[MAX_LIGHTS];
double ambient_light[3];

int num_triangles = 0;
int num_spheres = 0;
int num_lights = 0;

void plot_pixel_display(int x,int y,float r, float g, float b);
void plot_pixel_jpeg(int x,int y,float r, float g, float b);
void plot_pixel(int x,int y,float r, float g, float b);

void draw_scene()
{
  //a simple test output
  for(unsigned int x=0; x<WIDTH; x++)
  {
    glPointSize(2.0);  
    glBegin(GL_POINTS);
    for(unsigned int y=0; y<HEIGHT; y++)
    {
      plot_pixel(x, y, buffer[y][x][0],buffer[y][x][1],buffer[y][x][2]);
    }
    glEnd();
    glFlush();
  }
  // printf("Done!\n"); fflush(stdout);
}

void plot_pixel_display(int x, int y, float r, float g, float b)
{
  glColor3f(r , g , b);
  glVertex2i(x,y);
}


void plot_pixel(int x, int y, float r, float g, float b)
{
  plot_pixel_display(x,y,r,g,b);
}

void save_jpg()
{
  printf("Saving JPEG file: %s\n", filename);
  unsigned char saveBuffer[HEIGHT][WIDTH][3];
  for(unsigned int x=0; x<HEIGHT; x++){
    for(unsigned int y=0; y<WIDTH; y++){
      for(unsigned int z=0; z<3; z++) {
        // printf("?????%f\n\n\n\n");
        saveBuffer[x][y][z] = fmax(0,fmin(buffer[x][y][z]*255,255)) ;
      }
    }
  }
  ImageIO img(WIDTH, HEIGHT, 3, &saveBuffer[0][0][0]);
  if (img.save(filename, ImageIO::FORMAT_JPEG) != ImageIO::OK)
    printf("Error in Saving\n");
  else 
    printf("File saved Successfully\n");
}

void parse_check(const char *expected, char *found)
{
  if(strcasecmp(expected,found))
  {
    printf("Expected '%s ' found '%s '\n", expected, found);
    printf("Parse error, abnormal abortion\n");
    exit(0);
  }
}

void parse_doubles(FILE* file, const char *check, double p[3])
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check(check,str);
  fscanf(file,"%lf %lf %lf",&p[0],&p[1],&p[2]);
  // printf("%s %lf %lf %lf\n",check,p[0],p[1],p[2]);
}

void parse_rad(FILE *file, double *r)
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check("rad:",str);
  fscanf(file,"%lf",r);
  // printf("rad: %f\n",*r);
}

void parse_shi(FILE *file, double *shi)
{
  char s[100];
  fscanf(file,"%s",s);
  parse_check("shi:",s);
  fscanf(file,"%lf",shi);
  // printf("shi: %f\n",*shi);
}

int loadScene(char *argv)
{
  FILE * file = fopen(argv,"r");
  int number_of_objects;
  char type[50];
  Triangle t;
  Sphere s;
  Light l;
  fscanf(file,"%i", &number_of_objects);

  printf("number of objects: %i\n",number_of_objects);

  parse_doubles(file,"amb:",ambient_light);

  for(int i=0; i<number_of_objects; i++)
  {
    fscanf(file,"%s\n",type);
    // printf("%s\n",type);
    if(strcasecmp(type,"triangle")==0)
    {
      // printf("found triangle\n");
      for(int j=0;j < 3;j++)
      {
        parse_doubles(file,"pos:",t.v[j].position);
        parse_doubles(file,"nor:",t.v[j].normal);
        parse_doubles(file,"dif:",t.v[j].color_diffuse);
        parse_doubles(file,"spe:",t.v[j].color_specular);
        parse_shi(file,&t.v[j].shininess);
      }

      if(num_triangles == MAX_TRIANGLES)
      {
        printf("too many triangles, you should increase MAX_TRIANGLES!\n");
        exit(0);
      }
      triangles[num_triangles++] = t;
    }
    else if(strcasecmp(type,"sphere")==0)
    {
      // printf("found sphere\n");

      parse_doubles(file,"pos:",s.position);
      parse_rad(file,&s.radius);
      parse_doubles(file,"dif:",s.color_diffuse);
      parse_doubles(file,"spe:",s.color_specular);
      parse_shi(file,&s.shininess);

      if(num_spheres == MAX_SPHERES)
      {
        printf("too many spheres, you should increase MAX_SPHERES!\n");
        exit(0);
      }
      spheres[num_spheres++] = s;
    }
    else if(strcasecmp(type,"light")==0)
    {
      // printf("found light\n");
      parse_doubles(file,"pos:",l.position);
      parse_doubles(file,"col:",l.color);

      if(num_lights == MAX_LIGHTS)
      {
        printf("too many lights, you should increase MAX_LIGHTS!\n");
        exit(0);
      }
      lights[num_lights++] = l;
    }
    else
    {
      printf("unknown type in scene description:\n%s\n",type);
      exit(0);
    }
  }
  return 0;
}

void idle()
{
  //hack to make it only draw once
  static int once=0;
  if(!once)
  {
    draw_scene();
    if(mode == MODE_JPEG)
      save_jpg();
  }
  once=1;
}

//student codes ----------------------------------------------------------------------------------------------------

//sphere detection----------

//detects if a ray hits a specific sphere
//input: r0 -- where the ray starts, rd -- the unit direction vector of the ray,
//       s -- the specific sphere to check, t1_ptr/t2_ptr -- stores t values for two hitting points.
bool rayHitsSphere(glm::vec3 r0, glm::vec3 rd, Sphere s, float* t1_ptr, float* t2_ptr){
  double b = 2 * ( rd.x * (r0.x - s.position[0]) + 
                   rd.y * (r0.y - s.position[1]) + 
                   rd.z * (r0.z - s.position[2]) );
  double c = pow(r0.x-s.position[0], 2) + pow(r0.y-s.position[1], 2) + pow(r0.z-s.position[2], 2) - pow(s.radius ,2);
  double delta = pow(b,2) - 4 * c;
  //calculate detla for the eqution, if it's negative, then there's no intersection
  if (delta < 0) return false;
  //solve the equation to get t values.
  *t1_ptr = (-b - sqrt(delta)) / 2;
  *t2_ptr = (-b + sqrt(delta)) / 2;
  //get rid of t values that are too small
  if(*t1_ptr < EPSILON) *t1_ptr = LARGE_NUM;
  if(*t2_ptr < EPSILON) *t2_ptr = LARGE_NUM;
  //if there's any valid intersection
  return (*t1_ptr >= EPSILON) || (*t2_ptr >= EPSILON);
}


//triangle detection----------

//calculate the double value of a triangle given three vertices a, b and c
float doubleArea(glm::vec2 a,glm::vec2 b,glm::vec2 c){
  return (b.x-a.x)*(c.y-a.y)-(c.x-a.x)*(b.y-a.y);
}

bool floatBetween0and1(float f){
  return f>=-EPSILON && f <= 1+EPSILON;
}

//check if point p is in a specific triangle t. 
//if yes, stores the interpolation coordinate in interpolation_ptr
bool pointInTriangle(glm::vec3 p, Triangle t, glm::vec3* interpolation_ptr){
  //convert three vertices into a 2d plane, namely x-y plane.
  glm::vec2 c = glm::vec2(p.x,p.y);
  glm::vec2 c0 = glm::vec2(t.v[0].position[0],t.v[0].position[1]);
  glm::vec2 c1= glm::vec2(t.v[1].position[0],t.v[1].position[1]);
  glm::vec2 c2 = glm::vec2(t.v[2].position[0],t.v[2].position[1]);
  //calculate interpolation coordinates
  float denom = doubleArea(c0,c1,c2);
  if(fabs(denom) ==0){
    glm::vec2 c = glm::vec2(p.x,p.z);
    glm::vec2 c0 = glm::vec2(t.v[0].position[0],t.v[0].position[2]);
    glm::vec2 c1= glm::vec2(t.v[1].position[0],t.v[1].position[2]);
    glm::vec2 c2 = glm::vec2(t.v[2].position[0],t.v[2].position[2]);
    denom = doubleArea(c0,c1,c2);
    if(fabs(denom) ==0){
      glm::vec2 c = glm::vec2(p.y,p.z);
      glm::vec2 c0 = glm::vec2(t.v[0].position[1],t.v[0].position[2]);
      glm::vec2 c1= glm::vec2(t.v[1].position[1],t.v[1].position[2]);
      glm::vec2 c2 = glm::vec2(t.v[2].position[1],t.v[2].position[2]);
      float denom = doubleArea(c0,c1,c2);
      if(fabs(denom) ==0 ) return glm::length(c-c0) < EPSILON;
    } 
      
  }
  float alpha = doubleArea(c,c1,c2) / denom;
  float beta = doubleArea(c0,c,c2) / denom;
  float gamma = 1 - alpha - beta;
  *interpolation_ptr = glm::vec3(alpha,beta,gamma);
  return floatBetween0and1(alpha) && floatBetween0and1(beta) && floatBetween0and1(gamma);
}

//detect if a ray hits a specific triangle
//input: r0 -- where the ray starts, rd -- the unit direction vector of the ray,
//       s -- the specific triangle to check, t_ptr -- t value for the hitting point,
//       interpolation_ptr --  interpolation coordinates.
bool rayHitsTriangle(glm::vec3 r0, glm::vec3 rd, Triangle t, float* t_ptr, glm::vec3* interpolation_ptr){
  glm::vec3 c0 = glm::vec3(t.v[0].position[0],t.v[0].position[1],t.v[0].position[2]);
  glm::vec3 c1 = glm::vec3(t.v[1].position[0],t.v[1].position[1],t.v[1].position[2]);
  glm::vec3 c2 = glm::vec3(t.v[2].position[0],t.v[2].position[1],t.v[2].position[2]);
  glm::vec3 normal = glm::normalize(glm::cross(c1-c0,c2-c0));
  float d = -glm::dot(normal, glm::vec3(t.v[0].position[0],t.v[0].position[1],t.v[0].position[2]));
  float normalDotRd = glm::dot(normal,rd);
  if(normalDotRd == 0) return false;
  *t_ptr = -(glm::dot(r0,normal) +d)/normalDotRd;
  if(*t_ptr < EPSILON) return false;
  glm::vec3 hittingPoint = r0 + *t_ptr*rd;
  return pointInTriangle(hittingPoint,t, interpolation_ptr);
}


//general intersection funcitons and shading fucntions-----------------

//search for a closest hit of a ray
//cameraPosition -- position where the ray starts, dir -- a unit direction vector for the ray
//return a hit event
Hit cameraRayHits(glm::vec3 cameraPosition, glm::vec3 dir){
  float minT = LARGE_NUM;
  Hit h = {NULL,NULL,NONE,glm::vec3(0,0,0),glm::vec3(0,0,0)};
  //keep searching for a smaller t value in following searches
  //check sphere intersection 
  for(int i=0; i<num_spheres; i++){
    float tSphere1 = LARGE_NUM;  
    float tSphere2 = LARGE_NUM;
    if( rayHitsSphere(cameraPosition, dir, spheres[i], &tSphere1, &tSphere2) ){
      h.incomingRay = dir;
      if (tSphere1 < minT) {
        minT = tSphere1; 
        h.sphere_ptr = &spheres[i];
        h.objectType = SPHERE;
      }
      if (tSphere2 < minT){
        minT = tSphere2; 
        h.sphere_ptr = &spheres[i];
        h.objectType = SPHERE;
      } 
    
    } 
      
  }
  //check triangle intersection
  for(int i=0; i<num_triangles; i++){
    float tTriangle = LARGE_NUM;
    glm::vec3 interpolation_temp;
    if( rayHitsTriangle(cameraPosition, dir, triangles[i], &tTriangle, &interpolation_temp) ){
      h.incomingRay = dir;
      if (tTriangle < minT) {
        minT = tTriangle; 
        h.triangle_ptr = &triangles[i];
        h.objectType = TRIANGLE;
        h.interpolation = interpolation_temp;
      }
    } 
  }
  //if any hits detected, calculate the exact hitting point
  if(h.objectType != NONE) h.hittingPoint = minT*dir+cameraPosition; 
  return h;
}

//check if a hit h even is lit by a light source l by simply search for
//any objects blocking in between
bool shadowRayNotBlocked2Light(Hit h, Light l){
  glm::vec3 dir = glm::normalize( glm::vec3(l.position[0],l.position[1],l.position[2]) - h.hittingPoint );
  bool rc;
  float tSphere1, tSphere2, tTriangle;
  glm::vec3 interpolation;
  //search spheres in between
  for(int i=0; i<num_spheres; i++){
    if( h.objectType == SPHERE && h.sphere_ptr == &spheres[i]) continue;
    rc = rayHitsSphere(h.hittingPoint, dir, spheres[i], &tSphere1, &tSphere2);
    if(rc) return false;
  }
  //search for triangles in between
  for(int i=0; i<num_triangles; i++){
    if( h.objectType == TRIANGLE && h.triangle_ptr == &triangles[i]) continue;
    rc = rayHitsTriangle(h.hittingPoint, dir, triangles[i], &tTriangle, &interpolation);
    if(rc) return false;
  }
  //if no objects found, return true
  return true;
}

//get a normal vector for a hitting event
//if the object is a triangle, calculate an interpolated normal
glm::vec3 getNormOfObjectAtHit(Hit h){
  if ( h.objectType == SPHERE) {
    return glm::normalize( h.hittingPoint - 
      glm::vec3(h.sphere_ptr->position[0],h.sphere_ptr->position[1],h.sphere_ptr->position[2]) );
  } else if (h.objectType == TRIANGLE) {
    double normal_v[3];
    for(int i=0; i<3; i++){
      normal_v[i] = h.triangle_ptr->v[0].normal[i]*h.interpolation.x + 
                    h.triangle_ptr->v[1].normal[i]*h.interpolation.y +
                    h.triangle_ptr->v[2].normal[i]*h.interpolation.z;
    }
     return glm::vec3(normal_v[0],normal_v[1],normal_v[2]);
  }

  return glm::vec3(0,0,1);
}

void getShadingParametersAtPoint(double* ks, double* kd, double* alpha, Hit h){
    if ( h.objectType == SPHERE) {
    memcpy(ks,h.sphere_ptr->color_specular,3*sizeof(double));
    memcpy(kd,h.sphere_ptr->color_diffuse,3*sizeof(double));
    *alpha = h.sphere_ptr->shininess;
   
    } else if ( h.objectType == TRIANGLE ) {
      for (int i=0; i<3; i++) {
        ks[i] = h.interpolation.x * h.triangle_ptr->v[0].color_specular[i] + 
                h.interpolation.y * h.triangle_ptr->v[1].color_specular[i] +
                h.interpolation.z * h.triangle_ptr->v[2].color_specular[i];
        kd[i] = h.interpolation.x * h.triangle_ptr->v[0].color_diffuse[i] + 
                h.interpolation.y * h.triangle_ptr->v[1].color_diffuse[i] +
                h.interpolation.z * h.triangle_ptr->v[2].color_diffuse[i];
        
      }
      *alpha = h.interpolation.x * h.triangle_ptr->v[0].shininess + 
              h.interpolation.y * h.triangle_ptr->v[1].shininess +
              h.interpolation.z * h.triangle_ptr->v[2].shininess;
    }
}


bool lightHitWithRay(glm::vec3 r0, glm::vec3 rd, Light l){
  glm::vec3 lp = glm::vec3(l.position[0],l.position[1],l.position[2]);
  float t = (l.position[0]-r0.x) / rd.x;
  glm::vec3 l_expected = r0 + t * rd;
  return glm::distance(l_expected,lp) < EPSILON;
}


//calculate the final color of a hitting event
glm::vec3 shadingOfHit(Hit h, int* step, int largestStep){
  *step+=1;
  //if exceeds max depth, return a default color
  if (*step > largestStep) return glm::vec3(0,0,0); 
  // if nothing hit in this event return background color
  if(h.objectType == NONE) return glm::vec3(0,0,0);


  //if something hit in this hitting event
  double color[3];
  color[0] = .0; color[1] = .0; color[2] = .0;
  double ks[3]; double kd[3]; double alpha;
  //get different ks and kd for spheres or triangles
  getShadingParametersAtPoint(ks,kd,&alpha,h);
  //get corresponding normal for the object
  glm::vec3 n = getNormOfObjectAtHit(h);
  //apply Phong shading model for each light source, if any
  //get color_diffuse the current hit
  for(int i=0; i<num_lights; i++){
    glm::vec3 lightPosition = glm::vec3(lights[i].position[0],lights[i].position[1],lights[i].position[2]);
    glm::vec3 l = glm::normalize(lightPosition-h.hittingPoint);
    if (shadowRayNotBlocked2Light(h, lights[i])){
      for(int j=0; j<3; j++) color[j] += lights[i].color[j] * kd[j] * fmax(0,glm::dot(l,n)) ;
    }
  }
 
  //get next hit to calculate specular component
  glm::vec3 v = h.incomingRay;
  glm::vec3 r = glm::normalize(2*glm::dot(v,n)*n-v);

  //if a light is in reflection dirction, calculate specular component by light
  bool noLightHit = true;
  for(int i=0; i<num_lights; i++){
    if( lightHitWithRay(h.hittingPoint,r,lights[i])){
      noLightHit = false;
      for(int j=0; j<3; j++) color[j] += lights[i].color[j] * ks[j] * pow( fmax(0,glm::dot(v,r)), alpha);
    }
  }
  //if no light directly hit, get reflection color recursively
  if(noLightHit){
    Hit nextH = cameraRayHits(h.hittingPoint, r);
    //get color of the next hit
    glm::vec3 nextC = shadingOfHit(nextH, step, largestStep);
    //apply phong specular model
    for(int j=0; j<3; j++) color[j] += nextC[j] * ks[j] * pow( fmax(0,glm::dot(v,r)), alpha);
  }
  
  //other than everything apply ambient light
  for(int j=0; j<3; j++) color[j] += ambient_light[j];
  // printf("!!!!!!!!!!%d %f %f %f\n\n\n",*step,color[0],color[1],color[2]);
  
  return glm::vec3(color[0],color[1],color[2]);

  //rtx off
  // return glm::vec3(kd[0],kd[1],kd[2]); 
}


// bool recursivelyRT(int i, int j,glm::vec3 hittingP, glm::vec3 nextV, int* step, int largestStep){
//   *step += 1;
//   if (*step > largestStep) return false; 
//   Hit h = cameraRayHits(hittingP, nextV);
//   if(h.objectType != NONE) {
//     glm::vec3 c;
//     c = shadingOfHit(h, hittingP, &nextV);
//     for(int k=0; k<3; k++) buffer[i][j][k] += c[k]*pow(0.7,*step-1.0);
  
//   } 
//   return recursivelyRT(i,j,h.hittingPoint,nextV,step,largestStep) || h.objectType != NONE;
// }

//a placeholder for display function
void display(){};

void init()
{
  glMatrixMode(GL_PROJECTION);
  glOrtho(0,WIDTH,0,HEIGHT,1,-1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT);

  //initalize rays
  double dx = 2*aspectRatio*tanHalfFov/WIDTH;
  double dy = 2*tanHalfFov/HEIGHT;
  rays = (glm::vec3*) malloc(WIDTH*HEIGHT*sizeof(glm::vec3));
  for(int i=0; i<HEIGHT; i++){
    for(int j=0; j<WIDTH; j++){
      rays[WIDTH*i+j] =glm::normalize( glm::vec3(-aspectRatio*tanHalfFov + j*dx, -tanHalfFov + i*dy, -1) );
    }
  }
  glm::vec3 cameraPosition = glm::vec3(0,0,0);

  //for each ray, generate a hit event
  //and for every hit event, apply shading
  for(int i=0; i<HEIGHT; i++){
    for(int j=0; j<WIDTH; j++){
      int step = 0;
      
      Hit h = cameraRayHits(cameraPosition, rays[WIDTH*i+j]);
      // printf("%d %d \n",i,j);
      glm::vec3 c = shadingOfHit(h, &step, 10);
      //if the light hits nothing, set the pixel to background color 
      buffer[i][j][0] = c.x;
      buffer[i][j][1] = c.y;
      buffer[i][j][2] = c.z;
      

    }
   
  }

  free(rays);
}



int main(int argc, char ** argv)
{
  if ((argc < 2) || (argc > 3))
  {  
    printf ("Usage: %s <input scenefile> [output jpegname]\n", argv[0]);
    exit(0);
  }
  if(argc == 3)
  {
    mode = MODE_JPEG;
    filename = argv[2];
  }
  else if(argc == 2)
    mode = MODE_DISPLAY;

  glutInit(&argc,argv);
  loadScene(argv[1]);

  glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
  glutInitWindowPosition(0,0);
  glutInitWindowSize(WIDTH,HEIGHT);
  int window = glutCreateWindow("Ray Tracer");
  #ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(WIDTH - 1, HEIGHT - 1);
  #endif
  glutDisplayFunc(display);
  glutIdleFunc(idle);
  init();
  glutMainLoop();
}

