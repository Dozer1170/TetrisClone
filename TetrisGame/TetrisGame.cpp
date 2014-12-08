// include the basic windows header file
#include "stdafx.h"
#include <sstream>
#include <iostream>
#include <windows.h>
#include <windowsx.h>
#include <d3d9.h>
#include <d3dx9.h>

using namespace std;

//Direct3D Lib file
#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")

// define the screen resolution
#define SCREEN_WIDTH  500
#define SCREEN_HEIGHT 700

// define custom vertex format
#define CUSTOMFVF (D3DFVF_XYZ | D3DFVF_NORMAL| D3DFVF_DIFFUSE) 

// falling block height/width/size declarations
#define TILESIZE 2
#define MAPHEIGHT 20
#define MAPWIDTH 10
#define TILEBLACK 0
#define TILENODRAW 1
#define TILEBLUE 2
#define TILEGREEN 3
#define TILERED 4
#define TILEYELLOW 5
#define TILEORANGE 6
#define TILEPURPLE 7
#define TILEGREY 8
#define TILEAQUA 9

// text justification defines
#define LEFT 1
#define CENTER 2
#define RIGHT 3

int map[MAPWIDTH][MAPHEIGHT + 1];
DWORD startTime;
bool gameStarted = false;
bool danger = false;

//Global Direct3D Declarations
LPDIRECT3D9 d3d; //long pointer to direct3d interface
LPDIRECT3DDEVICE9 d3ddev; //long pointer to device
LPDIRECT3DVERTEXBUFFER9 v_buffer[8];    // the pointer to the vertex buffer
LPDIRECT3DINDEXBUFFER9 i_buffer = NULL;
LPD3DXFONT m_font = NULL;

//D3D function prototypes
void initD3D(HWND hWnd); //sets up D3d
void render_frame(void); //renders single frame
void cleanD3D(void); //closes Direct3D to release memory
void init_light(void);
void init_graphics(void); //initializes vertices and creates v_buffer
void display_text(wchar_t *disText, LONG rctLeft, LONG rctRight, LONG rctTop, LONG rctBottom, int justification); //displays given text to screen
void init_game(void); //create new game
void create_block(void); //create new block of struct piece
void move_block(int x, int y); // move the current block
int check_collision(int x, int y); // check if current block will collide with others (helper to move)
void game_timer(void); //advance block every 1 sec
void draw_blocks(void); //draws moving block and locked blocks
void create_vertices(int r, int g, int b, int vBufferIndex); //creates different colored vertices for drawing our blocks
void rotate_block(void); //rotates block
void remove_row(int row); //removes row
void game_over(); // displays game over message
wchar_t* score_display(wchar_t* text); //adds current score to text

// the WindowProc function prototype
LRESULT CALLBACK WindowProc(HWND hWnd,
                         UINT message,
                         WPARAM wParam,
                         LPARAM lParam);

struct CUSTOMVERTEX {FLOAT X, Y, Z; D3DVECTOR normal; DWORD color;};   //create custom vertex struct

struct Piece { int size[4][4], x , y; }; // represents a tetris piece
Piece piece; // current piece being moved
Piece prePiece; // preview of next piece

