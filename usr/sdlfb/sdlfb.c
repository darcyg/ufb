#include <SDL2/SDL.h>

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "ufb.h"

#define WIDTH  (640)
#define HEIGHT (480)

#define SCREEN_SIZE (WIDTH * HEIGHT * sizeof(uint32_t))
#define SCREEN_PITCH (WIDTH * sizeof(uint32_t))

#define VMEM_SIZE (10 * 1024 * 1024)

ufb_context_t *ufb;

SDL_Texture *texture;
SDL_Window *displayWindow;
SDL_Renderer *displayRenderer;

void setData( uint8_t value )
{
	memset(ufb_get_vmem(ufb), value, SCREEN_SIZE);
}

void writeTexture( void )
{
	SDL_UpdateTexture(texture, NULL, ufb_get_vmem(ufb), SCREEN_PITCH);

	SDL_RenderClear(displayRenderer);
	SDL_RenderCopy(displayRenderer, texture, NULL, NULL);
	SDL_RenderPresent(displayRenderer);
}

int main()
{
	int iterations = 0;

	if( UFB_OK != ufb_init(&ufb, WIDTH, HEIGHT, VMEM_SIZE) ) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
		                         "SDLFB ERROR",
		                         "Error initializing device",
		                         NULL);
		return 1;
	}

	SDL_Init(SDL_INIT_VIDEO);

	displayWindow = SDL_CreateWindow("usrfb", SDL_WINDOWPOS_UNDEFINED, 
	                                 SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT,
	                                 SDL_WINDOW_OPENGL);
	displayRenderer = SDL_CreateRenderer(displayWindow, -1, 
	                                     SDL_RENDERER_ACCELERATED |
	                                     SDL_RENDERER_PRESENTVSYNC);

	texture = SDL_CreateTexture(displayRenderer, 
	                            SDL_PIXELFORMAT_ARGB8888,
	                            SDL_TEXTUREACCESS_STREAMING,
	                            WIDTH, HEIGHT);

	while( 1 ) {
		SDL_Event e;
		if( SDL_PollEvent(&e) ) {
			if(e.type == SDL_QUIT) {
				break;
			}
		}

		//setData( iterations );

		writeTexture();

		iterations++;
	}

	ufb_free(ufb);

	return 0;
}

