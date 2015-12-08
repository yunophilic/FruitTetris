/*=======CMPT 361 Assignment 1 - FruitTetris implementation=======*/

/*Name: Ivan Jonathan Hoo*/
/*Student #: 301251368*/
/*Email: ihoo@sfu.ca*/

#include "include/Angel.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector> //for colorIntVector

using namespace std;

//speed (in ms)
const int FALL_SPEED = 250;

//score
int score = 0;

// xsize and ysize represent the window size - updated if window is reshaped to prevent stretching of the game
int xsize = 400; 
int ysize = 720;

//index of allRotationXshape array
int rotationIndex=0;

//special global variable
int iteration=0;

// current tile
vec2 tile[4]; // An array of 4 2d vectors representing displacement from a 'center' piece of the tile, on the grid
vec2 tilepos = vec2(5, 19); // The position of the current tile using grid coordinates ((0,0) is the bottom left corner)

// An array storing all possible orientations of all possible tiles
// The 'tile' array will always be some element [i][j] of this array (an array of vec2)

//"L" tile
vec2 allRotationsLshape[4][4] = 
	{{vec2(-1,-1), vec2(-1,0), vec2(0,0), vec2(1,0)},
	{vec2(1,-1), vec2(0,-1), vec2(0,0), vec2(0,1)},     
	{vec2(1,1), vec2(1,0), vec2(0,0), vec2(-1,0)},  
	{vec2(-1,1), vec2(0,1), vec2(0,0), vec2(0,-1)}};
//"I" tile
vec2 allRotationsIshape[2][4] =
	{{vec2(-2,0), vec2(-1,0), vec2(0,0), vec2(1,0)},
	{vec2(0,-2), vec2(0,-1), vec2(0,0), vec2(0,1)}};
//"S" tile
vec2 allRotationsSshape[2][4] =
	{{vec2(-1,-1), vec2(0,-1), vec2(0,0), vec2(1,0)},
	{vec2(1,-1), vec2(1,0), vec2(0,0), vec2(0,1)}};

// colors
/*============fruit colors============*/
vec4 purple = vec4(1.0, 0.0, 1.0, 1.0);
vec4 red = vec4(1.0, 0.0, 0.0, 1.0);
vec4 yellow = vec4(1.0, 1.0, 0.0, 1.0);
vec4 green = vec4(0.0, 1.0, 0.0, 1.0);
vec4 orange = vec4(1.0, 0.5, 0.0, 1.0);
/*====================================*/
vec4 white  = vec4(1.0, 1.0, 1.0, 1.0);
vec4 black  = vec4(0.0, 0.0, 0.0, 1.0);

//board[x][y] represents whether the cell (x,y) is occupied
bool board[10][20];

//occupied coordinate array represents coordinate currently occupied by tile (MY OWN ARRAY)
vec2 occupiedCoor[4];

//colour of current tile from a to d
int currentTileColor[4]; //-1 represents black, 0 represents purple, 1 represents red, 2 represents yellow, 3 represents green, 4 represents orange

//cells colours 2D array storing colour of each cell
int cellsColours[10][20]; //-1 represents black, 0 represents purple, 1 represents red, 2 represents yellow, 3 represents green, 4 represents orange

//An array containing the colour of each of the 10*20*2*3 vertices that make up the board
//Initially, all will be set to black. As tiles are placed, sets of 6 vertices (2 triangles; 1 square)
//will be set to the appropriate colour in this array before updating the corresponding VBO
vec4 boardcolours[1200];

// location of vertex attributes in the shader program
GLuint vPosition;
GLuint vColor;

// locations of uniform variables in shader program
GLuint locxsize;
GLuint locysize;

// VAO and VBO
GLuint vaoIDs[3]; // One VAO for each object: the grid, the board, the current piece
GLuint vboIDs[6]; // Two Vertex Buffer Objects for each VAO (specifying vertex positions and colours, respectively)

//-------------------------------------------------------------------------------------------------------------------

void testFunc() //testing function
{
	//output board arr
	cout<<"board arr:\n";
	for(int a=0; a<10; a++)
	{
		for(int b=0; b<20; b++)
			cout<<board[a][b]<<" ";
		cout<<endl;
	}
	cout<<endl;

	//output cellsColours arr
	cout<<"cellsColours arr:\n";
	for(int a=0; a<10; a++)
	{
		for(int b=0; b<20; b++)
		{
			if(cellsColours[a][b]>-1)
				cout<<" ";
			cout<<cellsColours[a][b]<<" ";
		}
		cout<<endl;
	}
	cout<<endl;

	//output itr
	cout<<"itr: "<<iteration<<endl<<endl;
}

//-------------------------------------------------------------------------------------------------------------------

// When the current tile is moved or rotated (or created), update the VBO containing its vertex position data
void updatetile()
{
	// Bind the VBO containing current tile vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[4]); 

	// For each of the 4 'cells' of the tile,
	for (int i = 0; i < 4; i++) 
	{
		// Calculate the grid coordinates of the cell
		GLfloat x = tilepos.x + tile[i].x; 
		GLfloat y = tilepos.y + tile[i].y;

		occupiedCoor[i]=vec2(x,y);

		// Create the 4 corners of the square - these vertices are using location in pixels
		// These vertices are later converted by the vertex shader
		vec4 p1 = vec4(33.0 + (x * 33.0), 33.0 + (y * 33.0), .4, 1); 
		vec4 p2 = vec4(33.0 + (x * 33.0), 66.0 + (y * 33.0), .4, 1);
		vec4 p3 = vec4(66.0 + (x * 33.0), 33.0 + (y * 33.0), .4, 1);
		vec4 p4 = vec4(66.0 + (x * 33.0), 66.0 + (y * 33.0), .4, 1);

		// Two points are used by two triangles each
		vec4 newpoints[6] = {p1, p2, p3, p2, p3, p4}; 

		// Put new data in the VBO
		glBufferSubData(GL_ARRAY_BUFFER, i*6*sizeof(vec4), 6*sizeof(vec4), newpoints); 
	}

	glBindVertexArray(0);
}