// this function initializes and prepares Direct3D for use
void initD3D(HWND hWnd)
{
    d3d = Direct3DCreate9(D3D_SDK_VERSION);    // create the Direct3D interface

    D3DPRESENT_PARAMETERS d3dpp;    // create a struct to hold various device information

    ZeroMemory(&d3dpp, sizeof(d3dpp));    // clear out the struct for use
    d3dpp.Windowed = TRUE;    // program windowed, not fullscreen
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;    // discard old frames
    d3dpp.hDeviceWindow = hWnd;    // set the window to be used by Direct3D
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8; // set back buffer to 32-bit
	d3dpp.BackBufferWidth = SCREEN_WIDTH; //set back buffer width
	d3dpp.BackBufferHeight = SCREEN_HEIGHT; //set back buffer height
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

    // create a device class using this information and information from the d3dpp stuct
    d3d->CreateDevice(D3DADAPTER_DEFAULT,
                      D3DDEVTYPE_HAL,
                      hWnd,
                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                      &d3dpp,
                      &d3ddev);

	if(D3DXCreateFont( d3ddev, 20, 0, FW_BOLD, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"), &m_font ) != D3D_OK)
	{
		cout << "Failed to create D3DFont, exiting.";
		exit(1);
	}

	init_graphics(); //initialize vertices and v buffer
	init_light();

	d3ddev->SetRenderState(D3DRS_LIGHTING, TRUE);    // turn on the 3D lighting
	d3ddev->SetRenderState(D3DRS_ZENABLE, TRUE);    // turn on/off the z-buffer
	d3ddev->SetRenderState(D3DRS_AMBIENT,RGB(50,50,50)); // set ambient lighting
	d3ddev->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);

	init_light();
}

// this is the function that sets up the lights and materials
void init_light(void)
{
    D3DLIGHT9 light;
    D3DMATERIAL9 material;

    ZeroMemory(&light, sizeof(light));    // clear out the light struct for use
    light.Type = D3DLIGHT_DIRECTIONAL;    // make the light type 'directional light'
    light.Diffuse = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);    // set the light's color
    light.Direction = D3DXVECTOR3(0.0f, -1.0f, -0.45f);

    d3ddev->SetLight(0, &light);
    d3ddev->LightEnable(0, TRUE);

    ZeroMemory(&material, sizeof(D3DMATERIAL9));
    material.Diffuse = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
    material.Ambient = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);

    d3ddev->SetMaterial(&material);
}

void init_graphics(void)
{
	//different coloured vertices
	create_vertices(10,10,255,TILEBLUE - 2);
	create_vertices(10,255,10,TILEGREEN - 2);
	create_vertices(255,10,10,TILERED - 2);
	create_vertices(255,255,10,TILEYELLOW - 2);
	create_vertices(255,150,10,TILEORANGE - 2);
	create_vertices(255,10,255,TILEPURPLE - 2);
	create_vertices(140,140,140,TILEGREY - 2);
	create_vertices(90,255,255,TILEAQUA - 2);

	// create the indices using an int array
	short indices[] =
	{
        0, 1, 2,    // side 1
        2, 1, 3,
        4, 5, 6,    // side 2
        6, 5, 7,
        8, 9, 10,    // side 3
        10, 9, 11,
        12, 13, 14,    // side 4
        14, 13, 15,
        16, 17, 18,    // side 5
        18, 17, 19,
        20, 21, 22,    // side 6
        22, 21, 23,
	}; 

	// create an index buffer interface called i_buffer
	d3ddev->CreateIndexBuffer(36*sizeof(short),
							  0,
							  D3DFMT_INDEX16,
							  D3DPOOL_MANAGED,
							  &i_buffer,
							  NULL);

	VOID *pVoid;

	// lock i_buffer and load the indices into it
	i_buffer->Lock(0, 0, (void**)&pVoid, 0);
	memcpy(pVoid, indices, sizeof(indices));
	i_buffer->Unlock(); 
}

