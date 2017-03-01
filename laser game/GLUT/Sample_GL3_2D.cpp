#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>

#include <GL/glew.h>
#include <GL/glu.h>
#include <GL/freeglut.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <bits/stdc++.h>
#include <ao/ao.h>

using namespace std;

void playsound(int rates)
{
ao_device *device;
	ao_sample_format format;
	int default_driver;
	char *buffer;
	int buf_size;
	int sample;
	float freq = 440.0;
	int i;

	/* -- Initialize -- */
	ao_initialize();
	/* -- Setup for default driver -- */
	default_driver = ao_default_driver_id();
    memset(&format, 0, sizeof(format));
	format.bits = 16;
	format.channels = 2;
	format.rate = rates;
	format.byte_format = AO_FMT_LITTLE;

	/* -- Open driver -- */
	device = ao_open_live(default_driver, &format, NULL /* no options */);
	if (device == NULL) {
		fprintf(stderr, "Error opening device.\n");
	}

	/* -- Play some stuff -- */
	buf_size = format.bits/8 * format.channels * format.rate;
	buffer = (char *)calloc(buf_size,sizeof(char));
	for (i = 0; i < format.rate; i++) {
		sample = (int)(0.75 * 32768.0 *
			sin(2 * M_PI * freq * ((float) i/format.rate)));
		/* Put the same stuff in left and right channel */
		buffer[4*i] = buffer[4*i+2] = sample & 0xff;
		buffer[4*i+1] = buffer[4*i+3] = (sample >> 8) & 0xff;
	}
	ao_play(device, buffer, buf_size/10);
	/* -- Close and shutdown -- */
	ao_close(device);
	ao_shutdown();
	free(buffer);
}

struct VAO {
	GLuint VertexArrayID;
	GLuint VertexBuffer;
	GLuint ColorBuffer;

	GLenum PrimitiveMode;
	GLenum FillMode;
	int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

struct bucketNode {
	struct VAO* bucket;
	glm::mat4 translationVector;
	float dx;
};
typedef struct bucketNode bucketNode;
bucketNode buckets[2];

struct laserPlatform{
	struct VAO* platform;
	glm::mat4 translationVector;
	glm::mat4 rotationVector;
	float rot;
	float dy;
};
typedef struct laserPlatform laserPlatform;
laserPlatform platform1[1];

struct laser{
  struct VAO* laserLoc;
  glm::mat4 translationVector;
  glm::mat4 rotationVector;
  float angle;
  float l;
  float b;  
  float v;
};
typedef struct laser laser;
map<int,laser> lasers;

struct block{
  struct VAO* blockLoc;
  glm::mat4 translationVector;
  int colorcode;
  float x;
  float dy;
  float dx;
  float dvy;
};
typedef struct block block;
map<int,block> blocks;

struct point {
  float x;
  float y;
};
typedef struct point point;

struct mirror {
  struct VAO* mirrorLoc;
  glm::mat4 translationVector;
  glm::mat4 rotationVector;
  float angle;
};
typedef struct mirror mirror;
vector<mirror> mirrors;

GLuint programID;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
	struct VAO* vao = new struct VAO;
	vao->PrimitiveMode = primitive_mode;
	vao->NumVertices = numVertices;
	vao->FillMode = fill_mode;

	// Create Vertex Array Object
	glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
	glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
	glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

	glBindVertexArray (vao->VertexArrayID); // Bind the VAO 
	glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices 
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
	glVertexAttribPointer(
						  0,                  // attribute 0. Vertices
						  3,                  // size (x,y,z)
						  GL_FLOAT,           // type
						  GL_FALSE,           // normalized?
						  0,                  // stride
						  (void*)0            // array buffer offset
						  );

	glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors 
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
	glVertexAttribPointer(
						  1,                  // attribute 1. Color
						  3,                  // size (r,g,b)
						  GL_FLOAT,           // type
						  GL_FALSE,           // normalized?
						  0,                  // stride
						  (void*)0            // array buffer offset
						  );

	return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
	GLfloat* color_buffer_data = new GLfloat [3*numVertices];
	for (int i=0; i<numVertices; i++) {
		color_buffer_data [3*i] = red;
		color_buffer_data [3*i + 1] = green;
		color_buffer_data [3*i + 2] = blue;
	}

	return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
	// Change the Fill Mode for this object
	glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

	// Bind the VAO to use
	glBindVertexArray (vao->VertexArrayID);

	// Enable Vertex Attribute 0 - 3d Vertices
	glEnableVertexAttribArray(0);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

	// Enable Vertex Attribute 1 - Color
	glEnableVertexAttribArray(1);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