//-------------------------------------------------------------------------------------------------------------------

int tileType; //1 represents "I" tile, 2 represents "S" tile, 3 represents "L" tile

int tileInitXpos; //tile initial pos

bool disableMoveTile=false;

bool gameOver=false;

// Called at the start of play and every time a tile is placed
void newtile()
{
	if(gameOver)
		return;

	iteration=0;

	tileType = rand() % 3 + 1; //select tile randomly (either I, S, or L)

	//select tile initial position and determine rotation shape randomly
	if (tileType==1) //"I" tile
	{
		rotationIndex = rand() % 2;
		tileInitXpos = rand() % 7 + 2;
	}
	if (tileType==2) //"S" tile
	{
		rotationIndex = rand() % 2;
		tileInitXpos = rand() % 8 + 1;
	}
	if (tileType==3) //"L" tile
	{
		rotationIndex = rand() % 4;
		tileInitXpos = rand() % 8 + 1;
	}

	int tileInitYpos=19;
	if(rotationIndex>0)
		tileInitYpos=18;
	tilepos = vec2( tileInitXpos, tileInitYpos ); // Put the tile at the top of the board

	// Update the geometry VBO of current tile
	if (tileType==1) //"I" tile
		for (int i = 0; i < 4; i++)
			tile[i] = allRotationsIshape[rotationIndex][i]; // Get the 4 pieces of the new tile
	if (tileType==2) //"S" tile
		for (int i = 0; i < 4; i++)
			tile[i] = allRotationsSshape[rotationIndex][i]; // Get the 4 pieces of the new tile
	if (tileType==3) //"L" tile
		for (int i = 0; i < 4; i++)
			tile[i] = allRotationsLshape[rotationIndex][i]; // Get the 4 pieces of the new tile

	for (int i = 0; i < 4; i++) //if there is occupied cell in any of the new tile initial position, increase the new tile initial y-position by 1.0 (need to make any blocks of the new tile outside the upper y-range of the board to have black color)
	{
		int X=tilepos.x + tile[i].x;
		int Y=tilepos.y + tile[i].y;
		if(board[X][Y]==true)
		{
			disableMoveTile=true;
			tilepos.y+=1.0;
			break;
		}
	}

	updatetile();

	// Update the color VBO of current tile

	vec4 newcolours[24];

	//My own variables
	int boxColorInt = -33; //-1 represents black, 0 represents purple, 1 represents red, 2 represents yellow, 3 represents green, 4 represents orange (-33 is just for initialization)
	int selectedIndex = -66; //-66 is just for initialization
	vector<int> colorIntVector; //this not the same as mathematical vector (this is C++ vector data structure)
	for(int i=0;i<5;i++)
		colorIntVector.push_back(i);
	vec4 selectedColor;
	int indexArr=0;

	for (int i = 0; i < 24; i++)
	{
		if( i%6 == 0 ) //select color for one box randomly
		{
			//make sure each box have different colors
			int colorIntVectorSize=colorIntVector.size();
			selectedIndex = rand() % colorIntVectorSize;
			boxColorInt = colorIntVector[selectedIndex];

			//assign color
			if(boxColorInt==0)
				selectedColor = purple;
			if(boxColorInt==1)
				selectedColor = red;
			if(boxColorInt==2)
				selectedColor = yellow;
			if(boxColorInt==3)
				selectedColor = green;
			if(boxColorInt==4)
				selectedColor = orange;

			currentTileColor[indexArr] = boxColorInt;

			if(occupiedCoor[indexArr].y>19.0) //make sure that blocks outside the upper y-range of the board is colored black (only happens when there is occupied cells in new tile initial position and we increase the new tile y-position by 1.0)
			{
				currentTileColor[indexArr]=-1;
				selectedColor=black;
			}

			indexArr++;

			//removes selected element
			colorIntVector.erase(colorIntVector.begin()+selectedIndex);
		}
		newcolours[i] = selectedColor;
	}

	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[5]); // Bind the VBO containing current tile vertex colours
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(newcolours), newcolours); // Put the colour data in the VBO
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);
}

//-------------------------------------------------------------------------------------------------------------------

void initGrid()
{
	// ***Generate geometry data
	vec4 gridpoints[64]; // Array containing the 64 points of the 32 total lines to be later put in the VBO
	vec4 gridcolours[64]; // One colour per vertex
	// Vertical lines 
	for (int i = 0; i < 11; i++){
		gridpoints[2*i] = vec4((33.0 + (33.0 * i)), 33.0, 0, 1);
		gridpoints[2*i + 1] = vec4((33.0 + (33.0 * i)), 693.0, 0, 1);
		
	}
	// Horizontal lines
	for (int i = 0; i < 21; i++){
		gridpoints[22 + 2*i] = vec4(33.0, (33.0 + (33.0 * i)), 0, 1);
		gridpoints[22 + 2*i + 1] = vec4(363.0, (33.0 + (33.0 * i)), 0, 1);
	}
	// Make all grid lines white
	for (int i = 0; i < 64; i++)
		gridcolours[i] = white;


	// *** set up buffer objects
	// Set up first VAO (representing grid lines)
	glBindVertexArray(vaoIDs[0]); // Bind the first VAO
	glGenBuffers(2, vboIDs); // Create two Vertex Buffer Objects for this VAO (positions, colours)

	// Grid vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[0]); // Bind the first grid VBO (vertex positions)
	glBufferData(GL_ARRAY_BUFFER, 64*sizeof(vec4), gridpoints, GL_STATIC_DRAW); // Put the grid points in the VBO
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0); 
	glEnableVertexAttribArray(vPosition); // Enable the attribute
	
	// Grid vertex colours
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[1]); // Bind the second grid VBO (vertex colours)
	glBufferData(GL_ARRAY_BUFFER, 64*sizeof(vec4), gridcolours, GL_STATIC_DRAW); // Put the grid colours in the VBO
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor); // Enable the attribute
}