void create_vertices(int r, int g, int b, int vBufferIndex)
{
	CUSTOMVERTEX vertices[] =
    {
       { -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(r,g,b) },    // side 1
        { 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(r,g,b) },
        { -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(r,g,b) },
        { 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(r,g,b) },

        { -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, D3DCOLOR_XRGB(r,g,b) },    // side 2
        { -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, D3DCOLOR_XRGB(r,g,b) },
        { 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, D3DCOLOR_XRGB(r,g,b) },
        { 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, D3DCOLOR_XRGB(r,g,b) },

        { -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, D3DCOLOR_XRGB(r,g,b) },    // side 3
        { -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, D3DCOLOR_XRGB(r,g,b) },
        { 1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, D3DCOLOR_XRGB(r,g,b) },
        { 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, D3DCOLOR_XRGB(r,g,b) },

        { -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, D3DCOLOR_XRGB(r,g,b) },    // side 4
        { 1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, D3DCOLOR_XRGB(r,g,b) },
        { -1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, D3DCOLOR_XRGB(r,g,b) },
        { 1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, D3DCOLOR_XRGB(r,g,b) },

        { 1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, D3DCOLOR_XRGB(r,g,b) },    // side 5
        { 1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, D3DCOLOR_XRGB(r,g,b) },
        { 1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, D3DCOLOR_XRGB(r,g,b) },
        { 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, D3DCOLOR_XRGB(r,g,b) },

        { -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, D3DCOLOR_XRGB(r,g,b) },    // side 6
        { -1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, D3DCOLOR_XRGB(r,g,b) },
        { -1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, D3DCOLOR_XRGB(r,g,b) },
        { -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, D3DCOLOR_XRGB(r,g,b) },
	};

    // create the vertex and store the pointer into v_buffer, which is created globally
    d3ddev->CreateVertexBuffer(24*sizeof(CUSTOMVERTEX),
                               0,
                               CUSTOMFVF,
                               D3DPOOL_MANAGED,
                               &v_buffer[vBufferIndex],
                               NULL);

    VOID* pVoid;    // the void pointer

    v_buffer[vBufferIndex]->Lock(0, 0, (void**)&pVoid, 0);    // lock the vertex buffer
	memcpy(pVoid, vertices, sizeof(vertices));    // copy the vertices to the locked buffer
    v_buffer[vBufferIndex]->Unlock();    // unlock the vertex buffer
}

void init_game(void)
{
	startTime = GetTickCount();
	gameStarted = false;

	//initialize map to all black
	for(int x = 0; x < MAPWIDTH; x++)
	{
		for(int y = 0; y < MAPHEIGHT + 1; y++)
		{
			if(y == MAPHEIGHT)
				map[x][y] = TILEGREY;
			else
				map[x][y] = TILEBLACK;
		}
	}

	create_block();
}

