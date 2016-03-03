#include "windows.h"
#include "stdint.h"
#include "math.h"

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

Rect2 RectPosSize(float x, float y, float width, float height)
{
	Rect2 result = {};
	
	result.min.x = x - 0.5f * width;
	result.min.y = y - 0.5f * height;
	result.max.x = x + 0.5f * width;
	result.max.y = y + 0.5f * height;
	
	return result;
}

/*
float GetWidth()
{
	
}

Rect2 GrowBy(Rect2 base, Rect2 extension)
{
	Rect2 result = base;
	
	result.min.x = result.min.x - extension.;
	result.height = result.height + extension.height;
	
	return result;
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
	return Contains(collision_rect, V2(rect2.x, rect2.y));
}
*/

int RoundFloatToInt(float in)
{
	int output = (int)(in + 0.5f);
	return output;
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

void DrawBitmap(int x_in, int y_in, int width, int height, Framebuffer fbo, uint32_t *bmap_pixel_data)
{
	int bitmap_width = width;
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
			
			uint32_t texel_color = *(bmap_pixel_data + ((y + y_offset) * bitmap_width) + x + x_offset);
			
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

	HANDLE bitmap_handle = CreateFileA("petro.bmp", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	int bitmap_file_size = GetFileSize(bitmap_handle, NULL);
	uint8_t *bitmap_file_data = (uint8_t *)VirtualAlloc(NULL, bitmap_file_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	DWORD number_of_bytes_read = 0;
	ReadFile(bitmap_handle, bitmap_file_data, bitmap_file_size, &number_of_bytes_read, NULL);
	CloseHandle(bitmap_handle);
	
	bitmap_header *header = (bitmap_header *)bitmap_file_data;
	int bitmap_width = header->width;
	int bitmap_height = header->height;
	uint32_t *bmap_pixel_data = (uint32_t *)(bitmap_file_data + header->bitmap_offset);
	
	MSG message;
	bool open = true;
	Vector2 velocity = V2(0, 0);
	Vector2 position = V2(100, 100);
	
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
		
		float timestep = 1.0f; //TODO proper timestep
		float multiplier = 1.0f;
		Vector2 acceleration = V2(0, 0);
		
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
		
		acceleration = acceleration - (velocity * 0.05f);
		velocity = (acceleration * timestep) + velocity;
		position = (acceleration * 0.5f * timestep * timestep) + (velocity * timestep) + position;
		
		DrawRectangle(0, 0, backbuffer.width, backbuffer.height, backbuffer, V4(0.0f, 0.0f, 0.0f, 1.0f));
		DrawRectangle(100, 100, 100, 100, backbuffer, V4(position.x / (float)backbuffer.width, position.y / (float)backbuffer.height, 0.0f, 1.0f));
		DrawRectangle(100, 100, 150, 150, backbuffer, V4(0.0f, 1.0f, 0.0f, 0.25f));
		
		//Rect2 static_collision_box = RectPosSize(0, 0, 100, 100);
		DrawRectangle(0, 0, 100, 100, backbuffer, V4(1.0f, 0.0f, 0.0f, 1.0f));
		
		//Rect2 collision_box = RectPosSize(position.x, position.y, bitmap_width, bitmap_height);
		bool intersects = false; /*Intersect(static_collision_box, collision_box);*/
		
		/*
		DrawRectangle(collision_box.x - 0.5f * collision_box.width,
					  collision_box.y - 0.5f * collision_box.height,
					  collision_box.width, collision_box.height, backbuffer, intersects ? V4(0.5f, 0.0f, 0.0f, 0.5f) : V4(0.5f, 0.5f, 0.5f, 0.5f));
		*/
		
		DrawBitmap(position.x - 0.5f * bitmap_width,
		           position.y - 0.5f * bitmap_height,
				   bitmap_width, bitmap_height, backbuffer, bmap_pixel_data);
		
		StretchDIBits(window_context,
					  0, 0, backbuffer.width, backbuffer.height,
					  0, 0, backbuffer.width, backbuffer.height,
					  backbuffer.pixels, &bitmap_info, DIB_RGB_COLORS, SRCCOPY);
	}
	
	return 0;
}