void initBoard()
{
	// *** Generate the geometric data
	vec4 boardpoints[1200];
	for (int i = 0; i < 1200; i++)
		boardcolours[i] = black; // Let the empty cells on the board be black
	// Each cell is a square (2 triangles with 6 vertices)
	for (int i = 0; i < 20; i++){
		for (int j = 0; j < 10; j++)
		{		
			vec4 p1 = vec4(33.0 + (j * 33.0), 33.0 + (i * 33.0), .5, 1);
			vec4 p2 = vec4(33.0 + (j * 33.0), 66.0 + (i * 33.0), .5, 1);
			vec4 p3 = vec4(66.0 + (j * 33.0), 33.0 + (i * 33.0), .5, 1);
			vec4 p4 = vec4(66.0 + (j * 33.0), 66.0 + (i * 33.0), .5, 1);
			
			// Two points are reused
			boardpoints[6*(10*i + j)    ] = p1;
			boardpoints[6*(10*i + j) + 1] = p2;
			boardpoints[6*(10*i + j) + 2] = p3;
			boardpoints[6*(10*i + j) + 3] = p2;
			boardpoints[6*(10*i + j) + 4] = p3;
			boardpoints[6*(10*i + j) + 5] = p4;
		}
	}

	// Initially no cell is occupied
	for (int i = 0; i < 10; i++)
		for (int j = 0; j < 20; j++)
			board[i][j] = false; 

	// Initially all cells are black
	for (int i = 0; i < 10; i++)
		for (int j = 0; j < 20; j++)
			cellsColours[i][j] = -1; 

	// *** set up buffer objects
	glBindVertexArray(vaoIDs[1]);
	glGenBuffers(2, &vboIDs[2]);

	// Grid cell vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[2]);
	glBufferData(GL_ARRAY_BUFFER, 1200*sizeof(vec4), boardpoints, GL_STATIC_DRAW);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	// Grid cell vertex colours
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[3]);
	glBufferData(GL_ARRAY_BUFFER, 1200*sizeof(vec4), boardcolours, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor);
}

// No geometry for current tile initially
void initCurrentTile()
{
	glBindVertexArray(vaoIDs[2]);
	glGenBuffers(2, &vboIDs[4]);

	// Current tile vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[4]);
	glBufferData(GL_ARRAY_BUFFER, 24*sizeof(vec4), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	// Current tile vertex colours
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[5]);
	glBufferData(GL_ARRAY_BUFFER, 24*sizeof(vec4), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor);
}

void init()
{
	disableMoveTile=false;
	gameOver=false;
	iteration=0;
	score=0;

	// Load shaders and use the shader program
	GLuint program = InitShader("vshader.glsl", "fshader.glsl");
	glUseProgram(program);

	// Get the location of the attributes (for glVertexAttribPointer() calls)
	vPosition = glGetAttribLocation(program, "vPosition");
	vColor = glGetAttribLocation(program, "vColor");

	// Create 3 Vertex Array Objects, each representing one 'object'. Store the names in array vaoIDs
	glGenVertexArrays(3, &vaoIDs[0]);

	// Initialize the grid, the board, and the current tile
	initGrid();
	initBoard();
	initCurrentTile();

	// The location of the uniform variables in the shader program
	locxsize = glGetUniformLocation(program, "xsize"); 
	locysize = glGetUniformLocation(program, "ysize");

	// Game initialization
	newtile(); // create new next tile

	// set to default
	glBindVertexArray(0);
	glClearColor(0, 0, 0, 0);
}

//-------------------------------------------------------------------------------------------------------------------

void shuffleCurrTileColour() //a‐>b, b‐>c, c‐>d, d‐>a (MY OWN FUNCTION)
{
	//-1 represents black, 0 represents purple, 1 represents red, 2 represents yellow, 3 represents green, 4 represents orange
	int aColor=currentTileColor[0];
	int bColor=currentTileColor[1];
	int cColor=currentTileColor[2];
	int dColor=currentTileColor[3];

	//shuffle!!!
	currentTileColor[0]=dColor;
	currentTileColor[1]=aColor;
	currentTileColor[2]=bColor;
	currentTileColor[3]=cColor;

	// Update the color VBO of current tile
	vec4 newcolours[24];
	vec4 selectedColor;
	int indexArr=0, boxColorInt=-33;
	for (int i = 0; i < 24; i++)
	{
		if( i%6 == 0 ) //select color for one box randomly
		{
			//assign color
			boxColorInt = currentTileColor[indexArr];
			if(boxColorInt==0)
				selectedColor = purple;
			if(boxColorInt==1)
				selectedColor = red;
			if(boxColorInt==2)
				selectedColor = yellow;
			if(boxColorInt==3)
				selectedColor = green;
			if(boxColorInt==4)
				selectedColor = orange;
			indexArr++;
		}
		newcolours[i] = selectedColor;
	}

	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[5]); // Bind the VBO containing current tile vertex colours
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(newcolours), newcolours); // Put the colour data in the VBO
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);
}

void reverseTileColour() //for "I" and "S" tile rotational purpose only since you ask for 90 degree rotation!
{
	//-1 represents black, 0 represents purple, 1 represents red, 2 represents yellow, 3 represents green, 4 represents orange
	int aColor=currentTileColor[0];
	int bColor=currentTileColor[1];
	int cColor=currentTileColor[2];
	int dColor=currentTileColor[3];

	//shuffle!!!
	currentTileColor[0]=dColor;
	currentTileColor[1]=cColor;
	currentTileColor[2]=bColor;
	currentTileColor[3]=aColor;

	// Update the color VBO of current tile
	vec4 newcolours[24];
	vec4 selectedColor;
	int indexArr=0, boxColorInt=-33;
	for (int i = 0; i < 24; i++)
	{
		if( i%6 == 0 ) //select color for one box randomly
		{
			//assign color
			boxColorInt = currentTileColor[indexArr];
			if(boxColorInt==0)
				selectedColor = purple;
			if(boxColorInt==1)
				selectedColor = red;
			if(boxColorInt==2)
				selectedColor = yellow;
			if(boxColorInt==3)
				selectedColor = green;
			if(boxColorInt==4)
				selectedColor = orange;
			indexArr++;
		}
		newcolours[i] = selectedColor;
	}

	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[5]); // Bind the VBO containing current tile vertex colours
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(newcolours), newcolours); // Put the colour data in the VBO
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);
}