void create_block(void)
{
	int randBlock;
	int i,j;

	srand(GetTickCount());

	//clear the piece
	for(i = 0; i < 4; ++i)
	{
		for(j=0; j < 4; ++j)
			piece.size[i][j] = TILENODRAW;
	}

	piece.x = MAPWIDTH/2 - 2;
	piece.y = 0;
	
	//case for if we need to generate preview and current piece
	if(gameStarted == false)
	{
		//get random num for type of block
		randBlock = rand() % 7;
		gameStarted = true;

		// have 7 different types of blocks: tower(red),box(blue),pyramid(green),
		// leftlean(yellow),rightlean(orange),leftknight(purple),rightknight(silver)
		switch(randBlock)
		{
		case 0: //Tower
			piece.size[1][0] = TILERED;
			piece.size[1][1] = TILERED;
			piece.size[1][2] = TILERED;
			piece.size[1][3] = TILERED;
			break;
		case 1: //BOX
			piece.size[1][1] = TILEBLUE;
			piece.size[2][1] = TILEBLUE;
			piece.size[1][2] = TILEBLUE;
			piece.size[2][2] = TILEBLUE;
			break;
		case 2: //Pyramid
			piece.size[1][1] = TILEGREEN;
			piece.size[1][2] = TILEGREEN;
			piece.size[0][2] = TILEGREEN;
			piece.size[2][2] = TILEGREEN;
			break;
		case 3: //Left Lean
			piece.size[1][1] = TILEYELLOW;
			piece.size[2][1] = TILEYELLOW;
			piece.size[2][2] = TILEYELLOW;
			piece.size[3][2] = TILEYELLOW;
			break;
		case 4: //Right Lean
			piece.size[2][1] = TILEORANGE;
			piece.size[3][1] = TILEORANGE;
			piece.size[1][2] = TILEORANGE;
			piece.size[2][2] = TILEORANGE;
			break;
		case 5: //Left Knight
			piece.size[1][1] = TILEPURPLE;
			piece.size[2][1] = TILEPURPLE;
			piece.size[2][2] = TILEPURPLE;
			piece.size[2][3] = TILEPURPLE;
			break;
		case 6: //Right Knight
			piece.size[2][1] = TILEAQUA;
			piece.size[1][1] = TILEAQUA;
			piece.size[1][2] = TILEAQUA;
			piece.size[1][3] = TILEAQUA;
			break;
		}
	}
	else
	{
		for(i = 0; i < 4; ++i)
			for(j = 0; j < 4; ++j)
				piece.size[i][j] = prePiece.size[i][j];
	}

	// NOW we create the preview piece!

	//clear the preview prePiece
	for(i = 0; i < 4; ++i)
	{
		for(j=0; j < 4; ++j)
			prePiece.size[i][j] = TILENODRAW;
	}

	prePiece.x = MAPWIDTH + 2;
	prePiece.y = MAPHEIGHT - 4;
	
	//get random num for type of block
	randBlock = rand() % 7;

	// have 7 different types of blocks: tower(red),box(blue),pyramid(green),
	// leftlean(yellow),rightlean(orange),leftknight(pink),rightknight(silver)
	switch(randBlock)
	{
		case 0: //Tower
			prePiece.size[1][0] = TILERED;
			prePiece.size[1][1] = TILERED;
			prePiece.size[1][2] = TILERED;
			prePiece.size[1][3] = TILERED;
			break;
		case 1: //BOX
			prePiece.size[1][1] = TILEBLUE;
			prePiece.size[2][1] = TILEBLUE;
			prePiece.size[1][2] = TILEBLUE;
			prePiece.size[2][2] = TILEBLUE;
			break;
		case 2: //Pyramid
			prePiece.size[1][1] = TILEGREEN;
			prePiece.size[1][2] = TILEGREEN;
			prePiece.size[0][2] = TILEGREEN;
			prePiece.size[2][2] = TILEGREEN;
			break;
		case 3: //Left Lean
			prePiece.size[1][1] = TILEYELLOW;
			prePiece.size[2][1] = TILEYELLOW;
			prePiece.size[2][2] = TILEYELLOW;
			prePiece.size[3][2] = TILEYELLOW;
			break;
		case 4: //Right Lean
			prePiece.size[2][1] = TILEORANGE;
			prePiece.size[3][1] = TILEORANGE;
			prePiece.size[1][2] = TILEORANGE;
			prePiece.size[2][2] = TILEORANGE;
			break;
		case 5: //Left Knight
			prePiece.size[1][1] = TILEPURPLE;
			prePiece.size[2][1] = TILEPURPLE;
			prePiece.size[2][2] = TILEPURPLE;
			prePiece.size[2][3] = TILEPURPLE;
			break;
		case 6: //Right Knight
			prePiece.size[2][1] = TILEAQUA;
			prePiece.size[1][1] = TILEAQUA;
			prePiece.size[1][2] = TILEAQUA;
			prePiece.size[1][3] = TILEAQUA;
			break;
	}
}

void move_block(int x, int y)
{
	//if there is a collision
	if(check_collision(x,y))
	{
		//if we were moving down
		if(y > 0)
		{
			//if at top of the screen
			if(piece.y < 1)
			{
				game_over();
			}
			else // add this to the map
			{
				if(piece.y < 5)
					danger = true;
				int i,j;

				for(i = 0; i < 4; ++i)
				{
					for(j = 0; j < 4; ++j)
					{
						if(piece.size[i][j] != TILENODRAW)
						{
							map[piece.x + i][piece.y + j] = piece.size[i][j];
						}
					}
				}

				// perhaps a row has been cleared?
				for(j = 0; j < MAPHEIGHT; j++)
				{
					bool filled = true;
					for(i = 0; i < MAPWIDTH; i++)
					{
						if(map[i][j] == TILEBLACK)
						{
							filled = false;
							break;
						}
					}

					if(filled)
					{
						remove_row(j);
					}

				}
				create_block();
			}
		}
	}
	else
	{
		piece.x+=x;
		piece.y+=y;
	}
}