	// Draw the geometry !
	glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/

float triangle_rot_dir = 1;
float rectangle_rot_dir = 1;
bool triangle_rot_status = true;
bool rectangle_rot_status = true;
bool shootLaser = true;
void createLaser();
void increaseSpeed();
void decreaseSpeed();
int timer=0;
int timerBlock = 0;
int numlaser = 0;
int numblocks = 0;
VAO* lowersection;
float speed = 0.025;
int score = 0;
int level = 1;
VAO* progressbar, *bar, *outsideWorld;
glm::mat4 barTranslate = glm::translate(glm::vec3(-3.55f, 3.7f, 0.0f));

/* Executed when a regular key is pressed */

float Pleft = -4.0f, Pright = 4.0f, Ptop = 4.0f, Pbottom = -4.0f, Pfar = 500.0f, Pnear = 0.1f;

void changeZoom() 
{
  Matrices.projection = glm::ortho(Pleft, Pright, Pbottom, Ptop, Pnear, Pfar);
  return;
}

void zoomIn()
{
  if(Pright > 2.048)
  {
	Pleft *= 0.8;
	Pright *= 0.8;
	Ptop *= 0.8;
	Pbottom *= 0.8;
  }
}

void zoomOut()
{
  if(Pright < 7.8125)
  {
	Pleft *= 1.25;
	Pright *= 1.25;
	Ptop *= 1.25;
	Pbottom *= 1.25;
  }
}

void panRight()
{
  if(Pright < 6.5)
  {
	Pleft += 0.5f;
	Pright += 0.5f;
  }
}
void panLeft()
{
  if(Pright > 1.5)
  {
	Pleft -= 1.0f;
	Pright -= 1.0f;
  }
}

void restoreOrignal()
{
	Pleft = -4.0f;
	Pright = 4.0f;
	Ptop = 4.0f;
	Pbottom = -4.0f;
}

void keyboardDown (unsigned char key, int x, int y)
{
   if(key == 'S' || key == 's')
   {
	if(platform1[0].translationVector[3][1] <= 3.0-platform1[0].dy)
	  platform1[0].translationVector[3][1] += platform1[0].dy;
   }
   if(key == 'F' || key == 'f')
   {
	if(platform1[0].translationVector[3][1] >= -2.0 + platform1[0].dy)
	  platform1[0].translationVector[3][1] -= platform1[0].dy;
   } 
   if(key == 'a' || key == 'A')
   {
	if(platform1[0].rot <= 44)
	  platform1[0].rot += 2;
   }
   if(key == 'd' || key == 'D')
   {
	if(platform1[0].rot >= -44)
	  platform1[0].rot -= 2;
   }
   if(key == ' ')
	{
	  if (shootLaser)
	  {
		createLaser();
	  }
	}
	if(key == 'n' || key == 'N')
	{
	  increaseSpeed();
	}
	if(key == 'm' || key == 'M')
	{
	  decreaseSpeed();
	}
	if(key == 'c' || key == 'C')
	{
		restoreOrignal();
		changeZoom();
	}
}

/* Executed when a regular key is released */
void keyboardUp (unsigned char key, int x, int y)
{
	
}

/* Executed when a special key is pressed */
void keyboardSpecialDown (int key, int x, int y)                //stored column major
{
	if (key == GLUT_KEY_LEFT && glutGetModifiers() == GLUT_ACTIVE_ALT)  //bucket 0
	{
		if(buckets[0].translationVector[3][0] >= -3.4+buckets[0].dx)
			buckets[0].translationVector[3][0] -= buckets[0].dx;
	}
	else if (key == GLUT_KEY_RIGHT && glutGetModifiers() == GLUT_ACTIVE_ALT)
	{
		if(buckets[0].translationVector[3][0] <= 3.4-buckets[0].dx)
			buckets[0].translationVector[3][0] += buckets[0].dx;
	}
	else if (key == GLUT_KEY_LEFT && glutGetModifiers() == GLUT_ACTIVE_CTRL)  //bucket 0
	{
		if(buckets[1].translationVector[3][0] >= -3.4+buckets[1].dx)
			buckets[1].translationVector[3][0] -= buckets[1].dx;
	}
	else if (key == GLUT_KEY_RIGHT && glutGetModifiers() == GLUT_ACTIVE_CTRL)
	{
		if(buckets[1].translationVector[3][0] <= 3.4-buckets[1].dx)
			buckets[1].translationVector[3][0] += buckets[1].dx;
	}
	else if (key == GLUT_KEY_LEFT)
	{
		panLeft();
		changeZoom();
	}
	else if (key == GLUT_KEY_RIGHT)
	{
		panRight();
		changeZoom();
	}
	else if (key == GLUT_KEY_UP)
	{
		zoomIn();
		changeZoom();
	}
	else if (key == GLUT_KEY_DOWN)
	{
		zoomOut();
		changeZoom();
	}
}

/* Executed when a special key is released */
void keyboardSpecialUp (int key, int x, int y)
{
}

/* Executed when a mouse button 'button' is put into state 'state'
 at screen position ('x', 'y')
 */

int platformSelect = 0, blueSelect = 0, redSelect = 0, py, bx, rx, panLock = 0, panx;
void mouseClick (int button, int state, int x, int y)
{
	float xcord = ((float)x/600)*8 - 4;
	float ycord = -((float)y/600)*8 + 4;
	//cout << xcord << '\t' << ycord <<endl;
	if(button==GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		if(ycord <= 0.6 + buckets[0].translationVector[3][1] && ycord >= buckets[0].translationVector[3][1])
		{
			if(xcord <= 0.4 + buckets[1].translationVector[3][0] && xcord >= -0.4 + buckets[1].translationVector[3][0])
			{
				redSelect = 1;
				rx = xcord;
				//cout << xcord << '\t' << ycord <<endl;
			}
			else if (xcord <= 0.4 + buckets[0].translationVector[3][0] && xcord >= -0.4 + buckets[0].translationVector[3][0])
			{
				blueSelect = 1;
				bx = xcord;
			}
		}
		else if(xcord <= -3.5 && ycord <= 0.25 + platform1[0].translationVector[3][1] && ycord >= -0.25 + platform1[0].translationVector[3][1])   		
		{
			platformSelect = 1;
			py = ycord;
		}
	}
	else if(button==GLUT_LEFT_BUTTON && state == GLUT_UP)
	{
		if(redSelect)
		{
			buckets[1].translationVector[3][0] += (xcord - rx);
			if(buckets[1].translationVector[3][0] > 3.4)
				buckets[1].translationVector[3][0] = 3.4;
			else if(buckets[1].translationVector[3][0] < -3.4)
				buckets[1].translationVector[3][0] = -3.4;
			redSelect = 0;
		}
		else if(blueSelect)
		{
			buckets[0].translationVector[3][0] += (xcord - bx);
			if(buckets[0].translationVector[3][0] > 3.4)
				buckets[0].translationVector[3][0] = 3.4;
			else if(buckets[0].translationVector[3][0] < -3.4)
				buckets[0].translationVector[3][0] = -3.4;
			blueSelect = 0;
		}
		else if(platformSelect)
		{
			platform1[0].translationVector[3][1] += (ycord - py);
			if(platform1[0].translationVector[3][1] > 3.0)
				platform1[0].translationVector[3][1] = 3.0;
			else if(platform1[0].translationVector[3][1] < -2.0)
				platform1[0].translationVector[3][1] = -2.0;
			platformSelect = 0;
		}
	}
	else if(button == 3 && state == GLUT_DOWN)
	{
		zoomIn();
		changeZoom();
	}
	else if(button == 4 && state == GLUT_DOWN)
	{
		zoomOut();
		changeZoom();
	}
	else if(button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
	{
		panx = xcord;
		panLock = 1;
	}
	else if(button == GLUT_RIGHT_BUTTON && state == GLUT_UP && panLock)
	{
		float direction = xcord - panx;
		if(direction > 0)
		{
			panRight();
			changeZoom();
		}
		else if(direction < 0)
		{
			panLeft();
			changeZoom();
		}
		panLock = 0;
	}
}
	
/* Executed when the mouse moves to position ('x', 'y') */
void mouseMotion (int x, int y)
{
}


/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (int width, int height)
{
	GLfloat fov = 90.0f;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) width, (GLsizei) height);