//-------------------------------------------------------------------------------------------------------------------

// Rotates the current tile, if there is room
void rotate()
{
	//cout<<rotationIndex<<endl;
	rotationIndex++;
	if(tileType==1) //"I" tile
	{
		bool reverseTileColourBool=false;
		if(rotationIndex>1)
		{
			rotationIndex=0;
			reverseTileColourBool=true;
			//reverseTileColour();
		}
		for (int i = 0; i < 4; i++) //check if tile can rotate
		{
			int XX = tilepos.x + allRotationsIshape[rotationIndex][i].x;
			int YY = tilepos.y + allRotationsIshape[rotationIndex][i].y;
			if(XX<=-1 || XX>9 || YY<0 || YY>19 || board[XX][YY]==true)
			{
				rotationIndex--;
				//reverseTileColourBool=false;
				return;
			}
		}
		if(reverseTileColourBool)
			reverseTileColour();
		for (int i = 0; i < 4; i++)
			tile[i] = allRotationsIshape[rotationIndex][i]; // Get the 4 pieces of the new tile
	}
	if(tileType==2) //"S" tile
	{
		bool reverseTileColourBool=false;
		if(rotationIndex>1)
		{
			rotationIndex=0;
			reverseTileColourBool=true;
			//reverseTileColour();
		}
		for (int i = 0; i < 4; i++) //check if tile can rotate
		{
			int XX = tilepos.x + allRotationsSshape[rotationIndex][i].x;
			int YY = tilepos.y + allRotationsSshape[rotationIndex][i].y;
			if(XX<=-1 || XX>9 || YY<0 || YY>19 || board[XX][YY]==true)
			{
				rotationIndex--;
				return;
			}
		}
		if(reverseTileColourBool)
			reverseTileColour();
		for (int i = 0; i < 4; i++)
			tile[i] = allRotationsSshape[rotationIndex][i]; // Get the 4 pieces of the new tile
	}
	if(tileType==3) //"L" tile
	{
		if(rotationIndex>3)
			rotationIndex=0;
		for (int i = 0; i < 4; i++) //check if tile can rotate
		{
			int XX = tilepos.x + allRotationsLshape[rotationIndex][i].x;
			int YY = tilepos.y + allRotationsLshape[rotationIndex][i].y;
			if(XX<=-1 || XX>9 || YY<0 || YY>19 || board[XX][YY]==true)
			{
				rotationIndex--;
				return;
			}
		}
		for (int i = 0; i < 4; i++)
			tile[i] = allRotationsLshape[rotationIndex][i]; // Get the 4 pieces of the new tile
	}
	updatetile();
}

//-------------------------------------------------------------------------------------------------------------------

//change boardcolours array in desired index into desired colour
void updateBoardColours(int y, int x, vec4 desireColour)
{
	boardcolours[6*(10*y + x)    ] = desireColour;
	boardcolours[6*(10*y + x) + 1] = desireColour;
	boardcolours[6*(10*y + x) + 2] = desireColour;
	boardcolours[6*(10*y + x) + 3] = desireColour;
	boardcolours[6*(10*y + x) + 4] = desireColour;
	boardcolours[6*(10*y + x) + 5] = desireColour;
}

//-------------------------------------------------------------------------------------------------------------------

// Checks if the specified row (0 is the bottom 19 the top) is full
// If every cell in the row is occupied, it will clear that cell and everything above it will shift down one row
void checkfullrow(int row)
{
	//Check if there is any empty cells in the specified row
	for(int i=0;i<10;i++)
		if(board[i][row]==false)
			return;

	score+=3;

	//shift everything down one row
	for(int b=row; b<19; b++)
		for(int a=0; a<10; a++)
		{
			board[a][b]=board[a][b+1];
			cellsColours[a][b]=cellsColours[a][b+1];
		}
	for(int a=0; a<10; a++)
	{
		board[a][19]=false;
		cellsColours[a][19]=-1;
	}

	//================UPDATE COLOR ON BOARD================

	// *** Generate the geometric data
	vec4 boardpoints[1200];

	//color occupied coordinates their corresponding color (NEW)
	for (int b = 0; b < 20; b++)
		for (int a = 0; a < 10; a++)
		{
			if(board[a][b]==true)
			{
				int colorCode=cellsColours[a][b];
				//-1 represents black, 0 represents purple, 1 represents red, 2 represents yellow, 3 represents green, 4 represents orange
				if(colorCode==0)
					updateBoardColours(b, a, purple);
				if(colorCode==1)
					updateBoardColours(b, a, red);
				if(colorCode==2)
					updateBoardColours(b, a, yellow);
				if(colorCode==3)
					updateBoardColours(b, a, green);
				if(colorCode==4)
					updateBoardColours(b, a, orange);
			}
			else
				updateBoardColours(b, a, black);
		}

	// Each cell is a square (2 triangles with 6 vertices)
	for (int i = 0; i < 20; i++)
	{
		for (int j = 0; j < 10; j++)
		{		
			vec4 p1 = vec4(33.0 + (j * 33.0), 33.0 + (i * 33.0), .5, 1);
			vec4 p2 = vec4(33.0 + (j * 33.0), 66.0 + (i * 33.0), .5, 1);
			vec4 p3 = vec4(66.0 + (j * 33.0), 33.0 + (i * 33.0), .5, 1);
			vec4 p4 = vec4(66.0 + (j * 33.0), 66.0 + (i * 33.0), .5, 1);
			
			// Two points are reused
			boardpoints[6*(10*i + j)    ] = p1;
			boardpoints[6*(10*i + j) + 1] = p2;
			boardpoints[6*(10*i + j) + 2] = p3;
			boardpoints[6*(10*i + j) + 3] = p2;
			boardpoints[6*(10*i + j) + 4] = p3;
			boardpoints[6*(10*i + j) + 5] = p4;
		}
	}

	// *** set up buffer objects
	glBindVertexArray(vaoIDs[1]);
	glGenBuffers(2, &vboIDs[2]);

	// Grid cell vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[2]);
	glBufferData(GL_ARRAY_BUFFER, 1200*sizeof(vec4), boardpoints, GL_STATIC_DRAW);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	// Grid cell vertex colours
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[3]);
	glBufferData(GL_ARRAY_BUFFER, 1200*sizeof(vec4), boardcolours, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor);

	//=====================================================
}