void game_over(void)
{
	gameStarted = false;
}

void remove_row(int row)
{
	int x,y;

	for(x = 0; x < MAPWIDTH; x++)
	{
		for(y = row; y > 0; y--)
		{
			map[x][y] = map[x][y - 1];
			if(y == 5)
				if(map[x][y] == TILEBLACK)
					danger = false;
		}
	}
}

void rotate_block(void)
{
	int i, j, temp[4][4];

	//copy &rotate the piece to the temporary array
	for(i=0; i<4; i++)
		for(j=0; j<4; j++)
			temp[3-j][ i ]=piece.size[ i ][j];

	//check collision of the temporary array with map borders
	for(i=0; i<4; i++)
		for(j=0; j<4; j++)
			if(temp[ i ][j] != TILENODRAW)
				if(piece.x + i < 0 || piece.x + i > MAPWIDTH - 1 ||
					piece.y + j < 0 || piece.y + j > MAPHEIGHT - 1)
					return;

	//check collision of the temporary array with the blocks on the map
	for(int x=0; x< MAPWIDTH; x++)
		for(int y=0; y< MAPHEIGHT; y++)
			if(x >= piece.x && x < piece.x + 4)
				if(y >= piece.y && y < piece.y +4)
					if(map[x][y] != TILEBLACK)
						if(temp[x - piece.x][y - piece.y] != TILENODRAW)
							return;

	//end collision check

	//successful!  copy the rotated temporary array to the original piece
	for(i=0; i<4; i++)
		for(j=0; j<4; j++)
			piece.size[ i ][j]=temp[ i ][j];
}

//check if piece moved by x and y if it will collide with walls or other blocks
int check_collision(int nx, int ny)
{
	int nextx = piece.x + nx;
	int nexty = piece.y + ny;
	int i,j,x,y;

	//walls checking for each part of the piece that is filled in
	for(i = 0; i < 4; i++)
	{
		for(j = 0; j < 4; j++)
		{
			if(piece.size[i][j] != TILENODRAW)
				if(nextx + i < 0 || nextx + i > MAPWIDTH - 1 || 
						nexty + j < 0 || nexty + j > MAPHEIGHT - 1)
						return 1;
		}
	}

	//check if it will collide with other blocks
	for(x=0; x< MAPWIDTH; x++)
		for(y=0; y< MAPHEIGHT; y++)
			if(x >= nextx && x < nextx + 4)
				if(y >= nexty && y < nexty +4)
					if(map[x][y] != TILEBLACK)
						if(piece.size[x - nextx][y - nexty] != TILENODRAW)
							return 1;

	return 0;
}

void game_timer(void)
{
	if(gameStarted)
	{
		if(GetTickCount() - startTime > 1000)
		{
			move_block(0,1);
			startTime = GetTickCount();
		}
	}
}