	// set the projection matrix as perspective/ortho
	// Store the projection matrix in a variable for future use

	// Perspective projection for 3D views
	// Matrices.projection = glm::perspective (fov, (GLfloat) width / (GLfloat) height, 0.1f, 500.0f);

	// Ortho projection for 2D views
	Matrices.projection = glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f, 500.0f);
}

void applySpeed()
{
  for(map<int,block>::iterator it = blocks.begin(); it != blocks.end(); it++)
  {
	(it->second).dvy = speed;
  }
}

void increaseSpeed()
{
  if(speed <= 0.1)
  {
	speed += 0.025;
	applySpeed();
  }
}

void decreaseSpeed()
{
  if(speed > 0.05)
  {
	speed -= 0.025;
	applySpeed();
  }
}

void increasePoints()
{
  score += 10;
}

void decreasePoints()
{
  score -= 5;
}

void createBuckets()
{
	GLfloat vertex_buffer_data [] = {
		-0.20, -0.20, 0,
		0.20, -0.20, 0,
		0.4, 0.60, 0,
		0.4, 0.60, 0,
		-0.4, 0.60, 0,
		-0.20, -0.20, 0
	};
	GLfloat color_buffer_data0 [] = {
		0, 0, 1,
		0, 0, 1,
		0, 0, 1,
		0, 0, 1,
		0, 0, 1,
		0, 0, 1
	};
	GLfloat color_buffer_data1 [] = {
		1, 0, 0,
		1, 0, 0,
		1, 0, 0,
		1, 0, 0,
		1, 0, 0,
		1, 0, 0
	};
	buckets[0].bucket = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data0, GL_FILL);
	buckets[1].bucket = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data1, GL_FILL);
	buckets[0].translationVector = glm::translate(glm::vec3(-3.4f, -3.4f, 0.0f));
	buckets[1].translationVector = glm::translate(glm::vec3(3.4f, -3.4f, 0.0f));
	buckets[0].dx = 0.04;
	buckets[1].dx = 0.04;
}

void createLaserPlatform()
{
	GLfloat vertex_buffer_data [] = {
		-0.25, -0.25, 0,
		0.25, -0.25, 0,
		0.25, 0.25, 0,
		0.25, 0.25, 0,
		-0.25, 0.25, 0,
		-0.25, -0.25, 0,
		0.25, -0.05, 0,
		0.45, -0.05, 0,
		0.45, 0.05, 0,
		0.45, 0.05, 0,
		0.25, 0.05, 0,
		0.25, -0.05, 0
	};
	GLfloat color_buffer_data [] = {
		0.5, 0.1, 0.25,
		0.5, 0.1, 0.25,
		0.5, 0.1, 0.25,
		0.5, 0.1, 0.25,
		0.5, 0.1, 0.25,
		0.5, 0.1, 0.25,
		0.5, 0.1, 0.25,
		0.5, 0.1, 0.25,
		0.90, 0.90, 0.90,
		0.90, 0.90, 0.90,
		0.5, 0.1, 0.25,
		0.5, 0.1, 0.25
	};
	platform1[0].rot = 0;
	platform1[0].platform = create3DObject(GL_TRIANGLES, 12, vertex_buffer_data, color_buffer_data, GL_FILL);
	platform1[0].translationVector = glm::translate(glm::vec3(-4.0f, 0.5f, 0.0f));
	platform1[0].rotationVector = glm::rotate((float)(platform1[0].rot*M_PI/180.0f), glm::vec3(0, 0, 1));
	platform1[0].dy = 0.04;
}