//-------------------------------------------------------------------------------------------------------------------

void checkThreeSameColorRow(int row) //check three consecutive same color cells for the row specified and delete the cells if it exists
{
	bool noSimilarColor=true;
	int startingColnIndex=-1;
	int endingColnIndex=-1;
	vec4 prevColor;
	for(int i=0; i<8; i++)
		if(cellsColours[i][row]!=-1)
			if( cellsColours[i][row] == cellsColours[i+1][row] && 
				cellsColours[i+1][row] == cellsColours[i+2][row] )
			{
				noSimilarColor=false;
				startingColnIndex=i;
				endingColnIndex=i+2;
				break;
			}

	if(noSimilarColor)
		return;

	score++;

	//shift down one row
	for(int b=row; b<19; b++)
		for(int a=startingColnIndex; a<endingColnIndex+1; a++)
		{
			board[a][b]=board[a][b+1];
			cellsColours[a][b]=cellsColours[a][b+1];
		}
	for(int a=startingColnIndex; a<endingColnIndex+1; a++)
	{
		board[a][19]=false;
		cellsColours[a][19]=-1;
	}

	//================UPDATE COLOR ON BOARD================

	// *** Generate the geometric data
	vec4 boardpoints[1200];

	//color occupied coordinates their corresponding color (NEW)
	for (int b = 0; b < 20; b++)
		for (int a = 0; a < 10; a++)
		{
			if(board[a][b]==true)
			{
				int colorCode=cellsColours[a][b];
				//-1 represents black, 0 represents purple, 1 represents red, 2 represents yellow, 3 represents green, 4 represents orange
				if(colorCode==0)
					updateBoardColours(b, a, purple);
				if(colorCode==1)
					updateBoardColours(b, a, red);
				if(colorCode==2)
					updateBoardColours(b, a, yellow);
				if(colorCode==3)
					updateBoardColours(b, a, green);
				if(colorCode==4)
					updateBoardColours(b, a, orange);
			}
			else
				updateBoardColours(b, a, black);
		}

	// Each cell is a square (2 triangles with 6 vertices)
	for (int i = 0; i < 20; i++)
	{
		for (int j = 0; j < 10; j++)
		{		
			vec4 p1 = vec4(33.0 + (j * 33.0), 33.0 + (i * 33.0), .5, 1);
			vec4 p2 = vec4(33.0 + (j * 33.0), 66.0 + (i * 33.0), .5, 1);
			vec4 p3 = vec4(66.0 + (j * 33.0), 33.0 + (i * 33.0), .5, 1);
			vec4 p4 = vec4(66.0 + (j * 33.0), 66.0 + (i * 33.0), .5, 1);
			
			// Two points are reused
			boardpoints[6*(10*i + j)    ] = p1;
			boardpoints[6*(10*i + j) + 1] = p2;
			boardpoints[6*(10*i + j) + 2] = p3;
			boardpoints[6*(10*i + j) + 3] = p2;
			boardpoints[6*(10*i + j) + 4] = p3;
			boardpoints[6*(10*i + j) + 5] = p4;
		}
	}

	// *** set up buffer objects
	glBindVertexArray(vaoIDs[1]);
	glGenBuffers(2, &vboIDs[2]);

	// Grid cell vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[2]);
	glBufferData(GL_ARRAY_BUFFER, 1200*sizeof(vec4), boardpoints, GL_STATIC_DRAW);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	// Grid cell vertex colours
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[3]);
	glBufferData(GL_ARRAY_BUFFER, 1200*sizeof(vec4), boardcolours, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor);

	//=====================================================

}