// this is the function used to render a single frame
void render_frame(void)
{
    // clear the window to black, clear zbuffer
    d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	d3ddev->Clear(0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

    d3ddev->BeginScene();    // begins the 3D scene

    // select which vertex format we are using
    d3ddev->SetFVF(CUSTOMFVF);

	// SET UP THE PIPELINE
    D3DXMATRIX matView;    // the view transform matrix

    D3DXMatrixLookAtLH(&matView,
                       &D3DXVECTOR3 ((FLOAT) (TILESIZE * (MAPWIDTH/2 + 2)), 75.0f, (FLOAT)(-MAPHEIGHT)),    // the camera position
					   &D3DXVECTOR3 ((FLOAT) (TILESIZE * (MAPWIDTH/2 + 2)), 0.0f, (FLOAT) (-MAPHEIGHT)),    // the look-at position
                       &D3DXVECTOR3 (0.0f, 0.0f, 1.0f));    // the up direction

    d3ddev->SetTransform(D3DTS_VIEW, &matView);    // set the view transform to matView

    D3DXMATRIX matProjection;     // the projection transform matrix

    D3DXMatrixPerspectiveFovLH(&matProjection,
                               D3DXToRadian(45),    // the horizontal field of view
                               (FLOAT) SCREEN_WIDTH / (FLOAT)SCREEN_HEIGHT, // aspect ratio
                               1.0f,    // the near view-plane
                               300.0f);    // the far view-plane

    d3ddev->SetTransform(D3DTS_PROJECTION, &matProjection);    // set the projection
 
	draw_blocks();

	display_text(score_display(L"Score:"), 2, 300, 10, 30, LEFT);

	if(!gameStarted)
		display_text(L"Game Over!", 0, SCREEN_WIDTH, SCREEN_HEIGHT/2, SCREEN_HEIGHT, CENTER);
	else
		display_text(L"Next Piece", SCREEN_WIDTH/2 + 70, SCREEN_WIDTH, SCREEN_HEIGHT/2 + 100, SCREEN_HEIGHT, CENTER);

    d3ddev->EndScene();    // ends the 3D scene

    d3ddev->Present(NULL, NULL, NULL, NULL);    // displays the created frame
}

wchar_t* score_display(wchar_t* text)
{
	static int score = 0; 
	if(gameStarted)
		score++;
	wchar_t istr[32];
	_itow_s(score,istr,10);
	wchar_t *disText = (wchar_t*) malloc(60 * sizeof(wchar_t));
	wcscpy(disText, text);
	wcscat(disText,istr);

	return disText;
}

void draw_blocks(void)
{
	D3DXMATRIX matTranslate;
	D3DXMATRIX matRotateZ;
	int i,j;
	static FLOAT rot = 0.0f; rot+=0.025f;
	if(rot >= 360.0f)
		rot = 0.0f;

	// select the vertex and index buffer to display
	d3ddev->SetIndices(i_buffer);
	
	if(danger)
		D3DXMatrixRotationZ(&matRotateZ,rot);
	else
		D3DXMatrixRotationZ(&matRotateZ,0.0f);

	//draw current block that is moving
	for(i = 0; i < 4; i++)
	{
		for(j = 0; j < 4; j++)
		{
			if(piece.size[i][j] != TILENODRAW)
			{
				D3DXMatrixTranslation(&matTranslate,(FLOAT)(TILESIZE * (piece.x + i)),0.0f,(FLOAT)(TILESIZE * -(piece.y + j)));
				d3ddev->SetTransform(D3DTS_WORLD, &(matRotateZ * matTranslate));
				d3ddev->SetStreamSource(0, v_buffer[piece.size[i][j] - 2], 0, sizeof(CUSTOMVERTEX));
				d3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 24, 0, 12);
			}
		}
	}

	//draw preview block
	for(i = 0; i < 4; i++)
	{
		for(j = 0; j < 4; j++)
		{
			if(prePiece.size[i][j] != TILENODRAW)
			{
				D3DXMatrixTranslation(&matTranslate,(FLOAT)(TILESIZE * (prePiece.x + i)),0.0f,(FLOAT)(TILESIZE * -(prePiece.y + j)));
				d3ddev->SetTransform(D3DTS_WORLD, &(matRotateZ * matTranslate));
				d3ddev->SetStreamSource(0, v_buffer[prePiece.size[i][j] - 2], 0, sizeof(CUSTOMVERTEX));
				d3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 24, 0, 12);
			}
		}
	}

	//draw map
	for(i = 0; i < MAPWIDTH; i++)
	{
		for(j = 0; j < MAPHEIGHT + 1; j++)
		{
			if(map[i][j] != TILEBLACK)
			{
				D3DXMatrixTranslation(&matTranslate,(FLOAT)(TILESIZE * i),0.0f,(FLOAT)-(TILESIZE * j));
				d3ddev->SetTransform(D3DTS_WORLD, &(matRotateZ * matTranslate));
				d3ddev->SetStreamSource(0, v_buffer[map[i][j] - 2], 0, sizeof(CUSTOMVERTEX));
				d3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 24, 0, 12);
			}
		}
	}

	//left border
	i = -1;
	d3ddev->SetStreamSource(0, v_buffer[TILEGREY - 2], 0, sizeof(CUSTOMVERTEX));
	for(j = 0; j < MAPHEIGHT + 1; j++)
	{
		D3DXMatrixTranslation(&matTranslate,(FLOAT)(TILESIZE * i),0.0f,(FLOAT)-(TILESIZE * j));
		d3ddev->SetTransform(D3DTS_WORLD, &(matRotateZ * matTranslate));
		d3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 24, 0, 12);
	}
	//top border
	j = -1;
	for(i = -1; i < MAPWIDTH + 1; i++)
	{
		D3DXMatrixTranslation(&matTranslate,(FLOAT)(TILESIZE * i),0.0f,(FLOAT)-(TILESIZE * j));
		d3ddev->SetTransform(D3DTS_WORLD, &(matRotateZ * matTranslate));
		d3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 24, 0, 12);
	}
	//right border
	i = MAPWIDTH;
	for(j = 0; j < MAPHEIGHT+ 1; j++)
	{
		D3DXMatrixTranslation(&matTranslate,(FLOAT)(TILESIZE * i),0.0f,(FLOAT)-(TILESIZE * j));
		d3ddev->SetTransform(D3DTS_WORLD, &(matRotateZ * matTranslate));
		d3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 24, 0, 12);
	}
}