void createLowerSection()
{
  GLfloat vertex_buffer_data [] = {
	-4, -4, 0,
	4, -4, 0,
	4, -2.5, 0,
	4, -2.5, 0,
	-4, -2.5, 0,
	-4, -4, 0
  };
  GLfloat color_buffer_data [] = {
	1, 1, 1,
	1, 1, 1,
	1, 1, 1,
	1, 1, 1,
	1, 1, 1,
	1, 1, 1
  };
  lowersection = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createLaser()
{
  if(shootLaser)
  {
	shootLaser = false;
	timer = 0;
	laser ptr;
	GLfloat vertex_buffer_data []= {        //length = 0.65, breadth = 0.1
	  -0.2, -0.05, 0,
	  0.45, -0.05, 0,
	  0.45, 0.05, 0,
	  0.45, 0.05, 0,
	  -0.2, 0.05, 0,
	  -0.2, -0.05, 0
	};
	GLfloat color_buffer_data [] = {
	  0,0,0,
	  0,0,0,
	  /*0,0,0,
	  0,0,0,
	  0,0,0,
	  0,0,0*/
	  0.5, 0.5, 0.5,
	  0.5, 0.5, 0.5,
	  0, 0, 0,
	  0, 0, 0
	};
	ptr.laserLoc = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
	ptr.translationVector = platform1[0].translationVector;
	ptr.rotationVector = platform1[0].rotationVector;
	ptr.angle = (float)(platform1[0].rot*M_PI/180.0f);
	ptr.l = 0.325;
	ptr.b = 0.05;
	ptr.v = 0.1;
	lasers[numlaser++] = ptr;
  }
}

void createBlocks()
{
  block ptr;
  float x = (rand()%600)/100 - 3;
  GLfloat vertex_buffer_data [] = {
	x, 4, 0,
	x + 0.25, 4, 0,
	x + 0.25, 4.25, 0,
	x + 0.25, 4.25, 0,
	x, 4.25, 0,
	x, 4, 0
  };
  GLfloat color_buffer_data [18] = {0};
  int color = rand()%3;
  if(color == 1)
  {
	for(int i = 2; i<18; i+=3) color_buffer_data[i] = 1;
  }
  else if(color == 2)
  {
	for(int i = 0; i<18; i+=3) color_buffer_data[i] = 1;
  }
  ptr.blockLoc = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
  ptr.translationVector = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
  ptr.colorcode = color;
  ptr.dvy = speed;
  ptr.dy = 0.125;
  ptr.dx = 0.125;
  ptr.x = x;
  blocks[numblocks++] = ptr;
}

void createMirror()
{
  GLfloat vertex_buffer_data [] = {
	-0.5, -0.03, 0,
	0.5, -0.03, 0,
	0.5, 0.03, 0,
	0.5, 0.03, 0,
	-0.5, 0.03, 0,
	-0.5, -0.03, 0
  };
  GLfloat color_buffer_data [18] = {0};
  for(int i = 0; i <1; i++)
  {
	mirror ptr;
	ptr.mirrorLoc = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
	mirrors.push_back(ptr);
  }
  mirrors[0].translationVector = glm::translate(glm::vec3(2.0f, 2.0f, 0.0f));
  //mirrors[1].translationVector = glm::translate(glm::vec3(-1.0f, -1.0f, 0.0f));
  mirrors[0].angle = 135*M_PI/180.0f;
  //mirrors[1].angle = 210*M_PI/180.0f;
  mirrors[0].rotationVector = glm::rotate(mirrors[0].angle, glm::vec3(0.0f, 0.0f, 1.0f));
  //mirrors[1].rotationVector = glm::rotate(mirrors[1].angle, glm::vec3(0.0f, 0.0f, 1.0f));
}

void createProgressBar()
{
  GLfloat vertex_buffer_data1 [] = {
	-3.9, 3.5, 0,
	-3.2, 3.5, 0,
	-3.2, 3.9, 0,
	-3.2, 3.9, 0,
	-3.9, 3.9, 0,
	-3.9, 3.5, 0
  };
  GLfloat vertex_buffer_data2 [] = {
	-0.35, -0.2, 0,
	0.35, -0.2, 0,
	0.35, 0.2, 0,
	0.35, 0.2, 0,
	-0.35, 0.2, 0,
	-0.35, -0.2, 0
  };
  GLfloat color_buffer_data1 [] = {
	1, 1, 1,
	1, 1, 1,
	1, 1, 1,
	1, 1, 1,
	1, 1, 1,
	1, 1, 1
  };
  GLfloat color_buffer_data2 [] = {
	1, 0.5, 0,
	1, 0.5, 0,
	1, 0.5, 0,
	1, 0.5, 0,
	1, 0.5, 0,
	1, 0.5, 0
  };
  progressbar = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data1, color_buffer_data1, GL_FILL);
  bar = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data2, color_buffer_data2, GL_FILL);
}