void checkThreeSameColorColumn(int column) //check three consecutive same color cells for the column specified and delete the cells if it exists
{
	bool noSimilarColor=true;
	int startingRowIndex=-1;
	//int endingRowIndex=-1;
	vec4 prevColor;
	for(int i=0; i<18; i++)
		if(cellsColours[column][i]!=-1)
			if( cellsColours[column][i] == cellsColours[column][i+1] && 
				cellsColours[column][i+1] == cellsColours[column][i+2] )
			{
				noSimilarColor=false;
				startingRowIndex=i;
				//endingRowIndex=i+2;
				break;
			}

	if(noSimilarColor)
		return;

	score++;

	//shift a coln down 3 rows
	for(int b=startingRowIndex; b<18; b++)
	{
		board[column][b]=board[column][b+3];
		cellsColours[column][b]=cellsColours[column][b+3];
	}
	for(int b=17; b<20; b++)
	{
		board[column][b]=false;
		cellsColours[column][b]=-1;
	}

	//================UPDATE COLOR ON BOARD================

	// *** Generate the geometric data
	vec4 boardpoints[1200];

	//color occupied coordinates their corresponding color (NEW)
	for (int b = 0; b < 20; b++)
		for (int a = 0; a < 10; a++)
		{
			if(board[a][b]==true)
			{
				int colorCode=cellsColours[a][b];
				//-1 represents black, 0 represents purple, 1 represents red, 2 represents yellow, 3 represents green, 4 represents orange
				if(colorCode==0)
					updateBoardColours(b, a, purple);
				if(colorCode==1)
					updateBoardColours(b, a, red);
				if(colorCode==2)
					updateBoardColours(b, a, yellow);
				if(colorCode==3)
					updateBoardColours(b, a, green);
				if(colorCode==4)
					updateBoardColours(b, a, orange);
			}
			else
				updateBoardColours(b, a, black);
		}

	// Each cell is a square (2 triangles with 6 vertices)
	for (int i = 0; i < 20; i++)
	{
		for (int j = 0; j < 10; j++)
		{		
			vec4 p1 = vec4(33.0 + (j * 33.0), 33.0 + (i * 33.0), .5, 1);
			vec4 p2 = vec4(33.0 + (j * 33.0), 66.0 + (i * 33.0), .5, 1);
			vec4 p3 = vec4(66.0 + (j * 33.0), 33.0 + (i * 33.0), .5, 1);
			vec4 p4 = vec4(66.0 + (j * 33.0), 66.0 + (i * 33.0), .5, 1);
			
			// Two points are reused
			boardpoints[6*(10*i + j)    ] = p1;
			boardpoints[6*(10*i + j) + 1] = p2;
			boardpoints[6*(10*i + j) + 2] = p3;
			boardpoints[6*(10*i + j) + 3] = p2;
			boardpoints[6*(10*i + j) + 4] = p3;
			boardpoints[6*(10*i + j) + 5] = p4;
		}
	}

	// *** set up buffer objects
	glBindVertexArray(vaoIDs[1]);
	glGenBuffers(2, &vboIDs[2]);

	// Grid cell vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[2]);
	glBufferData(GL_ARRAY_BUFFER, 1200*sizeof(vec4), boardpoints, GL_STATIC_DRAW);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	// Grid cell vertex colours
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[3]);
	glBufferData(GL_ARRAY_BUFFER, 1200*sizeof(vec4), boardcolours, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor);

	//=====================================================
}

bool checkForAnomaliesBool=false;

void checkForAnomalies(int column) //check for tiles with no support
{
	int indexRow=-1;
	bool anomaliesExists=false;
	for(int i=1; i<20; i++)
		if(board[column][i]==true)
			if( board[column][i-1] == false )
			{
				anomaliesExists=true;
				indexRow=i;
				break;
			}

	if(!anomaliesExists)
		return;

	checkForAnomaliesBool=true;

	int k=indexRow-1;
	while(k>=0)
	{
		if(board[column][k]==true)
			break;
		k--;
	}

	int diff=indexRow-k;

	// cout<<"indexRow: "<<indexRow<<endl;
	// cout<<"k: "<<k<<endl;
	// cout<<"diff: "<<diff<<endl;

	//shift a coln down 'diff' rows
	int b=k+1;
	while ( (b+diff-1)<20 )
	{
		board[column][b]=board[column][b+diff-1];
		cellsColours[column][b]=cellsColours[column][b+diff-1];
		b++;
	}
	for(int c=b; c<20; c++)
	{
		board[column][c]=false;
		cellsColours[column][c]=-1;
	}

	//================UPDATE COLOR ON BOARD================

	// *** Generate the geometric data
	vec4 boardpoints[1200];

	//color occupied coordinates their corresponding color (NEW)
	for (int b = 0; b < 20; b++)
		for (int a = 0; a < 10; a++)
		{
			if(board[a][b]==true)
			{
				int colorCode=cellsColours[a][b];
				//-1 represents black, 0 represents purple, 1 represents red, 2 represents yellow, 3 represents green, 4 represents orange
				if(colorCode==0)
					updateBoardColours(b, a, purple);
				if(colorCode==1)
					updateBoardColours(b, a, red);
				if(colorCode==2)
					updateBoardColours(b, a, yellow);
				if(colorCode==3)
					updateBoardColours(b, a, green);
				if(colorCode==4)
					updateBoardColours(b, a, orange);
			}
			else
				updateBoardColours(b, a, black);
		}

	// Each cell is a square (2 triangles with 6 vertices)
	for (int i = 0; i < 20; i++)
	{
		for (int j = 0; j < 10; j++)
		{		
			vec4 p1 = vec4(33.0 + (j * 33.0), 33.0 + (i * 33.0), .5, 1);
			vec4 p2 = vec4(33.0 + (j * 33.0), 66.0 + (i * 33.0), .5, 1);
			vec4 p3 = vec4(66.0 + (j * 33.0), 33.0 + (i * 33.0), .5, 1);
			vec4 p4 = vec4(66.0 + (j * 33.0), 66.0 + (i * 33.0), .5, 1);
			
			// Two points are reused
			boardpoints[6*(10*i + j)    ] = p1;
			boardpoints[6*(10*i + j) + 1] = p2;
			boardpoints[6*(10*i + j) + 2] = p3;
			boardpoints[6*(10*i + j) + 3] = p2;
			boardpoints[6*(10*i + j) + 4] = p3;
			boardpoints[6*(10*i + j) + 5] = p4;
		}
	}

	// *** set up buffer objects
	glBindVertexArray(vaoIDs[1]);
	glGenBuffers(2, &vboIDs[2]);

	// Grid cell vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[2]);
	glBufferData(GL_ARRAY_BUFFER, 1200*sizeof(vec4), boardpoints, GL_STATIC_DRAW);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	// Grid cell vertex colours
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[3]);
	glBufferData(GL_ARRAY_BUFFER, 1200*sizeof(vec4), boardcolours, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor);

	//=====================================================
}

//-------------------------------------------------------------------------------------------------------------------