void display_text(wchar_t *disText, LONG rctLeft, LONG rctRight, LONG rctTop, LONG rctBottom, int justify)
{	
	RECT rct;
	rct.left=rctLeft;
	rct.right=rctRight;
	rct.top=rctTop;
	rct.bottom=rctBottom;
	
	if(justify == LEFT)
		m_font->DrawText(NULL,disText, -1, &rct, DT_LEFT, D3DCOLOR_ARGB(255,255,255,255));
	
	if(justify == RIGHT)
		m_font->DrawText(NULL,disText, -1, &rct, DT_RIGHT, D3DCOLOR_ARGB(255,255,255,255));
	
	if(justify == CENTER)
		m_font->DrawText(NULL,disText, -1, &rct, DT_CENTER, D3DCOLOR_ARGB(255,255,255,255));
}

// this is the function that cleans up Direct3D and COM
void cleanD3D(void)
{
	for(int i = 0; i < 8; i++)
		v_buffer[i]->Release();// close and release the vertex buffer
	i_buffer->Release();// close and release index buffer
	d3ddev->Release();    // close and release the 3D device
    d3d->Release();    // close and release Direct3D
	m_font->Release(); // close and release font
}

// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    // the handle for the window, filled by a function
    HWND hWnd;
    // this struct holds information for the window class
    WNDCLASSEX wc;

    // clear out the window class for use
    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    // fill in the struct with the needed information
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    //wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = L"FallingBlock3D";

    // register the window class
    RegisterClassEx(&wc);

    // create the window and use the result as the handle
    hWnd = CreateWindowEx(NULL,
                          L"FallingBlock3D",    // name of the window class
						  L"Falling Blocks 3D", // title of the window
                        //  WS_EX_TOPMOST | WS_POPUP,    // fullscreen
						  WS_OVERLAPPED, //windowed
                          0,    // x-position of the window
                          0,    // y-position of the window
                          SCREEN_WIDTH,    // width of the window
                          SCREEN_HEIGHT,    // height of the window
                          NULL,    // we have no parent window, NULL
                          NULL,    // we aren't using menus, NULL
                          hInstance,    // application handle
                          NULL);    // used with multiple windows, NULL

	RAWINPUTDEVICE Rid[2];

	// Keyboard
	Rid[0].usUsagePage = 1;
	Rid[0].usUsage = 6;
	Rid[0].dwFlags = 0;
	Rid[0].hwndTarget=NULL;

	// Mouse
	Rid[1].usUsagePage = 1;
	Rid[1].usUsage = 2;
	Rid[1].dwFlags = 0;
	Rid[1].hwndTarget=NULL;

	if (RegisterRawInputDevices(Rid,2,sizeof(RAWINPUTDEVICE))==FALSE)
	{
		cerr << "Could not register raw input devices.";
		exit(11);
	}

    // display the window on the screen
    ShowWindow(hWnd, nCmdShow);

	initD3D(hWnd);

	init_game();

    // enter the main loop:

    // this struct holds Windows event messages
    MSG msg;

    // Enter the infinite message loop
	while(TRUE)
	{
		// Check to see if any messages are waiting in the queue
	    while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			// Translate the message and dispatch it to WindowProc()
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// If the message is WM_QUIT, exit the while loop
		if(msg.message == WM_QUIT)
			break;

		game_timer();
		render_frame();
	}

	cleanD3D();

    // return this part of the WM_QUIT message to Windows
    return msg.wParam;
}

// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // sort through and find what code to run for the message given
    switch(message)
    {
		case WM_INPUT:
			{
				static DWORD lastDInputTime = GetTickCount();
				static DWORD lastLInputTime = GetTickCount();
				static DWORD lastRInputTime = GetTickCount();
				static DWORD lastRotInputTime = GetTickCount();
				static bool keyArrowDUp;
				static bool keyArrowLUp;
				static bool keyArrowRUp;
				static bool keySpaceUp;
				// Determine how big the buffer should be
				UINT bufferSize;
				GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &bufferSize, sizeof (RAWINPUTHEADER));

				// Create a buffer of the correct size - but see note below
				BYTE *buffer=new BYTE[bufferSize];

				// Call the function again, this time with the buffer to get the data
				GetRawInputData((HRAWINPUT)lParam, RID_INPUT, (LPVOID)buffer, &bufferSize, sizeof (RAWINPUTHEADER));

				RAWINPUT *raw = (RAWINPUT*) buffer;
				//if (raw->header.dwType== RIM_TYPEMOUSE)
					// read mouse data
				if (raw->header.dwType== RIM_TYPEKEYBOARD)
				{
					
					USHORT keyCode = raw->data.keyboard.VKey;
					switch(keyCode)
					{
						case VK_DOWN:
							keyArrowDUp = raw->data.keyboard.Flags & RI_KEY_BREAK;
							if(keyArrowDUp)
								lastDInputTime = GetTickCount();
							if((GetTickCount() - lastDInputTime > 100))
								move_block(0,1);
							break;
						case VK_LEFT:
							keyArrowLUp = raw->data.keyboard.Flags & RI_KEY_BREAK;
							if(keyArrowLUp)
								lastLInputTime = GetTickCount();
							if(GetTickCount() - lastLInputTime > 100)
								move_block(-1,0);
							break;
						case VK_RIGHT:
							keyArrowRUp = raw->data.keyboard.Flags & RI_KEY_BREAK;
							if(keyArrowRUp)
								lastRInputTime = GetTickCount();
							if(GetTickCount() - lastRInputTime > 100)
								move_block(1,0);
							break;
						case VK_SPACE:
							keySpaceUp = raw->data.keyboard.Flags & RI_KEY_BREAK;
							if(keySpaceUp)
								lastRotInputTime = GetTickCount();
							if(GetTickCount() - lastRotInputTime > 100)
								rotate_block();
							break;
						default:
							break;
					}
				}
				return 0;
			}
			break;
        case WM_DESTROY:
            {
                // close the application entirely
                PostQuitMessage(0);
                return 0;
            } break;
    }

    // Handle any messages the switch statement didn't
    return DefWindowProc (hWnd, message, wParam, lParam);
} 