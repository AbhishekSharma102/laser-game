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
#include <FTGL/ftgl.h>
//#include <freetype2/ft2build.h>

using namespace std;

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

GLuint programID, fontProgramID;

struct FTGLFont {
		FTFont* font;
		GLuint fontMatrixID;
		GLuint fontColorID;
} GL3Font;

int levels = 0;

//0 empty, 1 normal tiles, 2 goal, -1 start, 3 bridge, 4 bridge activator, 5 weak
int levelMatrix[3][10][10] = {
		{
				{1, 1, 1, 1, 1, 1, 5, 1, 1, 2},
				{1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
				{1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
				{1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
				{1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
				{1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
				{1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
				{1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
				{1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
				{-1, 1, 1, 4, 3, 3, 3, 3, 3, 3}
		},
		{
				{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
				{0, 1, 1, 1, 1, 1, 1, 1, 1, 0},
				{0, 1, 2, 1, 1, 1, 1, 1, 1, 0},
				{0, 1, 1, 1, 1, 0, 0, 1, 1, 0},
				{0, 0, 0, 0, 0, 1, 0, 1, 1, 0},
				{0, 0, 0, 0, 0, 1, 1, 1, 1, 0},
				{1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
				{1, -1, 1, 1, 1, 1, 1, 0, 0, 0},
				{1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
				{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
		},
		{
				{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0, 0, 1, 1, 1, 0},
				{5, 5, 5, 5, 0, 0, 1, 2, 1, 0},
				{5, 1, 4, 1, 3, 3, 1, 1, 1, 0},
				{5, -1, 1, 1, 3, 3, 1, 1, 1, 0},
				{5, 1, 1, 1, 0, 0, 0, 0, 0, 0},
				{5, 5, 5, 5, 0, 0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
		}
};

struct point{
		float x;
		float y;
		float z;
};
typedef struct point point;
point start, goal;

struct tileNode{
		struct VAO* tile;
		//struct VAO* tileline;
		glm::mat4 translationVector;
		int type;
		int activated;
		int xpos;
		int ypos;
};
typedef struct tileNode tileNode;
vector<tileNode> tiles;

struct blockNode{
		struct VAO* block;
		glm::mat4 translationVector;
		glm::mat4 rotationVector;
		int xpos;
		int ypos;
};
typedef struct blockNode blockNode;
blockNode blocks[2];
int top = 1, lastTop = 0;

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

glm::vec3 getRGBfromHue (int hue)
{
		float intp;
		float fracp = modff(hue/60.0, &intp);
		float x = 1.0 - abs((float)((int)intp%2)+fracp-1.0);

		if (hue < 60)
				return glm::vec3(1,x,0);
		else if (hue < 120)
				return glm::vec3(x,1,0);
		else if (hue < 180)
				return glm::vec3(0,1,x);
		else if (hue < 240)
				return glm::vec3(0,x,1);
		else if (hue < 300)
				return glm::vec3(x,0,1);
		else
				return glm::vec3(1,0,x);
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
bool rotLock = false;
bool upLock[3] = {false, false, false};
bool downLock[3] = {false, false, false};
bool rightLock[3] = {false, false, false};
bool leftLock[3] = {false, false, false};
int activateBridgeFlag = 0;
int goalFlag = 0, fallFlag0 = 0, fallFlag1 = 0;
int towerViewLoc = 3;
bool heliViewOn = false;
bool blockViewOn = false;
bool followCamOn = false;
float heliRad = 15;
float heliElev = 0;
float heliAzi = 0;
bool downPressed = false;
int blockViewLoc = 3;
int followCamLoc = 3;
bool gameOver = false;
bool fallenBlock = false;
bool playedSound = false;
int score = 0;

// Eye - Location of camera. Don't change unless you are sure!!
//glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
glm::vec3 eye( 0, 0, 10);
// Target - Where is the camera looking at.  Don't change unless you are sure!!
glm::vec3 target (0, 0, 0);
// Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
glm::vec3 up (0, 1, 0);

void moveUp();
void moveDown();
void moveRight();
void moveLeft();
void createBase();
void playSound(int type);

void initLevel()
{
		tiles.clear();
		createBase();
		blocks[0].translationVector = glm::translate(glm::vec3(start.x, start.y, 0.5f));
		blocks[1].translationVector = glm::translate(glm::vec3(start.x, start.y, 1.5f));
		blocks[0].rotationVector = glm::rotate(0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
		blocks[1].rotationVector = glm::rotate(0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
		blocks[0].xpos = (int)(start.x + 4.5);
		blocks[1].xpos = (int)(start.x + 4.5);
		blocks[0].ypos = (int)(4.5 - start.y);
		blocks[1].ypos = (int)(4.5 - start.y);
		activateBridgeFlag = 0;
		goalFlag = 0;
		fallFlag0 = 0; 
		fallFlag1 = 0;
		rotLock = false;
		playedSound = false;
		//towerViewLoc = 3;
		//blockViewLoc = 3;
		//blockViewOn = false;
		downPressed = false;
		top = 1;
		score = 0;
		for(int i = 0; i<3; i++)
		{
				upLock[i] = false;
				downLock[i] = false;
				rightLock[i] = false;
				leftLock[i] = false;
		}
		playSound(4);
}

//different views
void topView()
{
		eye[0] = 0;
		eye[1] = 0;
		eye[2] = 10;
		target[0] = 0;
		target[1] = 0;
		target[2] = 0;
		up[0] = 0;
		up[1] = 1;
		up[2] = 0;
}

void towerView()
{

		if(towerViewLoc == 0)
		{
				eye[0] = 7;
				eye[1] = 7;
				eye[2] = 7;
				up[0] = -1;
				up[1] = -1;
				up[2] = 1;
		}
		else if(towerViewLoc == 1)
		{
				eye[0] = -7;
				eye[1] = 7;
				eye[2] = 7;
				up[0] = 1;
				up[1] = -1;
				up[2] = 1;
		}
		else if(towerViewLoc == 2)
		{
				eye[0] = -7;
				eye[1] = -7;
				eye[2] = 7;
				up[0] = 1;
				up[1] = 1;
				up[2] = 1;
		}
		else if(towerViewLoc == 3)
		{
				eye[0] = 7;
				eye[1] = -7;
				eye[2] = 7;
				up[0] = -1;
				up[1] = 1;
				up[2] = 1;
		}
		target[0] = 0;
		target[1] = 0;
		target[2] = 0;
}

void enableHeliView()
{
		heliViewOn = true;
		heliAzi = 45*(M_PI/180.0f);
		heliElev = 89*(M_PI/180.0f);
		heliRad = 15;
		up[0] = 0;
		up[1] = 0;
		up[2] = 1;
		eye[0] = heliRad * cos(heliElev) * cos(heliAzi);
		eye[1] = heliRad * cos(heliElev) * sin(heliAzi);
		eye[2] = heliRad * sin(heliElev);
		target[0] = 0;
		target[1] = 0;
		target[2] = 0;
}

void applyHeliSetting()
{
		eye[0] = heliRad * cos(heliElev) * cos(heliAzi);
		eye[1] = heliRad * cos(heliElev) * sin(heliAzi);
		eye[2] = heliRad * sin(heliElev);
}

void zoomIn()
{
		if(heliRad > 11)
				heliRad --;
		applyHeliSetting();
}

void zoomOut()
{
		if(heliRad < 19)
				heliRad ++;
		applyHeliSetting();
}

void blockView()
{
		up[0] = 0;
		up[1] = 0;
		up[2] = 1;
		eye[0] = (blocks[0].translationVector[3][0] + blocks[1].translationVector[3][0]) / 2;
		eye[1] = (blocks[0].translationVector[3][1] + blocks[1].translationVector[3][1]) / 2;
		eye[2] = (blocks[0].translationVector[3][2] + blocks[1].translationVector[3][2]) / 2;
		target[0] = (blocks[0].translationVector[3][0] + blocks[1].translationVector[3][0]) / 2;
		target[1] = (blocks[0].translationVector[3][1] + blocks[1].translationVector[3][1]) / 2;
		target[2] = (blocks[0].translationVector[3][2] + blocks[1].translationVector[3][2]) / 2;
		if(blockViewLoc == 0)
		{
				eye[0] += 1;
				target[0] += 1.5;
		}
		else if(blockViewLoc == 1)
		{
				eye[1] += 1;
				target[1] += 1.5;
		}
		else if(blockViewLoc == 2)
		{
				eye[0] -= 1;
				target[0] -= 1.5;
		}
		else if(blockViewLoc == 3)
		{
				eye[1] -= 1;
				target[1] -= 1.5;
		}
}

void followCam()
{
		up[0] = 0;
		up[1] = 0;
		up[2] = 1;
		eye[0] = (blocks[0].translationVector[3][0] + blocks[1].translationVector[3][0]) / 2;
		eye[1] = (blocks[0].translationVector[3][1] + blocks[1].translationVector[3][1]) / 2;
		eye[2] = ((blocks[0].translationVector[3][2] + blocks[1].translationVector[3][2]) / 2) + 4;
		target[0] = (blocks[0].translationVector[3][0] + blocks[1].translationVector[3][0]) / 2;
		target[1] = (blocks[0].translationVector[3][1] + blocks[1].translationVector[3][1]) / 2;
		target[2] = ((blocks[0].translationVector[3][2] + blocks[1].translationVector[3][2]) / 2);
		if(followCamLoc == 0)
		{
				eye[0] -= 2;
				target[0] += 1.5;
		}
		else if(followCamLoc == 1)
		{
				eye[1] -= 2;
				target[1] += 1.5;
		}
		else if(followCamLoc == 2)
		{
				eye[0] += 2;
				target[0] -= 1.5;
		}
		else if(followCamLoc == 3)
		{
				eye[1] += 2;
				target[1] -= 1.5;
		}
}

/* Executed when a regular key is pressed */
void keyboardDown (unsigned char key, int x, int y)
{
		if(key == 't' || key == 'T')
		{
				topView();
				//towerViewLoc = 3;
				heliViewOn = false;
				blockViewOn = false;
				followCamOn = false;
		}
		if(key == 'y' || key == 'Y')
		{
				towerViewLoc += 1;
				towerViewLoc = (towerViewLoc)%4;
				towerView();
				heliViewOn = false;
				blockViewOn = false;
				followCamOn = false;
		}
		if(key == 'u' || key == 'U')
		{
				followCamOn = true;
				followCam();
				//towerViewLoc = 3;
				heliViewOn = false;
				blockViewOn = false;
		}
		if(key == 'I' || key == 'i')
		{
				enableHeliView();
				//towerViewLoc = 3;
				blockViewOn = false;
				followCamOn = false;
		}
		if(key == 'o' || key == 'O')
		{
				blockViewLoc += 1;
				blockViewLoc = blockViewLoc % 4;
				blockViewOn = true;
				blockView();
				//towerViewLoc = 3;
				heliViewOn = false;
				followCamOn = false;
		}
		if(key == 'q' || key == 'Q')
		{
				cout << endl << "You pressed Quit" << endl;
				exit(0);
		}
}

/* Executed when a regular key is released */
void keyboardUp (unsigned char key, int x, int y)
{
}

void controlSet0(int key)
{
		if(key == GLUT_KEY_UP)
		{
				moveRight();
				activateBridgeFlag = 0;
		}
		if(key == GLUT_KEY_DOWN)
		{
				moveLeft();
				activateBridgeFlag = 0;
		}
		if(key == GLUT_KEY_RIGHT)
		{
				moveDown();
				activateBridgeFlag = 0;
		}
		if(key == GLUT_KEY_LEFT)
		{
				moveUp();
				activateBridgeFlag = 0;
		}
}

void controlSet1(int key)
{
		if(key == GLUT_KEY_UP)
		{
				moveUp();
				activateBridgeFlag = 0;
		}
		if(key == GLUT_KEY_DOWN)
		{
				moveDown();
				activateBridgeFlag = 0;
		}
		if(key == GLUT_KEY_RIGHT)
		{
				moveRight();
				activateBridgeFlag = 0;
		}
		if(key == GLUT_KEY_LEFT)
		{
				moveLeft();
				activateBridgeFlag = 0;
		}
}

void controlSet2(int key)
{
		if(key == GLUT_KEY_UP)
		{
				moveLeft();
				activateBridgeFlag = 0;
		}
		if(key == GLUT_KEY_DOWN)
		{
				moveRight();
				activateBridgeFlag = 0;
		}
		if(key == GLUT_KEY_RIGHT)
		{
				moveUp();
				activateBridgeFlag = 0;
		}
		if(key == GLUT_KEY_LEFT)
		{
				moveDown();
				activateBridgeFlag = 0;
		}
}

void controlSet3(int key)
{
		if(key == GLUT_KEY_UP)
		{
				moveDown();
				activateBridgeFlag = 0;
		}
		if(key == GLUT_KEY_DOWN)
		{
				moveUp();
				activateBridgeFlag = 0;
		}
		if(key == GLUT_KEY_RIGHT)
		{
				moveLeft();
				activateBridgeFlag = 0;
		}
		if(key == GLUT_KEY_LEFT)
		{
				moveRight();
				activateBridgeFlag = 0;
		}
}

/* Executed when a special key is pressed */
void keyboardSpecialDown (int key, int x, int y)
{
		if(!rotLock && fallFlag1 == 0 && fallFlag0 == 0 && goalFlag == 0)
		{
				if(blockViewOn)
				{
						if(blockViewLoc == 0)
						{
								controlSet0(key);
						}
						else if(blockViewLoc == 1)
						{
								controlSet1(key);
						}
						else if(blockViewLoc == 2)
						{
								controlSet2(key);
						}
						else if(blockViewLoc == 3)
						{
								controlSet3(key);
						}
						//activateBridgeFlag = 0;
				}
				else if(followCamOn)
				{
						if(followCamLoc == 0)
						{
								controlSet0(key);
								if(key == GLUT_KEY_RIGHT)
								{
										followCamLoc = 3;
								}
								if(key == GLUT_KEY_LEFT)
								{
										followCamLoc = 1;
								}
						}
						else if(followCamLoc == 1)
						{
								controlSet1(key);
								if(key == GLUT_KEY_RIGHT)
								{
										followCamLoc = 0;
								}
								if(key == GLUT_KEY_LEFT)
								{
										followCamLoc = 2;
								}
						}
						else if(followCamLoc == 2)
						{
								controlSet2(key);
								if(key == GLUT_KEY_RIGHT)
								{
										followCamLoc = 1;
								}
								if(key == GLUT_KEY_LEFT)
								{
										followCamLoc = 3;
								}
						}
						else if(followCamLoc == 3)
						{
								controlSet3(key);
								if(key == GLUT_KEY_RIGHT)
								{
										followCamLoc = 2;
								}
								if(key == GLUT_KEY_LEFT)
								{
										followCamLoc = 0;
								}
						}
						//activateBridgeFlag = 0;
				}
				else
				{
						if(key == GLUT_KEY_UP)
						{
								moveUp();
								activateBridgeFlag = 0;
						}
						if(key == GLUT_KEY_DOWN)
						{
								moveDown();
								activateBridgeFlag = 0;
						}
						if(key == GLUT_KEY_RIGHT)
						{
								moveRight();
								activateBridgeFlag = 0;
						}
						if(key == GLUT_KEY_LEFT)
						{
								moveLeft();
								activateBridgeFlag = 0;
						}
						//activateBridgeFlag = 0;
				}
		}
}

/* Executed when a special key is released */
void keyboardSpecialUp (int key, int x, int y)
{
}

/* Executed when a mouse button 'button' is put into state 'state'
   at screen position ('x', 'y')
 */
void mouseClick (int button, int state, int x, int y)
{
		if(heliViewOn)
		{
				if(button == 3 && state == GLUT_DOWN)
				{
						zoomIn();
				}
				if(button == 4 && state == GLUT_DOWN)
				{
						zoomOut();
				}
		}
}

/* Executed when the mouse moves to position ('x', 'y') */
float lastxcord, lastycord, lastlock = false;
void mouseMotion (int x, int y)
{
		if(heliViewOn)
		{
				float xcord = x;
				float ycord = y;
				if(!lastlock)
				{
						lastlock = true;
						lastycord = ycord;
						lastxcord = xcord;
				}
				else
				{
						lastlock = false;
						heliElev += (ycord - lastycord)*(M_PI/180.0f);
						if(heliElev >= 89*(M_PI/180.0f))
								heliElev -= (ycord - lastycord)*(M_PI/180.0f);
						else if(heliElev <= 0)
								heliElev = 0;
						heliAzi += (xcord - lastxcord)*(M_PI/180.0f);
						applyHeliSetting();
				}
		}
}

void createTile(int type, int y, int x)
{
		//board in xy plane
		GLfloat vertex_buffer_data [] = {
				-0.5, -0.5, 0.0,			//top face
				0.5, -0.5, 0.0,
				0.5, 0.5, 0.0,
				0.5, 0.5, 0.0,
				-0.5, 0.5, 0.0,
				-0.5, -0.5, 0.0,

				-0.5, -0.5, -0.2,			//bottom face
				0.5, -0.5, -0.2,
				0.5, 0.5, -0.2,
				0.5, 0.5, -0.2,
				-0.5, 0.5, -0.2,
				-0.5, -0.5, -0.2,

				-0.5, -0.5, -0.2,			//sideface1
				0.5, -0.5, -0.2,
				0.5, -0.5, 0.0,
				0.5, -0.5, 0.0,
				-0.5, -0.5, 0.0,
				-0.5, -0.5, -0.2,

				0.5, -0.5, -0.2,			//sideface2
				0.5, 0.5, -0.2,
				0.5, 0.5, 0.0,
				0.5, 0.5, 0.0,
				0.5, -0.5, 0.0,
				0.5, -0.5, -0.2,

				0.5, 0.5, -0.2,			//sideface3
				-0.5, 0.5, -0.2,
				-0.5, 0.5, 0.0,
				-0.5, 0.5, 0.0,
				0.5, 0.5, 0.0,
				0.5, 0.5, -0.2,

				-0.5, 0.5, -0.2,			//sideface4
				-0.5, -0.5, -0.2,
				-0.5, -0.5, 0.0,
				-0.5, -0.5, 0.0,
				-0.5, 0.5, 0.0,
				-0.5, 0.5, -0.2
		};

		GLfloat color_buffer_data1[] = {
				0, 1, 1,
				0, 0, 0,
				0, 1, 1,
				0, 1, 1,
				0, 1, 1,
				0, 1, 1,

				0, 1, 1,
				1, 1, 1,
				0, 1, 1,
				0, 1, 1,
				0, 1, 1,
				0, 1, 1,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0
		};
		GLfloat color_buffer_data2[] = {
				1, 0, 0,
				1, 1, 1,
				1, 0, 0,
				1, 0, 0,
				1, 0, 0,
				1, 0, 0,

				1, 0, 0,
				1, 1, 1,
				1, 0, 0,
				1, 0, 0,
				1, 0, 0,
				1, 0, 0,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0
		};
		GLfloat color_buffer_data_1[] = {
				0, 1, 0,
				1, 1, 1,
				0, 1, 0,
				0, 1, 0,
				0, 1, 0,
				0, 1, 0,

				0, 1, 0,
				1, 1, 1,
				0, 1, 0,
				0, 1, 0,
				0, 1, 0,
				0, 1, 0,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0
		};
		GLfloat color_buffer_data3 [] = {
				0.5, 0, 0.5,
				1, 1, 1,
				0.5, 0, 0.5,
				0.5, 0, 0.5,
				0.5, 0, 0.5,
				0.5, 0, 0.5,

				0.5, 0, 0.5,
				1, 1, 1,
				0.5, 0, 0.5,
				0.5, 0, 0.5,
				0.5, 0, 0.5,
				0.5, 0, 0.5,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0
		};
		GLfloat color_buffer_data4 [] = {
				0, 0, 0.6,
				1, 1, 1,
				0, 0, 0.6,
				0, 0, 0.6,
				0, 0, 0.6,
				0, 0, 0.6,

				0, 0, 0.6,
				1, 1, 1,
				0, 0, 0.6,
				0, 0, 0.6,
				0, 0, 0.6,
				0, 0, 0.6,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0
		};

		GLfloat color_buffer_data5 [] = {
				0.8, 0.8, 1,
				1, 1, 1,
				0.8, 0.8, 1,
				0.8, 0.8, 1,
				0.8, 0.8, 1,
				0.8, 0.8, 1,

				0.8, 0.8, 1,
				1, 1, 1,
				0.8, 0.8, 1,
				0.8, 0.8, 1,
				0.8, 0.8, 1,
				0.8, 0.8, 1,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0
		};

		tileNode temp;
		temp.type = type;
		temp.activated = 1;
		if(type == 1)
				temp.tile = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data1, GL_FILL);
		else if(type == 2)
		{
				temp.tile = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data2, GL_FILL);
				goal.x = -4.5 + (float)x;
				goal.y = 4.5 - (float)y;
				goal.z = 0;
		}
		else if(type == -1)
		{
				temp.tile = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data_1, GL_FILL);
				start.x = -4.5 + (float)x;
				start.y = 4.5 - (float)y;
				start.z = 0;
		}
		else if(type == 3)
		{
				temp.tile = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data3, GL_FILL);
				temp.activated = 0;
		}
		else if(type == 4)
		{
				temp.tile = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data4, GL_FILL);	
		}
		else if(type == 5)
		{
				temp.tile = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data5, GL_FILL);
		}
		temp.translationVector = glm::translate(glm::vec3(-4.5f + (float)x, 4.5f - (float)y, 0));
		temp.xpos = x;
		temp.ypos = y;
		tiles.push_back(temp);
}

void createBase()
{
		for(int i = 0; i < 10; i++)
		{
				for(int j = 0; j < 10; j++)
				{
						if(levelMatrix[levels][i][j] != 0)
								createTile(levelMatrix[levels][i][j], i, j);
				}
		}
}

void createBlock()
{
		GLfloat vertex_buffer_data[] = {
				-0.5, -0.5, 0.5,
				0.5, -0.5, 0.5,
				0.5, 0.5, 0.5,
				0.5, 0.5, 0.5,
				-0.5, 0.5, 0.5,
				-0.5, -0.5, 0.5,

				-0.5, -0.5, -0.5,
				0.5, -0.5, -0.5,
				0.5, 0.5, -0.5,
				0.5, 0.5, -0.5,
				-0.5, 0.5, -0.5,
				-0.5, -0.5, -0.5,

				-0.5, -0.5, -0.5,
				0.5, -0.5, -0.5,
				0.5, -0.5, 0.5,
				0.5, -0.5, 0.5,
				-0.5, -0.5, 0.5,
				-0.5, -0.5, -0.5,

				0.5, -0.5, -0.5,
				0.5, 0.5, -0.5,
				0.5, 0.5, 0.5,
				0.5, 0.5, 0.5,
				0.5, -0.5, 0.5,
				0.5, -0.5, -0.5,

				0.5, 0.5, -0.5,
				-0.5, 0.5, -0.5,
				-0.5, 0.5, 0.5,
				-0.5, 0.5, 0.5,
				0.5, 0.5, 0.5,
				0.5, 0.5, -0.5,

				-0.5, 0.5, -0.5,
				-0.5, -0.5, -0.5,
				-0.5, -0.5, 0.5,
				-0.5, -0.5, 0.5,
				-0.5, 0.5, 0.5,
				-0.5, 0.5, -0.5
		};
		GLfloat color_buffer_data1[] = {
				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,

				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,

				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,

				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,

				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,

				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06,
				0.87, 0.21, 0.06
		};
		GLfloat color_buffer_data2 [] = {
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,

				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0
		};
		blocks[0].block = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data1, GL_FILL);
		blocks[1].block = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data1, GL_FILL);
		blocks[0].translationVector = glm::translate(glm::vec3(start.x, start.y, 0.5f));
		blocks[1].translationVector = glm::translate(glm::vec3(start.x, start.y, 1.5f));
		blocks[0].rotationVector = glm::rotate(0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
		blocks[1].rotationVector = glm::rotate(0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
		blocks[0].xpos = (int)(start.x + 4.5);
		blocks[1].xpos = (int)(start.x + 4.5);
		blocks[0].ypos = (int)(4.5 - start.y);
		blocks[1].ypos = (int)(4.5 - start.y);
}

int checkActivated(int y, int x)
{
		for(vector<tileNode>::iterator it = tiles.begin(); it != tiles.end(); it++)
		{
				if(it->xpos == x && it->ypos == y && it->type == 3)
				{
						if(it->activated == 1)
								return 1;
						else
								return 0;
				}
		}
}

void moveUp()
{
		if(top != -1)	//along z axis
		{
				rotLock = true;
				upLock[2] = true;
		}
		else			//along x or y axis
		{
				if(blocks[0].xpos == blocks[1].xpos)	//along y axis
				{
						rotLock = true;
						upLock[1] = true;
				}
				else if(blocks[0].ypos == blocks[1].ypos)	//along x axis
				{
						rotLock = true;
						upLock[0] = true;
				}
		}
}

void moveDown()
{
		if(top != -1)	//along z axis
		{
				rotLock = true;
				downLock[2] = true;
		}
		else			//along x or y axis
		{
				if(blocks[0].xpos == blocks[1].xpos)	//along y axis
				{
						rotLock = true;
						downLock[1] = true;
				}
				else if(blocks[0].ypos == blocks[1].ypos)	//along x axis
				{
						rotLock = true;
						downLock[0] = true;
				}
		}
}

void moveRight()
{
		if(top != -1)	//along z axis
		{
				rotLock = true;
				rightLock[2] = true;
		}
		else			//along x or y axis
		{
				if(blocks[0].xpos == blocks[1].xpos)	//along y axis
				{
						rotLock = true;
						rightLock[1] = true;
				}
				else if(blocks[0].ypos == blocks[1].ypos)	//along x axis
				{
						rotLock = true;
						rightLock[0] = true;
				}
		}
}

void moveLeft()
{
		if(top != -1)	//along z axis
		{
				rotLock = true;
				leftLock[2] = true;
		}
		else			//along x or y axis
		{
				if(blocks[0].xpos == blocks[1].xpos)	//along y axis
				{
						rotLock = true;
						leftLock[1] = true;
				}
				else if(blocks[0].ypos == blocks[1].ypos)	//along x axis
				{
						rotLock = true;
						leftLock[0] = true;
				}
		}
}

int timer = 0;

void rotate()
{
		int completeRotation = 0;
		if(timer >= 85 && rotLock)
		{
				completeRotation = 1;
				timer = 0;
		}
		if(completeRotation == 0 && rotLock)		//rotation still happening
		{
				float transx=0, transy=0, transz=0, angle=0, axisx=0, axisy=0, axisz=0;
				timer += 5;
				if(upLock[0])				//up, block along x axis
				{
						angle = -timer*M_PI/180.0f;
						glm::mat4 temptrans = glm::translate(glm::vec3(0, -0.5, 0.5));
						glm::mat4 temptransrev = glm::translate(glm::vec3(0, 0.5, -0.5));
						blocks[0].rotationVector = glm::rotate(angle, glm::vec3(1, 0, 0));
						blocks[1].rotationVector = glm::rotate(angle, glm::vec3(1, 0, 0));
						blocks[0].rotationVector = temptransrev * blocks[0].rotationVector * temptrans;
						blocks[1].rotationVector = temptransrev * blocks[1].rotationVector * temptrans;
				}
				else if(upLock[1])			//up, block along y axis
				{
						angle = -timer*M_PI/180.0f;
						glm::mat4 temptrans0, temptrans1,temptrans0rev, temptrans1rev;
						if(blocks[0].ypos < blocks[1].ypos)			//block 0 is up
						{
								temptrans0 = glm::translate(glm::vec3(0, -0.5, 0.5));
								temptrans0rev = glm::translate(glm::vec3(0, 0.5, -0.5));
								temptrans1 = glm::translate(glm::vec3(0, -1.5, 0.5));
								temptrans1rev = glm::translate(glm::vec3(0, 1.5, -0.5));
						}
						else
						{
								temptrans1 = glm::translate(glm::vec3(0, -0.5, 0.5));
								temptrans1rev = glm::translate(glm::vec3(0, 0.5, -0.5));
								temptrans0 = glm::translate(glm::vec3(0, -1.5, 0.5));
								temptrans0rev = glm::translate(glm::vec3(0, 1.5, -0.5));
						}
						blocks[0].rotationVector = glm::rotate(angle, glm::vec3(1, 0, 0));
						blocks[1].rotationVector = glm::rotate(angle, glm::vec3(1, 0, 0));
						blocks[0].rotationVector = temptrans0rev * blocks[0].rotationVector * temptrans0;
						blocks[1].rotationVector = temptrans1rev * blocks[1].rotationVector * temptrans1;
				}
				else if(upLock[2])			//up, block along z axis
				{
						int bottom = (top == 0) ? 1:0;
						angle = -timer*M_PI/180.0f;
						glm::mat4 temptranstop = glm::translate(glm::vec3(0, -0.5, 1.5));
						glm::mat4 temptransbottom = glm::translate(glm::vec3(0, -0.5, 0.5));
						glm::mat4 temptranstoprev = glm::translate(glm::vec3(0, 0.5, -1.5));
						glm::mat4 temptransbottomrev = glm::translate(glm::vec3(0, 0.5, -0.5));
						blocks[top].rotationVector = glm::rotate(angle, glm::vec3(1, 0, 0));
						blocks[bottom].rotationVector = glm::rotate(angle, glm::vec3(1, 0, 0));
						blocks[top].rotationVector = temptranstoprev * blocks[top].rotationVector * temptranstop;
						blocks[bottom].rotationVector = temptransbottomrev * blocks[bottom].rotationVector * temptransbottom;
				}
				else if(downLock[0])		//down, along x axis
				{
						angle = timer*M_PI/180.0f;
						glm::mat4 temptrans = glm::translate(glm::vec3(0, 0.5, 0.5));
						glm::mat4 temptransrev = glm::translate(glm::vec3(0, -0.5, -0.5));
						blocks[0].rotationVector = glm::rotate(angle, glm::vec3(1, 0, 0));
						blocks[1].rotationVector = glm::rotate(angle, glm::vec3(1, 0, 0));
						blocks[0].rotationVector = temptransrev * blocks[0].rotationVector * temptrans;
						blocks[1].rotationVector = temptransrev * blocks[1].rotationVector * temptrans;
				}
				else if(downLock[1])		//down, along y axis
				{
						angle = timer*M_PI/180.0f;
						glm::mat4 temptrans0, temptrans1,temptrans0rev, temptrans1rev;
						if(blocks[0].ypos < blocks[1].ypos)			//block 0 is up
						{
								temptrans1 = glm::translate(glm::vec3(0, 0.5, 0.5));
								temptrans1rev = glm::translate(glm::vec3(0, -0.5, -0.5));
								temptrans0 = glm::translate(glm::vec3(0, 1.5, 0.5));
								temptrans0rev = glm::translate(glm::vec3(0, -1.5, -0.5));
						}
						else
						{
								temptrans0 = glm::translate(glm::vec3(0, 0.5, 0.5));
								temptrans0rev = glm::translate(glm::vec3(0, -0.5, -0.5));
								temptrans1 = glm::translate(glm::vec3(0, 1.5, 0.5));
								temptrans1rev = glm::translate(glm::vec3(0, -1.5, -0.5));
						}
						blocks[0].rotationVector = glm::rotate(angle, glm::vec3(1, 0, 0));
						blocks[1].rotationVector = glm::rotate(angle, glm::vec3(1, 0, 0));
						blocks[0].rotationVector = temptrans0rev * blocks[0].rotationVector * temptrans0;
						blocks[1].rotationVector = temptrans1rev * blocks[1].rotationVector * temptrans1;
				}
				else if(downLock[2])		//down, along z axis
				{
						int bottom = (top == 0) ? 1:0;
						angle = timer*M_PI/180.0f;
						glm::mat4 temptranstop = glm::translate(glm::vec3(0, 0.5, 1.5));
						glm::mat4 temptransbottom = glm::translate(glm::vec3(0, 0.5, 0.5));
						glm::mat4 temptranstoprev = glm::translate(glm::vec3(0, -0.5, -1.5));
						glm::mat4 temptransbottomrev = glm::translate(glm::vec3(0, -0.5, -0.5));
						blocks[top].rotationVector = glm::rotate(angle, glm::vec3(1, 0, 0));
						blocks[bottom].rotationVector = glm::rotate(angle, glm::vec3(1, 0, 0));
						blocks[top].rotationVector = temptranstoprev * blocks[top].rotationVector * temptranstop;
						blocks[bottom].rotationVector = temptransbottomrev * blocks[bottom].rotationVector * temptransbottom;
				}
				else if(rightLock[0])		//right, along x axis
				{
						angle = timer*M_PI/180.0f;
						glm::mat4 temptrans0, temptrans1,temptrans0rev, temptrans1rev;
						if(blocks[0].xpos > blocks[1].xpos)		//block 0 is towards right
						{
								temptrans0 = glm::translate(glm::vec3(-0.5, 0, 0.5));
								temptrans0rev = glm::translate(glm::vec3(0.5, 0, -0.5));
								temptrans1 = glm::translate(glm::vec3(-1.5, 0, 0.5));
								temptrans1rev = glm::translate(glm::vec3(1.5, 0, -0.5));
						}
						else
						{
								temptrans1 = glm::translate(glm::vec3(-0.5, 0, 0.5));
								temptrans1rev = glm::translate(glm::vec3(0.5, 0, -0.5));
								temptrans0 = glm::translate(glm::vec3(-1.5, 0, 0.5));
								temptrans0rev = glm::translate(glm::vec3(1.5, 0, -0.5));
						}
						blocks[0].rotationVector = glm::rotate(angle, glm::vec3(0, 1, 0));
						blocks[1].rotationVector = glm::rotate(angle, glm::vec3(0, 1, 0));
						blocks[0].rotationVector = temptrans0rev * blocks[0].rotationVector * temptrans0;
						blocks[1].rotationVector = temptrans1rev * blocks[1].rotationVector * temptrans1;
				}
				else if(rightLock[1])		//right, along y axis
				{
						angle = timer*M_PI/180.0f;
						glm::mat4 temptrans = glm::translate(glm::vec3(-0.5, 0, 0.5));
						glm::mat4 temptransrev = glm::translate(glm::vec3(0.5, 0, -0.5));
						blocks[0].rotationVector = glm::rotate(angle, glm::vec3(0, 1, 0));
						blocks[1].rotationVector = glm::rotate(angle, glm::vec3(0, 1, 0));
						blocks[0].rotationVector = temptransrev * blocks[0].rotationVector * temptrans;
						blocks[1].rotationVector = temptransrev * blocks[1].rotationVector * temptrans;
				}
				else if(rightLock[2])		//right, along z axis
				{
						angle = timer*M_PI/180.0f;
						int bottom = (top == 0) ? 1: 0;
						glm::mat4 temptranstop = glm::translate(glm::vec3(-0.5, 0, 1.5));
						glm::mat4 temptransbottom = glm::translate(glm::vec3(-0.5, 0, 0.5));
						glm::mat4 temptranstoprev = glm::translate(glm::vec3(0.5, 0, -1.5));
						glm::mat4 temptransbottomrev = glm::translate(glm::vec3(0.5, 0, -0.5));
						blocks[top].rotationVector = glm::rotate(angle, glm::vec3(0, 1, 0));
						blocks[bottom].rotationVector = glm::rotate(angle, glm::vec3(0, 1, 0));
						blocks[top].rotationVector = temptranstoprev * blocks[top].rotationVector * temptranstop;
						blocks[bottom].rotationVector = temptransbottomrev * blocks[bottom].rotationVector * temptransbottom;
				}
				else if(leftLock[0])		//left, along x axis
				{
						angle = -timer*M_PI/180.0f;
						glm::mat4 temptrans0, temptrans1,temptrans0rev, temptrans1rev;
						if(blocks[0].xpos > blocks[1].xpos)		//block 0 is towards right
						{
								temptrans1 = glm::translate(glm::vec3(0.5, 0, 0.5));
								temptrans1rev = glm::translate(glm::vec3(-0.5, 0, -0.5));
								temptrans0 = glm::translate(glm::vec3(1.5, 0, 0.5));
								temptrans0rev = glm::translate(glm::vec3(-1.5, 0, -0.5));
						}
						else
						{
								temptrans0 = glm::translate(glm::vec3(0.5, 0, 0.5));
								temptrans0rev = glm::translate(glm::vec3(-0.5, 0, -0.5));
								temptrans1 = glm::translate(glm::vec3(1.5, 0, 0.5));
								temptrans1rev = glm::translate(glm::vec3(-1.5, 0, -0.5));
						}
						blocks[0].rotationVector = glm::rotate(angle, glm::vec3(0, 1, 0));
						blocks[1].rotationVector = glm::rotate(angle, glm::vec3(0, 1, 0));
						blocks[0].rotationVector = temptrans0rev * blocks[0].rotationVector * temptrans0;
						blocks[1].rotationVector = temptrans1rev * blocks[1].rotationVector * temptrans1;
				}
				else if(leftLock[1])		//left, along y axis
				{
						angle = -timer*M_PI/180.0f;
						glm::mat4 temptrans = glm::translate(glm::vec3(0.5, 0, 0.5));
						glm::mat4 temptransrev = glm::translate(glm::vec3(-0.5, 0, -0.5));
						blocks[0].rotationVector = glm::rotate(angle, glm::vec3(0, 1, 0));
						blocks[1].rotationVector = glm::rotate(angle, glm::vec3(0, 1, 0));
						blocks[0].rotationVector = temptransrev * blocks[0].rotationVector * temptrans;
						blocks[1].rotationVector = temptransrev * blocks[1].rotationVector * temptrans;
				}
				else if(leftLock[2])		//left, along z axis
				{
						angle = -timer*M_PI/180.0f;
						int bottom = (top == 0) ? 1: 0;
						glm::mat4 temptranstop = glm::translate(glm::vec3(0.5, 0, 1.5));
						glm::mat4 temptransbottom = glm::translate(glm::vec3(0.5, 0, 0.5));
						glm::mat4 temptranstoprev = glm::translate(glm::vec3(-0.5, 0, -1.5));
						glm::mat4 temptransbottomrev = glm::translate(glm::vec3(-0.5, 0, -0.5));
						blocks[top].rotationVector = glm::rotate(angle, glm::vec3(0, 1, 0));
						blocks[bottom].rotationVector = glm::rotate(angle, glm::vec3(0, 1, 0));
						blocks[top].rotationVector = temptranstoprev * blocks[top].rotationVector * temptranstop;
						blocks[bottom].rotationVector = temptransbottomrev * blocks[bottom].rotationVector * temptransbottom;
				}
		}
		else if(rotLock)						//rotation over, translate and restore
				//if(rotLock)
		{
				playSound(3);
				score += 1;
				//playedSound = true;
				blocks[0].rotationVector = glm::mat4(1.0f);
				blocks[1].rotationVector = glm::mat4(1.0f);
				rotLock = false;
				if(upLock[0])
				{
						upLock[0] = false;
						blocks[0].ypos -= 1;
						blocks[1].ypos -= 1;
						blocks[0].translationVector[3][1] += 1;
						blocks[1].translationVector[3][1] += 1;
						top = -1;
				}
				else if(upLock[1])
				{
						upLock[1] = false;
						if(blocks[0].ypos < blocks[1].ypos)
						{
								top = 1;
								blocks[0].ypos -= 1;
								blocks[1].ypos -= 2;
								blocks[1].translationVector[3][2] += 1;
								blocks[0].translationVector[3][2] = 0.5;
								blocks[0].translationVector[3][1] += 1;
								blocks[1].translationVector[3][1] += 2;
						}
						else
						{
								top = 0;
								blocks[1].ypos -= 1;
								blocks[0].ypos -= 2;
								blocks[0].translationVector[3][2] += 1;
								blocks[1].translationVector[3][2] = 0.5;
								blocks[1].translationVector[3][1] += 1;
								blocks[0].translationVector[3][1] += 2;
						}
				}
				else if(upLock[2])
				{
						upLock[2] = false;
						int bottom;
						if(top == 1)
								bottom = 0;
						else
								bottom = 1;
						blocks[top].ypos -= 2;
						blocks[bottom].ypos -= 1;
						blocks[top].translationVector[3][2] = blocks[bottom].translationVector[3][2] = 0.5;
						blocks[top].translationVector[3][1] += 2;
						blocks[bottom].translationVector[3][1] += 1;
						top = -1;
				}
				else if(downLock[0])
				{
						downLock[0] = false;
						blocks[0].ypos += 1;
						blocks[1].ypos += 1;
						blocks[0].translationVector[3][1] -= 1;
						blocks[1].translationVector[3][1] -= 1;
						top = -1;
				}
				else if(downLock[1])
				{
						downLock[1] = false;
						if(blocks[0].ypos < blocks[1].ypos)
						{
								top = 0;
								blocks[1].ypos += 1;
								blocks[0].ypos += 2;
								blocks[0].translationVector[3][2] += 1;
								blocks[1].translationVector[3][2] = 0.5;
								blocks[1].translationVector[3][1] -= 1;
								blocks[0].translationVector[3][1] -= 2;
						}
						else
						{
								top = 1;
								blocks[0].ypos += 1;
								blocks[1].ypos += 2;
								blocks[1].translationVector[3][2] += 1;
								blocks[0].translationVector[3][2] = 0.5;
								blocks[0].translationVector[3][1] -= 1;
								blocks[1].translationVector[3][1] -= 2;
						}
				}
				else if(downLock[2])
				{
						downLock[2] = false;
						int bottom = (top == 1)? 0 : 1;
						blocks[top].ypos += 2;
						blocks[bottom].ypos += 1;
						blocks[top].translationVector[3][2] = blocks[bottom].translationVector[3][2] = 0.5;
						blocks[top].translationVector[3][1] -= 2;
						blocks[bottom].translationVector[3][1] -= 1;
						top = -1;
				}
				else if(rightLock[0])
				{
						rightLock[0] = false;
						if(blocks[0].xpos > blocks[1].xpos)
						{
								top = 1;
								blocks[0].xpos += 1;
								blocks[1].xpos += 2;
								blocks[0].translationVector[3][0] += 1;
								blocks[1].translationVector[3][0] += 2;
								blocks[1].translationVector[3][2] += 1;
								blocks[0].translationVector[3][2] = 0.5;
						}
						else
						{
								top = 0;
								blocks[1].xpos += 1;
								blocks[0].xpos += 2;
								blocks[1].translationVector[3][0] += 1;
								blocks[0].translationVector[3][0] += 2;
								blocks[0].translationVector[3][2] += 1;
								blocks[1].translationVector[3][2] = 0.5;
						}
				}
				else if(rightLock[1])
				{
						rightLock[1] = false;
						blocks[0].xpos += 1;
						blocks[1].xpos += 1;
						blocks[0].translationVector[3][0] += 1;
						blocks[1].translationVector[3][0] += 1;
						top = -1;
				}
				else if(rightLock[2])
				{
						rightLock[2] = false;
						int bottom = (top == 1)? 0:1;
						blocks[top].xpos += 2;
						blocks[bottom].xpos += 1;
						blocks[top].translationVector[3][0] += 2;
						blocks[bottom].translationVector[3][0] += 1;
						blocks[top].translationVector[3][2] = blocks[bottom].translationVector[3][2] = 0.5;
						top = -1;
				}
				else if(leftLock[0])
				{
						if(blocks[0].xpos > blocks[1].xpos)
						{
								top = 0;
								blocks[1].xpos -= 1;
								blocks[0].xpos -= 2;
								blocks[1].translationVector[3][0] -= 1;
								blocks[0].translationVector[3][0] -= 2;
								blocks[0].translationVector[3][2] += 1;
								blocks[1].translationVector[3][2] = 0.5;
						}
						else
						{
								top = 1;
								blocks[0].xpos -= 1;
								blocks[1].xpos -= 2;
								blocks[0].translationVector[3][0] -= 1;
								blocks[1].translationVector[3][0] -= 2;
								blocks[1].translationVector[3][2] += 1;
								blocks[0].translationVector[3][2] = 0.5;
						}
						leftLock[0] = false;
				}
				else if(leftLock[1])
				{
						leftLock[1] = false;
						blocks[0].xpos -= 1;
						blocks[1].xpos -= 1;
						blocks[0].translationVector[3][0] -= 1;
						blocks[1].translationVector[3][0] -= 1;
						top = -1;
				}
				else if(leftLock[2])
				{
						leftLock[2] = false;
						int bottom = (top == 1)? 0:1;
						blocks[top].xpos -= 2;
						blocks[bottom].xpos -= 1;
						blocks[top].translationVector[3][0] -= 2;
						blocks[bottom].translationVector[3][0] -= 1;
						blocks[top].translationVector[3][2] = blocks[bottom].translationVector[3][2] = 0.5;
						top = -1;
				}
		}
		if(blockViewOn)
				blockView();
		if(followCamOn && !rotLock)
				followCam();
}

VAO* gameOverCover;
VAO* backgroundCover;
VAO* levelResetCover;

void createFallCover()
{
		GLfloat vertex_buffer_data[] = {
				-100, -100, 9,
				100, -100, 9,
				100, 100, 9,
				100, 100, 9,
				-100, 100, 9,
				-100, -100, 9
		};
		GLfloat color_buffer_data [] = {
				1, 0, 0,
				1, 0, 0,
				1, 0, 0,
				1, 0, 0,
				1, 0, 0,
				1, 0, 0
		};
		levelResetCover = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createCoverBack()
{
		GLfloat vertex_buffer_data[] = {
				-100, -100, -10,
				100, -100, -10,
				100, 100, -10,
				100, 100, -10,
				-100, 100, -10,
				-100, -100, -10
		};
		GLfloat color_buffer_data [] = {
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0,
				0, 0, 0
		};
		backgroundCover = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createCover()
{
		GLfloat vertex_buffer_data[] = {
				-100, -100, 5,
				100, -100, 5,
				100, 100, 5,
				100, 100, 5,
				-100, 100, 5,
				-100, -100, 5
		};
		GLfloat color_buffer_data [] = {
				1, 1, 1,
				1, 1, 1,
				1, 1, 1,
				1, 1, 1,
				1, 1, 1,
				1, 1, 1
		};
		gameOverCover = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void activateBridge()
{
		for(vector<tileNode>::iterator it = tiles.begin(); it != tiles.end(); it++)
		{
				if(it->type == 3)
				{
						it->activated = (it->activated == 1) ? 0 : 1;;
				}
		}
}

void gameIsOver()
{
		eye[0] = 0;
		eye[1] = 0;
		eye[2] = 10;
		up[0] = 0;
		up[1] = 1;
		up[2] = 0;
		target[0] = 0;
		target[1] = 0;
		target[2] = 0;
		heliViewOn = false;
		blockViewOn = false;

}

void checkFallen(int wayFallen)
{
		if(blocks[0].translationVector[3][2] < -11.0)
		{
				if(wayFallen == 0)
				{
						if(levels < 2)
								levels ++;
						else
						{
								gameOver = true;
								gameIsOver();
						}
				}
				if(wayFallen == 1)
				{
						fallenBlock = true;
						gameIsOver();
				}
				initLevel();
		}
}

void playSound(int type)
{
		if(!playedSound)
		{
				if(type == 1)
				{
						system("aplay -q bomb.wav &");
						system("aplay -q fall.wav &");
				}
				if(type == 2)
				{
						system("aplay -q applause.wav &");
						system("aplay -q bomb.wav &");
				}
				if(type == 3)
				{
						system("aplay -q rotate.wav &");
				}
				if(type == 4)
				{
						system("aplay -q game.wav &");
				}
		}
}

void aftermath()
{
		if(goalFlag == 1)
		{
				playSound(2);
				playedSound = true;
				blocks[0].translationVector[3][2] -= 0.1;
				blocks[1].translationVector[3][2] -= 0.1;
				checkFallen(0);
		}
		else if(fallFlag1 == 1 || fallFlag0 == 1)
		{
				playSound(1);
				playedSound = true;
				blocks[0].translationVector[3][2] -= 0.1;
				blocks[1].translationVector[3][2] -= 0.1;
				checkFallen(1);
		}
}

void checkConditions()
{
		if(top != -1)	//block along z axis, need to check for weak tiles, bridge activators, goal
		{
				int value = levelMatrix[levels][blocks[0].ypos][blocks[0].xpos];
				if(value == 2)
				{
						goalFlag = 1;
				}
				else if(value == 5 || value == 0)
				{
						fallFlag0 = 1;
						fallFlag1 = 1;
				}
				else if(value == 4)
				{
						if(activateBridgeFlag == 0 && !rotLock)
						{
								//cout << top << " " << value << endl;
								activateBridge();
								activateBridgeFlag = 1;
						}
				}
				else if(value == 3 && checkActivated(blocks[0].ypos, blocks[0].xpos) == 0)
				{
						fallFlag0 = 1;
						fallFlag1 = 1;
				}
		}
		else	//block is along either x or y axis, need to check for falling only
		{
				int value0 = levelMatrix[levels][blocks[0].ypos][blocks[0].xpos], value1 = levelMatrix[levels][blocks[1].ypos][blocks[1].xpos]; 
				//cout << value0 << " " << value1 << endl;
				if(value0 == 0 || (value0 == 3 && checkActivated(blocks[0].ypos, blocks[0].xpos) == 0) || blocks[0].ypos < 0 || blocks[0].ypos > 9 || blocks[0].xpos < 0 || blocks[0].xpos > 9)
						fallFlag0 = 1;
				else if(value1 == 0 || (value1 == 3 && checkActivated(blocks[1].ypos, blocks[1].xpos) == 0) || blocks[1].ypos < 0 || blocks[1].ypos > 9 || blocks[1].xpos < 0 || blocks[1].xpos > 9)
						fallFlag1 = 1;
				else if(value0 == 0 && value1 == 0 || ((value0 == 3 && checkActivated(blocks[0].ypos, blocks[0].xpos) == 0) && (value1 == 3 && checkActivated(blocks[1].ypos, blocks[1].xpos) == 0)))
				{
						fallFlag0 = 1;
						fallFlag1 = 1;
				}
		}
		aftermath();
}

/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (int width, int height)
{
		GLfloat fov = 108.0f;

		// sets the viewport of openGL renderer
		glViewport (0, 0, (GLsizei) width, (GLsizei) height);

		// set the projection matrix as perspective/ortho
		// Store the projection matrix in a variable for future use

		// Perspective projection for 3D views
		Matrices.projection = glm::perspective (fov, (GLfloat) width / (GLfloat) height, 0.1f, 500.0f);

		// Ortho projection for 2D views
		//Matrices.projection = glm::ortho(-6.0f, 6.0f, -6.0f, 6.0f, 0.1f, 500.0f);
}

float camera_rotation_angle = 120;

int gameOverTimer = 0;

/* Render the scene with openGL */
/* Edit this function according to your assignment */
void draw ()
{
		// clear the color and depth in the frame buffer
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// use the loaded shader program
		// Don't change unless you know what you are doing
		glUseProgram (programID);

		// Compute Camera matrix (view)
		Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
		//  Don't change unless you are sure!!
		//Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

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

		//base
		for(vector<tileNode>::iterator it = tiles.begin(); it != tiles.end(); it++)
		{
				if(it->activated)
				{
						Matrices.model = glm::mat4(1.0f);
						Matrices.model *= it->translationVector;
						MVP = VP * Matrices.model;
						glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
						draw3DObject(it->tile);
				}
		}

		//blocks
		//rotate block
		rotate();
		checkConditions();
		for(int i = 0; i < 2; i++)
		{
				Matrices.model = glm::mat4(1.0f);
				Matrices.model *= (blocks[i].translationVector * blocks[i].rotationVector);
				MVP = VP * Matrices.model;
				glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
				draw3DObject(blocks[i].block);
		}

		Matrices.model = glm::mat4(1.0f);
		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(backgroundCover);



		Matrices.view = glm::lookAt(glm::vec3(0, 0, 10), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		VP = Matrices.projection * Matrices.view;

		glm::vec3 fontColor = glm::vec3(1, 1, 1);
		glUseProgram(fontProgramID);
		Matrices.model = glm::mat4(1.0f);
		glm::mat4 translateText = glm::translate(glm::vec3(-6, 6, 0));
		glm::mat4 scaleText = glm::scale(glm::vec3( 0.5, 0.5, 0.5));
		Matrices.model *= (translateText * scaleText);
		MVP = Matrices.projection * Matrices.view * Matrices.model;
		// send font's MVP and font color to fond shaders
		glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
		glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);
		char levelString[] = {"Level :"};
		char scoreString[] = {"Moves :"};
		char toPrint[1000];
		sprintf(toPrint, "%s %d %s %d", levelString, levels + 1, scoreString, score);
		GL3Font.font->Render(toPrint);
		//fontScale = (fontScale + 1) % 360;*/
		/*glm::mat4 translateText;
		  glm::mat4 scaleText;
		  glm::vec3 fontColor;*/


		if(gameOver)
		{
				if(gameOverTimer < 100)
				{

						gameOverTimer += 2;
						glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
						Matrices.view = glm::lookAt(eye, target, up);
						VP = Matrices.projection * Matrices.view;
						Matrices.model = glm::mat4(1.0f);
						MVP = VP * Matrices.model;
						glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
						draw3DObject(gameOverCover);
						fontColor = glm::vec3(0, 0, 0);
						glUseProgram(fontProgramID);
						translateText = glm::translate(glm::vec3(-0.25, 0, 9.1));
						scaleText = glm::scale(glm::vec3( 0.05, 0.05, 0.05));
						Matrices.model = glm::mat4(1.0f);
						Matrices.model *= (translateText * scaleText);
						MVP = Matrices.projection * Matrices.view * Matrices.model;
						// send font's MVP and font color to fond shaders
						glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
						glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);
						char gameOverString [] = {"Game Over. You WON!!!! Congratulations!"};
						GL3Font.font->Render(gameOverString);

				}
				else
				{
						cout << endl << "Game Over. You WON!!!! Congratulations!" <<endl;
						exit(0);
				}
		}
		if(fallenBlock)
		{
				if(gameOverTimer < 100)
				{
						gameOverTimer += 2;
						glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
						Matrices.view = glm::lookAt(eye, target, up);
						VP = Matrices.projection * Matrices.view;
						Matrices.model = glm::mat4(1.0f);
						MVP = VP * Matrices.model;
						glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
						draw3DObject(levelResetCover);
						fontColor = glm::vec3(0, 0, 0);
						glUseProgram(fontProgramID);
						translateText = glm::translate(glm::vec3(-0.12, 0, 9.1));
						scaleText = glm::scale(glm::vec3( 0.05, 0.05, 0.05));
						Matrices.model = glm::mat4(1.0f);
						Matrices.model *= (translateText * scaleText);
						MVP = Matrices.projection * Matrices.view * Matrices.model;
						// send font's MVP and font color to fond shaders
						glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
						glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);
						char fellString [] = {"You fell! Level Reset!"};
						GL3Font.font->Render(fellString);
				}
				else
				{
						gameOverTimer = 0;
						fallenBlock = false;
						cout << endl << "You fell! Level Reset!" << endl;
				}
		}

		// Swap the frame buffers
		glutSwapBuffers ();
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
		glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE);
		glutInitContextVersion (3, 3); // Init GL 3.3
		glutInitContextFlags (GLUT_CORE_PROFILE); // Use Core profile - older functions are deprecated
		glutInitWindowSize (width, height);
		glutCreateWindow ("GAME");

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

		glutIgnoreKeyRepeat (true); // Ignore keys held down
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
		createBase();
		createBlock();
		createCover();
		createCoverBack();
		createFallCover();
		playSound(4);

		// Create and compile our GLSL program from the shaders
		programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
		// Get a handle for our "MVP" uniform
		Matrices.MatrixID = glGetUniformLocation(programID, "MVP");

		const char* fontfile = "arial.ttf";
		GL3Font.font = new FTExtrudeFont(fontfile); // 3D extrude style rendering

		if(GL3Font.font->Error())
		{
				cout << "Error: Could not load font `" << fontfile << "'" << endl;
				exit(EXIT_FAILURE);
		}

		fontProgramID = LoadShaders( "fontrender.vert", "fontrender.frag" );
		GLint fontVertexCoordAttrib, fontVertexNormalAttrib, fontVertexOffsetUniform;
		fontVertexCoordAttrib = glGetAttribLocation(fontProgramID, "vertexPosition");
		fontVertexNormalAttrib = glGetAttribLocation(fontProgramID, "vertexNormal");
		fontVertexOffsetUniform = glGetUniformLocation(fontProgramID, "pen");
		GL3Font.fontMatrixID = glGetUniformLocation(fontProgramID, "MVP");
		GL3Font.fontColorID = glGetUniformLocation(fontProgramID, "fontColor");

		GL3Font.font->ShaderLocations(fontVertexCoordAttrib, fontVertexNormalAttrib, fontVertexOffsetUniform);
		GL3Font.font->FaceSize(1);
		GL3Font.font->Depth(0);
		GL3Font.font->Outset(0, 0);
		GL3Font.font->CharMap(ft_encoding_unicode);



		reshapeWindow (width, height);

		// Background color of the scene
		glClearColor (0.3f, 0.3f, 0.3f, 0.0f); // R, G, B, A
		glClearDepth (1.0f);

		glEnable (GL_DEPTH_TEST);
		glDepthFunc (GL_LEQUAL);

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