// Places the current tile - update the board vertex colour VBO and the array maintaining occupied cells
void settile()
{
	// *** Generate the geometric data
	vec4 boardpoints[1200];

	//color occupied coordinates their corresponding color as well as updating the board array
	for(int u=0;u<4;u++)
	{
		int XX = occupiedCoor[u].x;
		int YY = occupiedCoor[u].y;
		board[XX][YY]=true;
		//update cellsColours (-1 represents black, 0 represents purple, 1 represents red, 2 represents yellow, 3 represents green, 4 represents orange)
		cellsColours[XX][YY]=currentTileColor[u];
		if(currentTileColor[u]==0)
			updateBoardColours(YY, XX, purple);
		if(currentTileColor[u]==1)
			updateBoardColours(YY, XX, red);
		if(currentTileColor[u]==2)
			updateBoardColours(YY, XX, yellow);
		if(currentTileColor[u]==3)
			updateBoardColours(YY, XX, green);
		if(currentTileColor[u]==4)
			updateBoardColours(YY, XX, orange);
	}

	// Each cell is a square (2 triangles with 6 vertices)
	for (int i = 0; i < 20; i++)
	{
		for (int j = 0; j < 10; j++)
		{		
			vec4 p1 = vec4(33.0 + (j * 33.0), 33.0 + (i * 33.0), .5, 1);
			vec4 p2 = vec4(33.0 + (j * 33.0), 66.0 + (i * 33.0), .5, 1);
			vec4 p3 = vec4(66.0 + (j * 33.0), 33.0 + (i * 33.0), .5, 1);
			vec4 p4 = vec4(66.0 + (j * 33.0), 66.0 + (i * 33.0), .5, 1);
			
			// Two points are reused
			boardpoints[6*(10*i + j)    ] = p1;
			boardpoints[6*(10*i + j) + 1] = p2;
			boardpoints[6*(10*i + j) + 2] = p3;
			boardpoints[6*(10*i + j) + 3] = p2;
			boardpoints[6*(10*i + j) + 4] = p3;
			boardpoints[6*(10*i + j) + 5] = p4;
		}
	}

	// *** set up buffer objects
	glBindVertexArray(vaoIDs[1]);
	glGenBuffers(2, &vboIDs[2]);

	// Grid cell vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[2]);
	glBufferData(GL_ARRAY_BUFFER, 1200*sizeof(vec4), boardpoints, GL_STATIC_DRAW);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	// Grid cell vertex colours
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[3]);
	glBufferData(GL_ARRAY_BUFFER, 1200*sizeof(vec4), boardcolours, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor);
}

//-------------------------------------------------------------------------------------------------------------------

// Given (x,y), tries to move the tile x squares to the right and y squares down
// Returns true if the tile was successfully moved, or false if there was some issue
bool movetile(vec2 direction)
{
	if(disableMoveTile)
		return false;

	for(int i=0; i<4; i++) //check if tile is in left-most or right-most of the screen
	{
		int XX=occupiedCoor[i].x+direction.x;
		int YY=occupiedCoor[i].y+direction.y;
		if( ( occupiedCoor[i].x + direction.x ) < 0.0 || ( occupiedCoor[i].x + direction.x ) > 9.0  || ( occupiedCoor[i].y + direction.y ) == -1.0 || (board[XX][YY]==true))
			return false;
	}

	tilepos.x+=direction.x;
	tilepos.y+=direction.y;
	updatetile();
	return true;
}

//-------------------------------------------------------------------------------------------------------------------

// Starts the game over - empties the board, creates new tiles, resets line counters
void restart()
{
	init();
}

//-------------------------------------------------------------------------------------------------------------------

//function to display text into GUI
template<class T>
void displayText(T str, float x, float y) {
	stringstream ss(str);
	glRasterPos2f(x, y);
	char c;
	while(ss >> noskipws >> c) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, c);
}

//-------------------------------------------------------------------------------------------------------------------

// Draws the game
void display()
{

	glClear(GL_COLOR_BUFFER_BIT);

	glUniform1i(locxsize, xsize); // x and y sizes are passed to the shader program to maintain shape of the vertices on screen
	glUniform1i(locysize, ysize);

	glBindVertexArray(vaoIDs[1]); // Bind the VAO representing the grid cells (to be drawn first)
	glDrawArrays(GL_TRIANGLES, 0, 1200); // Draw the board (10*20*2 = 400 triangles)

	glBindVertexArray(vaoIDs[2]); // Bind the VAO representing the current tile (to be drawn on top of the board)
	glDrawArrays(GL_TRIANGLES, 0, 24); // Draw the current tile (8 triangles)

	glBindVertexArray(vaoIDs[0]); // Bind the VAO representing the grid lines (to be drawn on top of everything else)
	glDrawArrays(GL_LINES, 0, 64); // Draw the grid lines (21+11 = 32 lines)

	if(!gameOver)
	{
		glColor4f(1.0, 0.0, 0.0, 1.0);
		stringstream ss;
		ss << noskipws << "Score: " << score; //display score
		displayText(ss.str(), -0.2, -0.98);
	}
	else
	{
		glColor4f(1.0, 0.0, 0.0, 1.0);
		stringstream ss;
		ss << noskipws << "Game Over!"; //display game over text
		displayText(ss.str(), -0.315, 0.945);
		ss.clear();
		ss.str("");
		ss << noskipws << "Final Score: " << score; //display final score
		displayText(ss.str(), -0.35, -0.98);
	}

	glutSwapBuffers();
}

//-------------------------------------------------------------------------------------------------------------------

// Reshape callback will simply change xsize and ysize variables, which are passed to the vertex shader
// to keep the game the same from stretching if the window is stretched
void reshape(GLsizei w, GLsizei h)
{
	xsize = w;
	ysize = h;
	glViewport(0, 0, w, h);
}

//-------------------------------------------------------------------------------------------------------------------

