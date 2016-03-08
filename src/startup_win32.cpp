#include "windows.h"
#include "stdint.h"
#include "stdlib.h"
#include "stdio.h"
#include "math.h"

#define ArrayCount(array) ((sizeof(array)) / (sizeof((array)[0])))

LRESULT WindowCallback(HWND hwnd, UINT uMsg,
					   WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_CLOSE:
			DestroyWindow(hwnd);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

#pragma pack(push, 1)
struct bitmap_header
{
   uint16_t file_type;
   uint32_t file_size;
   uint16_t reserved1;
   uint16_t reserved2;
   uint32_t bitmap_offset;
   uint32_t size;
   int32_t width;
   int32_t height;
   uint16_t planes;
   uint16_t bits_per_pixel;
};
#pragma pack(pop)

struct LoadedBitmap
{
	uint32_t *pixels;
	int width;
	int height;
};

struct Framebuffer
{
	uint32_t *pixels;
	int width;
	int height;
};

struct Vector4
{
	float r;
	float g;
	float b;
	float a;
};

struct Vector2
{
	float x;
	float y;
};

struct Rect2
{
	Vector2 min;
	Vector2 max;
};

Vector4 V4(float r, float g, float b, float a)
{
	Vector4 output = {};
	output.r = r;
	output.g = g;
	output.b = b;
	output.a = a;
	return output;
}

Vector2 V2(float x, float y)
{
	Vector2 output = {};
	output.x = x;
	output.y = y;
	return output;
}

Vector2 operator+ (Vector2 v1, Vector2 v2)
{
	Vector2 output = {};
	output.x = v1.x + v2.x;
	output.y = v1.y + v2.y;
	return output;
}

Vector2 operator- (Vector2 v1, Vector2 v2)
{
	Vector2 output = {};
	output.x = v1.x - v2.x;
	output.y = v1.y - v2.y;
	return output;
}

Vector2 operator* (Vector2 v, float s)
{
	Vector2 output = {};
	output.x = v.x * s;
	output.y = v.y * s;
	return output;
}

Vector2 operator/ (Vector2 v, float s)
{
	Vector2 output = {};
	output.x = v.x / s;
	output.y = v.y / s;
	return output;
}

float Length(Vector2 v)
{
	return sqrtf((v.x * v.x) + (v.y * v.y));
}

Vector2 Normalize(Vector2 v)
{
	return v / Length(v);
}

#define PI 3.14159265359f

float GetAngle(Vector2 v)
{
	if(Length(v) > 0.0f)
	{
		Vector2 uv = Normalize(v);
		return (asin(uv.y) * 180) / PI;
	}
	
	return 0.0f;
}

enum Direction
{
	Direction_Up,
	Direction_Down,
	Direction_Left,
	Direction_Right,
	Direction_Unknown
};

Direction AngleToDirection(float angle)
{
	if((angle <= 135) && (angle > 45)) return Direction_Up;
	if((angle <= 45) || (angle > 315)) return Direction_Right;
	if((angle <= 225) && (angle > 135)) return Direction_Left;
	if((angle <= 315) && (angle > 225)) return Direction_Down;
	
	return Direction_Unknown;
}

Rect2 RectPosSize(float x, float y, float width, float height)
{
	Rect2 result = {};
	
	result.min.x = x - 0.5f * width;
	result.min.y = y - 0.5f * height;
	result.max.x = x + 0.5f * width;
	result.max.y = y + 0.5f * height;
	
	return result;
}

float GetWidth(Rect2 rect)
{
	return (rect.max.x - rect.min.x);
}

float GetHeight(Rect2 rect)
{
	return (rect.max.y - rect.min.y);
}

Vector2 GetCenter(Rect2 rect)
{
	return V2(rect.min.x + 0.5f * GetWidth(rect), rect.min.y + 0.5f * GetHeight(rect));
}

Rect2 GrowBy(Rect2 base, Rect2 extension)
{
	return RectPosSize(GetCenter(base).x, GetCenter(base).y, GetWidth(base) + GetWidth(extension), GetHeight(base) + GetHeight(extension));
}

bool Contains(Rect2 rect, Vector2 vec)
{
	return (vec.x > rect.min.x) &&
		   (vec.x < rect.max.x) &&
		   (vec.y > rect.min.y) &&
		   (vec.y < rect.max.y);
}

bool Intersect(Rect2 rect1, Rect2 rect2)
{
	Rect2 collision_rect = GrowBy(rect1, rect2);
	return Contains(collision_rect, GetCenter(rect2));
}

int RoundFloatToInt(float in)
{
	int output = (int)(in + 0.5f);
	return output;
}

enum EntityType
{
	EntityType_Invalid,
	EntityType_Player,
	EntityType_Box
};

struct Entity
{
	EntityType type;
	Vector2 velocity;
	Rect2 bounds;
};

struct EntityArray
{
	Entity entities[32];
	int count;
};

Entity *MakeEntity(EntityArray *entities, EntityType type, Vector2 pos, int width, int height, Vector2 velocity = {0, 0})
{
	Entity *entity = entities->entities + entities->count++;
	
	entity->type = type;
	entity->bounds = RectPosSize(pos.x, pos.y, width, height);
	entity->velocity = velocity;
	
	return entity;
}

LoadedBitmap LoadBitmap(char *path)
{
	LoadedBitmap result = {};
	
	HANDLE bitmap_handle = CreateFileA(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	int bitmap_file_size = GetFileSize(bitmap_handle, NULL);
	uint8_t *bitmap_file_data = (uint8_t *)VirtualAlloc(NULL, bitmap_file_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	DWORD number_of_bytes_read = 0;
	ReadFile(bitmap_handle, bitmap_file_data, bitmap_file_size, &number_of_bytes_read, NULL);
	CloseHandle(bitmap_handle);
	
	bitmap_header *header = (bitmap_header *)bitmap_file_data;
	result.width = header->width;
	result.height = header->height;
	result.pixels = (uint32_t *)(bitmap_file_data + header->bitmap_offset);
	
	return result;
}

void DrawRectangle(int x_in, int y_in, int width, int height, Framebuffer fbo, Vector4 color)
{
	if(x_in > fbo.width)
		width = 0;
	
	if((x_in + width) > fbo.width)
		width = fbo.width - x_in;
	
	if(y_in > fbo.height)
		height = 0;
	
	if((y_in + height) > fbo.height)
		height = fbo.height - y_in;
	
	if(x_in < 0)
	{
		width = width + x_in;
		x_in = 0;
	}
	
	if((x_in + width) < 0)
		width = 0;
	
	if(y_in < 0)
	{
		height = height + y_in;
		y_in = 0;
	}
	
	if((y_in + height) < 0)
		height = 0;
	
	for (int x = x_in; x < (x_in + width); x++)
	{
		for(int y = y_in; y < (y_in + height); y++)
		{
			uint32_t old_color = *(fbo.pixels + (y * fbo.width) + x);
			float oldr = ((float)((old_color & 0x00FF0000) >> 16)) / 255.0f;
			float oldg = ((float)((old_color & 0x0000FF00) >> 8)) / 255.0f;
			float oldb = ((float)((old_color & 0x000000FF) >> 0)) / 255.0f;
			
			float newr = color.a * color.r + (1 - color.a) * oldr;
			float newg = color.a * color.g + (1 - color.a) * oldg;
			float newb = color.a * color.b + (1 - color.a) * oldb;
			
			uint32_t color32 = (RoundFloatToInt(color.a * 255) < 24) |
							   (RoundFloatToInt(newr * 255) << 16) |
							   (RoundFloatToInt(newg * 255) << 8) |
							   (RoundFloatToInt(newb * 255) << 0);
			
			*(fbo.pixels + (y * fbo.width) + x) = color32;
		}
	}
}

void DrawBitmap(int x_in, int y_in, int width, int height, Framebuffer fbo, LoadedBitmap bitmap)
{
	int x_offset = 0;
	int y_offset = 0;
	
	if(x_in > fbo.width)
		width = 0;
	
	if((x_in + width) > fbo.width)
		width = fbo.width - x_in;
	
	if(y_in > fbo.height)
		height = 0;
	
	if((y_in + height) > fbo.height)
		height = fbo.height - y_in;
	
	if(x_in < 0)
	{
		x_offset = -x_in;
		width = width + x_in;
		x_in = 0;
	}
	
	if((x_in + width) < 0)
		width = 0;
	
	if(y_in < 0)
	{
		y_offset = -y_in;
		height = height + y_in;
		y_in = 0;
	}
	
	if((y_in + height) < 0)
		height = 0;

	for (int x = 0; (x + x_in) < (x_in + width); x++)
	{
		for(int y = 0; (y + y_in) < (y_in + height); y++)
		{
			uint32_t old_color = *(fbo.pixels + ((y + y_in) * fbo.width) + (x + x_in));
			
			float oldr = ((float)((old_color & 0x00FF0000) >> 16)) / 255.0f;
			float oldg = ((float)((old_color & 0x0000FF00) >> 8)) / 255.0f;
			float oldb = ((float)((old_color & 0x000000FF) >> 0)) / 255.0f;
			
			uint32_t texel_color = *(bitmap.pixels + ((y + y_offset) * bitmap.width) + x + x_offset);
			
			float texelr = ((float)((texel_color & 0x00FF0000) >> 16)) / 255.0f;
			float texelg = ((float)((texel_color & 0x0000FF00) >> 8)) / 255.0f;
			float texelb = ((float)((texel_color & 0x000000FF) >> 0)) / 255.0f;
			float texela = ((float)((texel_color & 0xFF000000) >> 24)) / 255.0f;
			
			float newr = texela * texelr + (1 - texela) * oldr;
			float newg = texela * texelg + (1 - texela) * oldg;
			float newb = texela * texelb + (1 - texela) * oldb;
			
			uint32_t color32 = (RoundFloatToInt(texela * 255) < 24) |
							   (RoundFloatToInt(newr * 255) << 16) |
							   (RoundFloatToInt(newg * 255) << 8) |
							   (RoundFloatToInt(newb * 255) << 0);
			
			*(fbo.pixels + ((y + y_in) * fbo.width) + (x + x_in)) = color32;
		}
	}
}

#define W_KEY_CODE 0x57
#define A_KEY_CODE 0x41
#define S_KEY_CODE 0x53
#define D_KEY_CODE 0x44
#define Q_KEY_CODE 0x51

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
			LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX window_class = {}; 
	window_class.cbSize = sizeof(window_class);

    window_class.style = CS_HREDRAW | CS_VREDRAW;
	window_class.lpfnWndProc = WindowCallback;
    window_class.hInstance = hInstance;
    window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    window_class.lpszClassName = "WCPetroCanada";
	
	RegisterClassEx(&window_class);

	HWND window = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, "WCPetroCanada", "Game :)", WS_OVERLAPPEDWINDOW,
							     CW_USEDEFAULT, CW_USEDEFAULT,
								 1280, 720,
								 NULL, NULL, hInstance, NULL);
	
	ShowWindow(window, nCmdShow);
	UpdateWindow(window);

	RECT client_rect = {};
	GetClientRect(window, &client_rect);
	
	Framebuffer backbuffer = {};
	backbuffer.width = client_rect.right;
	backbuffer.height = client_rect.bottom;
	backbuffer.pixels = (uint32_t *)VirtualAlloc(NULL, backbuffer.width * backbuffer.height * sizeof(uint32_t ), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	HDC window_context = GetDC(window);
	
	BITMAPINFO bitmap_info = {};
	bitmap_info.bmiHeader.biSize = sizeof(bitmap_info);
	bitmap_info.bmiHeader.biWidth = backbuffer.width;
	bitmap_info.bmiHeader.biHeight = backbuffer.height;
	bitmap_info.bmiHeader.biPlanes = 1;
	bitmap_info.bmiHeader.biBitCount = 32;
	bitmap_info.bmiHeader.biCompression = BI_RGB;
	
	LoadedBitmap player_head = LoadBitmap("petro.bmp");
	
	MSG message;
	bool open = true;
	Vector2 velocity = V2(0, 0);
	Vector2 position = V2(200, 200);
	float timestep = 1.0f; //TODO proper timestep
	
	EntityArray entities = {};
	Entity *player_entity = MakeEntity(&entities, EntityType_Player, V2(200, 200), player_head.width, player_head.height, V2(10, 10));
	MakeEntity(&entities, EntityType_Box, V2(50, 50), 100, 100);
	
	bool w_down = false;
	bool a_down = false;
	bool s_down = false;
	bool d_down = false;
	bool q_down = false;
	
	while(open)
	{
		while(PeekMessage(&message, NULL, 0,
						  0, PM_REMOVE))
		{
			if(message.message == WM_QUIT)
			{
				open = false;
			}
			else if(message.message == WM_KEYDOWN)
			{
				if(message.wParam == W_KEY_CODE)
				{
					w_down = true;
				}
				else if(message.wParam == S_KEY_CODE)
				{
					s_down = true;
				}
				else if(message.wParam == D_KEY_CODE)
				{
					d_down = true;
				}
				else if(message.wParam == A_KEY_CODE)
				{
					a_down = true;
				}
				else if(message.wParam == Q_KEY_CODE)
				{
					q_down = true;
				}
			}
			else if(message.message == WM_KEYUP)
			{
				if(message.wParam == W_KEY_CODE)
				{
					w_down = false;
				}
				else if(message.wParam == S_KEY_CODE)
				{
					s_down = false;
				}
				else if(message.wParam == D_KEY_CODE)
				{
					d_down = false;
				}
				else if(message.wParam == A_KEY_CODE)
				{
					a_down = false;
				}
				else if(message.wParam == Q_KEY_CODE)
				{
					q_down = false;
				}
			}
			
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
		
		DrawRectangle(0, 0, backbuffer.width, backbuffer.height, backbuffer, V4(0.0f, 0.0f, 0.0f, 1.0f));
		DrawRectangle(100, 100, 100, 100, backbuffer, V4(position.x / (float)backbuffer.width, position.y / (float)backbuffer.height, 0.0f, 1.0f));
		DrawRectangle(100, 100, 150, 150, backbuffer, V4(0.0f, 1.0f, 0.0f, 0.25f));
		
		for(int i = 0; i < entities.count; i++)
		{
			Entity *entity = entities.entities + i;
			
			if(entity->type != EntityType_Invalid)
			{	
				Vector2 acceleration = V2(0, 0);
				
				if(entity == player_entity)
				{
					float multiplier = 1.0f;
					
					if(w_down)
					{
						acceleration = acceleration + V2(0, 1);
					}
					
					if(s_down)
					{
						acceleration = acceleration + V2(0, -1);
					}
					
					if(d_down)
					{
						acceleration = acceleration + V2(1, 0);
					}
					
					if(a_down)
					{
						acceleration = acceleration + V2(-1, 0);
					}
					
					if(q_down)
					{
						multiplier = 5.0f;
					}
					
					if(Length(acceleration) > 0)
					{
						acceleration = Normalize(acceleration) * multiplier;
					}
				}
				
				acceleration = acceleration - (entity->velocity * 0.05f);
				Vector2 new_velocity = (acceleration * timestep) + entity->velocity;
				Vector2 new_position = (acceleration * 0.5f * timestep * timestep) + (new_velocity * timestep) + GetCenter(entity->bounds);
				Rect2 collision_box = RectPosSize(new_position.x, new_position.y, GetWidth(entity->bounds), GetHeight(entity->bounds));
				bool intersects = false;
			
				for(int j = 0; j < entities.count; j++)
				{
					if(j != i)
					{
						Entity *other_entity = entities.entities + j;
						intersects = intersects || Intersect(other_entity->bounds, collision_box);
					}
				}
				
				if(!intersects)
				{
					entity->velocity = new_velocity;
					entity->bounds = RectPosSize(new_position.x, new_position.y, GetWidth(entity->bounds), GetHeight(entity->bounds));
				}
				else
				{
					entity->velocity = V2(0, 0);
				}
				
				switch(entity->type)
				{
					case EntityType_Box:
					{
						DrawRectangle(entity->bounds.min.x, entity->bounds.min.y,
									  GetWidth(entity->bounds), GetHeight(entity->bounds),
									  backbuffer, V4(1.0f, 0.0f, 0.0f, 1.0f));
					}
					break;
					
					case EntityType_Player:
					{
						float angle = GetAngle(entity->velocity);
						Direction direction = AngleToDirection(angle);
						Vector4 color = V4(1.0f, 1.0f, 1.0f, 0.0f);
						
						if(direction == Direction_Up)
						{
							color = V4(1.0f, 0.0f, 0.0f, 1.0f);
						}
						else if(direction == Direction_Down)
						{
							color = V4(1.0f, 1.0f, 0.0f, 1.0f);
						}
						else if(direction == Direction_Left)
						{
							color = V4(0.0f, 1.0f, 1.0f, 1.0f);
						}
						else if(direction == Direction_Right)
						{
							color = V4(0.0f, 1.0f, 0.0f, 1.0f);
						}
						
						char output[255];
						sprintf(output, "%f\n", angle);
						OutputDebugStringA(output);
						
						DrawRectangle(300,
									  300,
									  10, 10,
									  backbuffer, color);
						
						DrawRectangle(entity->bounds.min.x,
									  entity->bounds.min.y,
									  GetWidth(entity->bounds), GetHeight(entity->bounds),
									  backbuffer, intersects ? V4(0.5f, 0.0f, 0.0f, 0.5f) : V4(0.5f, 0.5f, 0.5f, 0.5f));
		
						DrawBitmap(entity->bounds.min.x,
								   entity->bounds.min.y,
								   GetWidth(entity->bounds), GetHeight(entity->bounds),
								   backbuffer, player_head);
					}
					break;
				}
			}
		}
		
		StretchDIBits(window_context,
					  0, 0, backbuffer.width, backbuffer.height,
					  0, 0, backbuffer.width, backbuffer.height,
					  backbuffer.pixels, &bitmap_info, DIB_RGB_COLORS, SRCCOPY);
	}
	
	return 0;
}