void outsideBorder()
{
	GLfloat vertex_buffer_data [] = {
		-50.0, 4.0, 0,		//top
		50.0, 4.0, 0,
		50.0, 50.0, 0,
		50.0, 50.0, 0,
		-50.0, 50.0, 0,
		-50.0, 4.0, 0,
		-50.0, -50.0, 0,	//bottom
		50.0, -50.0, 0,
		50.0, -4.0, 0,
		50.0, -4.0, 0,
		-50.0, -4.0, 0,
		-50.0, -50.0, 0,
		-50.0, -4.0, 0,		//left
		-4.0, -4.0, 0,
		-4.0, 4.0, 0,
		-4.0, 4.0, 0,
		-50.0, 4.0, 0,
		-50.0, -4.0, 0,
		4.0, -4.0, 0,		//right
		50.0, -4.0, 0,
		50.0, 4.0, 0,
		50.0, 4.0, 0,
		4.0, 4.0, 0,
		4.0, -4.0, 0
	};
	GLfloat color_buffer_data [72] = {0};
	outsideWorld = create3DObject(GL_TRIANGLES, 24, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void checkInsideLaser()
{
  for(map<int,laser>::iterator it = lasers.begin(); it != lasers.end(); it++)
  {
	if((it->second).translationVector[3][0] > 4.7 || (it->second.translationVector[3][0] < -4.7))
	{
	  lasers.erase(it);
	  break;
	}
  }
}

void checkInsideBlock()
{
  for(map<int,block>::iterator it = blocks.begin(); it != blocks.end(); it++)
  {
	if((it->second).translationVector[3][1] < -8.9)
	{
	  blocks.erase(it);
	  break;
	}
  }
}

void checkCollision()
{
  int flag = 0;
  for(map<int,laser>::iterator laserit = lasers.begin(); laserit != lasers.end(); laserit++)
  {
	for(map<int,block>::iterator blockit = blocks.begin(); blockit != blocks.end(); blockit++)
	{
	//if((blockit->second).translationVector[3][1] >= -2.25)
	//{  
	  point pa, pb, pc, pd, px, py, pz, pw, rl, rb;
	  rl.x = 0.125 + (laserit->second).translationVector[3][0];
	  rl.y = (laserit->second).translationVector[3][1];
	  rb.x = 0.125 + (blockit->second).translationVector[3][0] + (blockit->second).x;
	  rb.y = 4.125 + (blockit->second).translationVector[3][1] ;
	  pa.x = rl.x + (((laserit->second).l) * cos((laserit->second).angle)) + (((laserit->second).b)*sin((laserit->second).angle));
	  pa.y = rl.y + (((laserit->second).l) * sin((laserit->second).angle)) - (((laserit->second).b)*cos((laserit->second).angle));
	  pb.x = rl.x + (((laserit->second).l) * cos((laserit->second).angle)) - (((laserit->second).b)*sin((laserit->second).angle));
	  pb.y = rl.y + (((laserit->second).l) * sin((laserit->second).angle)) + (((laserit->second).b)*cos((laserit->second).angle));
	  pc.x = rl.x - (((laserit->second).l) * cos((laserit->second).angle)) - (((laserit->second).b)*sin((laserit->second).angle));
	  pc.y = rl.y - (((laserit->second).l) * sin((laserit->second).angle)) + (((laserit->second).b)*cos((laserit->second).angle));
	  pd.x = rl.x - (((laserit->second).l) * cos((laserit->second).angle)) + (((laserit->second).b)*sin((laserit->second).angle));
	  pd.y = rl.y - (((laserit->second).l) * sin((laserit->second).angle)) - (((laserit->second).b)*cos((laserit->second).angle));

	  px.x = rb.x - (blockit->second).dx;
	  px.y = rb.y + (blockit->second).dy;
	  py.x = rb.x + (blockit->second).dx;
	  py.y = rb.y + (blockit->second).dy;
	  pz.x = rb.x + (blockit->second).dx;
	  pz.y = rb.y - (blockit->second).dy;
	  pw.x = rb.x - (blockit->second).dx;
	  pw.y = rb.y - (blockit->second).dy;

	  if(pa.x >= px.x && px.x >= pc.x && pb.y >= px.y && px.y >= pd.y)
	  {
		if((blockit->second).colorcode != 0)
		  decreasePoints();
		else
		  increasePoints();
		//playsound(9000);
		lasers.erase(laserit);
		blocks.erase(blockit);
		flag = 1;
		break;
	  }
	  else if(pb.x >= pw.x && pw.x >= pd.x && pa.y >= pw.y && pw.y >= pc.y)
	  {
		if((blockit->second).colorcode != 0)
		  decreasePoints();
		else
		  increasePoints();
		lasers.erase(laserit);
		blocks.erase(blockit);
		flag = 1;
		break;
	  }
	  else if(pa.x >= py.x && py.x >= pd.x && pa.y >= py.y && py.y >= pc.y)
	  {
		if((blockit->second).colorcode != 0)
		  decreasePoints();
		else
		  increasePoints();
		lasers.erase(laserit);
		blocks.erase(blockit);
		flag = 1;
		break;
	  }
	  else if(pa.x >= pz.x && pz.x >= pc.x && pb.y >= pz.y && pz.y >= pd.y)
	  {
		if((blockit->second).colorcode != 0)
		  decreasePoints();
		else
		  increasePoints();
		lasers.erase(laserit);
		blocks.erase(blockit);
		flag = 1;
		break;
	  }
	  else if((pa.x >= px.x) && (px.x >=pc.x) && (px.y >= pa.y) && (pa.y >= pw.y))
	  {
		if((blockit->second).colorcode != 0)
		  decreasePoints();
		else
		  increasePoints();
		lasers.erase(laserit);
		blocks.erase(blockit);
		flag = 1;
		break;
	  }
	}
	//}
	if (flag == 1)
	{
	  playsound(9000);
	  break;
	}
  }
}

void checkBucket()
{
  point rb, b00, b01, b10, b11, pa, pb;
  for(map<int,block>::iterator it = blocks.begin(); it != blocks.end(); it++)
  {
	rb.x = 0.125 + (it->second).translationVector[3][0] + (it->second).x;
	rb.y = 4.125 + (it->second).translationVector[3][1];
	pa.x = rb.x - (it->second).dx;
	pa.y = rb.y - (it->second).dy;
	pb.x = rb.x + (it->second).dx;
	pb.y = rb.y - (it->second).dy;
	b00.x = -0.4 + buckets[0].translationVector[3][0];
	b01.x = 0.4 + buckets[0].translationVector[3][0];
	b00.y = 0.6 + buckets[0].translationVector[3][1];
	b01.y = 0.6 + buckets[0].translationVector[3][1];
	b10.x = -0.4 + buckets[1].translationVector[3][0];
	b11.x = 0.4 + buckets[1].translationVector[3][0];
	b10.y = 0.6 + buckets[1].translationVector[3][1];
	b11.y = 0.6 + buckets[1].translationVector[3][1];

	if((b00.y - pa.y)>= 0.05 && b00.x <= pa.x && pb.x <= b01.x)
	{
	  playsound(50000);
	  blocks.erase(it);
	  break;
	}
	if((b10.y - pa.y)>= 0.05 && b10.x <= pa.x && pb.x <= b11.x)
	{
		playsound(50000);
	  blocks.erase(it);
	  break;
	}
  }
}

void checkCollisionMirror()
{
  for(vector<mirror>::iterator mirrorit = mirrors.begin(); mirrorit != mirrors.end(); mirrorit++)
  {
	point a, b;
	a.x = 0.5*cos(mirrorit->angle) + mirrorit->translationVector[3][0];
	b.x = -0.5*cos(mirrorit->angle) + mirrorit->translationVector[3][0];
	a.y = 0.5*sin(mirrorit->angle) + mirrorit->translationVector[3][1];
	b.y = -0.5*sin(mirrorit->angle) + mirrorit->translationVector[3][1];
	float m = tan(mirrorit->angle);
	for(map<int,laser>::iterator laserit = lasers.begin(); laserit != lasers.end(); laserit++)
	{
	  point lasercord;
	  lasercord.x = 0.125 + (laserit->second).translationVector[3][0] + (laserit->second).l * cos((laserit->second).angle);
	  lasercord.y = (laserit->second).translationVector[3][1] + (laserit->second).l * sin((laserit->second).angle);
	  if(abs(m*lasercord.x - lasercord.y - (m*(mirrorit->translationVector[3][0])) + mirrorit->translationVector[3][1]) <= 0.1
		&& (a.y >= lasercord.y && b.y <= lasercord.y))
	  {
		(laserit->second).angle = 2*mirrors[0].angle - (laserit->second).angle - (180*M_PI/180.0f);
		(laserit->second).v = -(laserit->second).v;
	  }
	  //else if(abs(m*lasercord.x - lasercord.y - (m*(mirrorit->translationVector[3][0])) + mirrorit->translationVector[3][1]) <= 0.1 && (a.y <= lasercord.y && b.y >= lasercord.y))
	  //{
		//(laserit->second).angle = 2*mirrors[0].angle - (laserit->second).angle - (180*M_PI/180.0f);
		//(laserit->second).v = -(laserit->second).v;
	  //}
	}
  }
}

float camera_rotation_angle = 90;
float rectangle_rotation = 0;
float triangle_rotation = 0;

/* Render the scene with openGL */
/* Edit this function according to your assignment */


void draw ()
{
  // clear the color and depth in the frame buffer
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // use the loaded shader program
  // Don't change unless you know what you are doing
  glUseProgram (programID);

  // Eye - Location of camera. Don't change unless you are sure!!
  glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
  // Target - Where is the camera looking at.  Don't change unless you are sure!!
  glm::vec3 target (0, 0, 0);
  // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
  glm::vec3 up (0, 1, 0);

  // Compute Camera matrix (view)
  // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
  //  Don't change unless you are sure!!
  Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

  // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
  //  Don't change unless you are sure!!
  glm::mat4 VP = Matrices.projection * Matrices.view;

  // Send our transformation to the currently bound shader, in the "MVP" uniform
  // For each model you render, since the MVP will be different (at least the M part)
  //  Don't change unless you are sure!!
  glm::mat4 MVP;	// MVP = Projection * View * Model

  // Load identity to model matrix
  Matrices.model = glm::mat4(1.0f);

  /* Render your scene */
  
  //lasers
  timer ++;
  if(timer == 50)
  {
	shootLaser = true;
  }
  checkInsideLaser();
  checkInsideBlock();
  checkCollision();
  checkBucket();
  checkCollisionMirror();
  for(map<int,laser>::iterator it = lasers.begin(); it != lasers.end(); it++)
	{
	  Matrices.model = glm::mat4(1.0f);
	  glm::mat4 rotLaser = glm::rotate((float)((it->second).angle), glm::vec3(0, 0, 1));
	  (it->second).translationVector[3][0] += (it->second).v * cos((it->second).angle);
	  (it->second).translationVector[3][1] += (it->second).v * sin((it->second).angle);
	  Matrices.model *= ((it->second).translationVector * rotLaser);
	  MVP = VP * Matrices.model;
	  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	  draw3DObject((it->second).laserLoc);
	}

  //laserplatform
  Matrices.model = glm::mat4(1.0f);             //laserplatform
  glm::mat4 rotVector = glm::rotate((float)(platform1[0].rot*M_PI/180.0f), glm::vec3(0, 0, 1));
  Matrices.model *= (platform1[0].translationVector * rotVector);
  MVP = VP * Matrices.model;
  //  Don't change unless you are sure!!
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(platform1[0].platform);

  //lowersection boundary
  Matrices.model = glm::mat4(1.0f);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(lowersection);

  //blocks
  timerBlock ++;
  if (timerBlock == 75)
  {
	timerBlock = 0;
	createBlocks();
  }
  for (map<int,block>::iterator it = blocks.begin(); it != blocks.end(); it++)
  {
	Matrices.model = glm::mat4(1.0f);
	(it->second).translationVector[3][1] -= (it->second).dvy;
	Matrices.model *= (it->second).translationVector;
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	draw3DObject((it->second).blockLoc);
  }

  //bucket
  for(int i = 0; i<=1; i++)       
  {
	Matrices.model = glm::mat4(1.0f);
	Matrices.model *= buckets[i].translationVector;
	MVP = VP * Matrices.model;
  //  Don't change unless you are sure!!
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
	draw3DObject(buckets[i].bucket);
  }

  //progressbar
  Matrices.model = glm::mat4(1.0f);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(progressbar);

  Matrices.model = glm::mat4(1.0f);
  glm::mat4 scaleMatrix;
  if (timer <= 50)
	scaleMatrix = glm::scale(glm::vec3((float)timer/50, 1.0f, 1.0f));
  else
	scaleMatrix = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
  Matrices.model *= (barTranslate * scaleMatrix);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(bar);

  //mirror
  for(vector<mirror>::iterator it = mirrors.begin(); it != mirrors.end(); it++)
  {
	Matrices.model = glm::mat4(1.0f);
	Matrices.model *= (it->translationVector * it->rotationVector);
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	draw3DObject(it->mirrorLoc);
  }

  //outside border

  Matrices.model = glm::mat4(1.0f);
  MVP = VP*Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(outsideWorld);

  //score
  /*string menu = "SCORE: ";
  stringstream convert;
  convert << score;
  menu += convert.str();*/
  //printtext(10, 10, menu);
  /*glPushMatrix();
  //glTranslatef(1.0f, 1.0f, 0.0f);
   glBitmap (0, 0, 0, 0, 10, 10, NULL);
  glRasterPos3i(300, 300, 0);
  glLoadIdentity();
  glColor3f(0.0f, 0.0f, 0.0f);
  glScalef(15.0f, 15.0f, 15.0f);
  for (int i=0; i<menu.size(); i++)
  {
    glutStrokeCharacter(GLUT_STROKE_ROMAN, menu[i]);
    int width = glutStrokeWidth(GLUT_STROKE_ROMAN, menu[i]);
    cout << width << '\t';
  }*/

  /*int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    //cout << endl << "viewport :: " << viewport[0] << "\t" << viewport[1] << "\t" << viewport[2] << "\t" << viewport[3] << "\n";
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(viewport[0], viewport[2], viewport[1], viewport[3], -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRasterPos2f(2.0f, viewport[3] - 200);
    glScalef(0.5f, 0.5f, 0.0f);
    for (int i=0; i<menu.size(); i++)
  {
    glutStrokeCharacter(GLUT_STROKE_ROMAN, menu[i]);
    int width = glutStrokeWidth(GLUT_STROKE_ROMAN, menu[i]);
    cout << width << '\t';
  }

  GLenum err = glGetError();
  if(err == GL_NO_ERROR)
  {
  	cout << endl << "no error" << endl;
  }
  else if(err == GL_INVALID_ENUM)
  {
  	cout << endl << "2" << endl;
  }
  else if(err == GL_INVALID_VALUE)
  {
  	cout << endl << "3" << endl;
  }
  else if(err == GL_INVALID_OPERATION)
  {
  	cout << endl << "4" << endl;
  }
  else if(err == GL_INVALID_FRAMEBUFFER_OPERATION)
  {
  	cout << endl << "5" << endl;
  }
  else if(err == GL_OUT_OF_MEMORY)
  {
  	cout << endl << "6" << endl;
  }
  int rasterpos[4];
  glGetIntegerv(GL_CURRENT_RASTER_POSITION, rasterpos);
  cout << endl << "rasterpos :: " << rasterpos[0] << "\t" << rasterpos[1] << "\t" << rasterpos[2] << "\t" << rasterpos[3] << "\n";
  
      glMatrixMode( GL_PROJECTION );
    glPopMatrix();
    glMatrixMode( GL_MODELVIEW );   
    glPopMatrix();*/

/*int rasterpos1[4];
  glGetIntegerv(GL_CURRENT_RASTER_POSITION, rasterpos1);
  cout << endl << "rasterpos1 :: " << rasterpos1[0] << "\t" << rasterpos1[1] << "\t" << rasterpos1[2] << "\t" << rasterpos1[3] << "\n";
  

  glPushMatrix();
  glRasterPos3i(0, 0, 0);
  glLoadIdentity();
  glColor3f(1.0f, 0.0f, 0.0f);
  glScalef(0.25f, 0.25f, 0.25f);
  for (int i=0; i<menu.size(); i++)
  {
    glutStrokeCharacter(GLUT_STROKE_ROMAN, menu[i]);
    int width = glutStrokeWidth(GLUT_STROKE_ROMAN, menu[i]);
    cout << width << '\t';
  }
  GLenum err = glGetError();
  if(err == GL_NO_ERROR)
  {
  	cout << endl << "no error" << endl;
  }
  else if(err == GL_INVALID_ENUM)
  {
  	cout << endl << "2" << endl;
  }
  else if(err == GL_INVALID_VALUE)
  {
  	cout << endl << "3" << endl;
  }
  else if(err == GL_INVALID_OPERATION)
  {
  	cout << endl << "4" << endl;
  }
  else if(err == GL_INVALID_FRAMEBUFFER_OPERATION)
  {
  	cout << endl << "5" << endl;
  }
  else if(err == GL_OUT_OF_MEMORY)
  {
  	cout << endl << "6" << endl;
  }
  int rasterpos[4];
  glGetIntegerv(GL_CURRENT_RASTER_POSITION, rasterpos);
  cout << endl << "rasterpos :: " << rasterpos[0] << "\t" << rasterpos[1] << "\t" << rasterpos[2] << "\t" << rasterpos[3] << "\n";
  
  glPopMatrix();*/


  // Swap the frame buffers
  glutSwapBuffers ();

  // Increment angles
  float increments = 1;

}

/* Executed when the program is idle (no I/O activity) */
void idle () {
	// OpenGL should never stop drawing
	// can draw the same scene or a modified scene
	draw (); // drawing same scene
}


/* Initialise glut window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
void initGLUT (int& argc, char** argv, int width, int height)
{
	// Init glut
	glutInit (&argc, argv);

	// Init glut window
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitContextVersion (3, 3); // Init GL 3.3
	glutInitContextFlags (GLUT_CORE_PROFILE); // Use Core profile - older functions are deprecated
	glutInitWindowSize (width, height);
	glutCreateWindow ("Sample OpenGL3.3 Application");

	// Initialize GLEW, Needed in Core profile
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (err != GLEW_OK) {
		cout << "Error: Failed to initialise GLEW : "<< glewGetErrorString(err) << endl;
		exit (1);
	}

	// register glut callbacks
	glutKeyboardFunc (keyboardDown);
	glutKeyboardUpFunc (keyboardUp);

	glutSpecialFunc (keyboardSpecialDown);
	glutSpecialUpFunc (keyboardSpecialUp);

	glutMouseFunc (mouseClick);
	glutMotionFunc (mouseMotion);

	glutReshapeFunc (reshapeWindow);

	glutDisplayFunc (draw); // function to draw when active
	glutIdleFunc (idle); // function to draw when idle (no I/O activity)
	
	glutIgnoreKeyRepeat (false); // Ignore keys held down
}

/* Process menu option 'op' */
void menu(int op)
{
	switch(op)
	{
		case 'Q':
		case 'q':
			exit(0);
	}
}

void addGLUTMenus ()
{
	// create sub menus
	int subMenu = glutCreateMenu (menu);
	glutAddMenuEntry ("Do Nothing", 0);
	glutAddMenuEntry ("Really Quit", 'q');

	// create main "middle click" menu
	glutCreateMenu (menu);
	glutAddSubMenu ("Sub Menu", subMenu);
	glutAddMenuEntry ("Quit", 'q');
	glutAttachMenu (GLUT_MIDDLE_BUTTON);
}


/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (int width, int height)
{
	// Create the models
	//createTriangle (); // Generate the VAO, VBOs, vertices data & copy into the array buffer
	createBuckets();
	createLaserPlatform();
	createLowerSection();
	createProgressBar();
	createMirror();
	outsideBorder();
	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");


	reshapeWindow (width, height);

	// Background color of the scene
	glClearColor (0.3f, 0.3f, 0.3f, 0.0f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);

	//createRectangle ();

	cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
	cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
	cout << "VERSION: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main (int argc, char** argv)
{
	int width = 600;
	int height = 600;

	initGLUT (argc, argv, width, height);

	addGLUTMenus ();

	initGL (width, height);

	glutMainLoop ();

	return 0;
}