// Handle arrow key keypresses
void special(int key, int x, int y)
{
	switch(key)
	{
		case GLUT_KEY_UP:
			rotate();
			break;

		case GLUT_KEY_DOWN:

			if(gameOver) //if game over
				return;

			if(iteration==0 && gameOver==false)
			{
				for(int i=0; i<4; i++)
				{
					int XXX=occupiedCoor[i].x;
					int YYY=occupiedCoor[i].y-1.0;

					if(occupiedCoor[i].y==0.0 || board[XXX][YYY]==true)
					{
						settile();

						for(int u=0; u<9; u++) //check for tiles on top row
							if(board[u][19]==true)
							{
								gameOver=true;
								return;
							}

						for(int itr=0; itr<5; itr++) //check for conditions that make tile shift downwards
						{
							for(int z=0; z<20; z++)
								for(int v=0; v<20; v++)
								{
									checkfullrow(v);
									checkThreeSameColorRow(v);
								}
							for(int z=0; z<10; z++)
								for(int v=0; v<10; v++)
									checkThreeSameColorColumn(v);
							for(int z=0; z<10; z++)
								for(int zz=0; zz<10; zz++)
									for(int v=0; v<10; v++)
										checkForAnomalies(v);
						}

						newtile();
						return;
					}
				}
			}

			movetile(vec2(0.0,-1.0));
			break;

		case GLUT_KEY_LEFT:
			movetile(vec2(-1.0,0.0));
			break;

		case GLUT_KEY_RIGHT:
			movetile(vec2(1.0,0.0));
			break;
	}
}

//-------------------------------------------------------------------------------------------------------------------

// Handles standard keypresses
void keyboard(unsigned char key, int x, int y)
{
	switch(key) 
	{
		case 033: // Both escape key and 'q' cause the game to exit
		    exit(EXIT_SUCCESS);
		    break;

		case 'q':
			exit (EXIT_SUCCESS);
			break;

		case 'r': // 'r' key restarts the game
			restart();
			break;

		case ' ':
			shuffleCurrTileColour(); //shuffle tile colors
			break;
	}

	glutPostRedisplay();
}

//-------------------------------------------------------------------------------------------------------------------

void idle(void)
{
	glutPostRedisplay();
}

//-------------------------------------------------------------------------------------------------------------------

//============HELPER_FUNCTIONS============

void f1()
{
	// Bind the VBO containing current tile vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[4]); 

	// For each of the 4 'cells' of the tile,
	for (int i = 0; i < 4; i++) 
	{
		// Calculate the grid coordinates of the cell
		GLfloat x = -2.0; 
		GLfloat y = -2.0;

		//==MY CODE==

		occupiedCoor[i]=vec2(x,y);

		//==END MY CODE==

		// Create the 4 corners of the square - these vertices are using location in pixels
		// These vertices are later converted by the vertex shader
		vec4 p1 = vec4(33.0 + (x * 33.0), 33.0 + (y * 33.0), .4, 1); 
		vec4 p2 = vec4(33.0 + (x * 33.0), 66.0 + (y * 33.0), .4, 1);
		vec4 p3 = vec4(66.0 + (x * 33.0), 33.0 + (y * 33.0), .4, 1);
		vec4 p4 = vec4(66.0 + (x * 33.0), 66.0 + (y * 33.0), .4, 1);

		// Two points are used by two triangles each
		vec4 newpoints[6] = {p1, p2, p3, p2, p3, p4}; 

		// Put new data in the VBO
		glBufferSubData(GL_ARRAY_BUFFER, i*6*sizeof(vec4), 6*sizeof(vec4), newpoints); 
	}

	glBindVertexArray(0);
}

void f2()
{
	vec4 newcolours[24];

	for (int i = 0; i < 24; i++)
	{
		newcolours[i] = black;
	}

	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[5]); // Bind the VBO containing current tile vertex colours
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(newcolours), newcolours); // Put the colour data in the VBO
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);
}

void f3()
{
	f1();
	f2();
}

//-------------------------------------------------------------------------------------------------------------------

//function to make tile falls automatically
void fallingTileAuto(int data) 
{
	glutTimerFunc(FALL_SPEED, fallingTileAuto, -1);

	if(gameOver) //don't fall if gameOver
		return;

	if(iteration==1)
	{
		for(int itr=0; itr<5; itr++)
		{
			for(int z=0; z<20; z++)
				for(int v=0; v<20; v++)
					checkfullrow(v);
			for(int z=0; z<20; z++)
				for(int v=0; v<20; v++)
					checkThreeSameColorRow(v);
			for(int z=0; z<10; z++)
				for(int v=0; v<10; v++)
					checkThreeSameColorColumn(v);
		}

		newtile();
		return;
	}

	for(int i=0; i<4; i++)
	{
		int XXX=occupiedCoor[i].x;
		int YYY=occupiedCoor[i].y-1.0;

		if(occupiedCoor[i].y==0.0 || board[XXX][YYY]==true)
		{
			if(iteration==0)
			{
				settile();

				//check for tiles on top row
				for(int u=0; u<9; u++)
					if(board[u][19]==true)
					{
						gameOver=true;
						return;
					}

				//fix tiles with no support
				for(int itr=0; itr<5; itr++)
				{
					for(int z=0; z<10; z++)
						for(int zz=0; zz<10; zz++)
							for(int v=0; v<10; v++)
								checkForAnomalies(v);
				}

				f3();
				iteration++;
				return;
			}
		}
	}

	movetile(vec2(0.0,-1.0));

	glutPostRedisplay(); 
}

//-------------------------------------------------------------------------------------------------------------------

int main(int argc, char **argv)
{
	srand(time(NULL));

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowSize(xsize, ysize);
	glutInitWindowPosition(680, 178); // Center the game window (well, on a 1920x1080 display)
	glutCreateWindow("Fruit Tetris");
	glewInit();
	init();

	fallingTileAuto(-1);

	// Callback functions
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutSpecialFunc(special);
	glutKeyboardFunc(keyboard);
	glutIdleFunc(idle);

	glutMainLoop(); // Start main loop

	return 0;
}
