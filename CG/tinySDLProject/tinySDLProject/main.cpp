#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

static SDL_Window *window = NULL;
static SDL_Renderer* renderer = NULL;

int g_MonitorWidth = 640;
int g_MonitorHeight = 480;
SDL_DisplayID display_id = -1;
SDL_Rect display_bounds = { 0, 0, 0, 0 };


SDL_AppResult SDL_AppInit(void** appstate, int agrc, char* argv[])
{
	SDL_Init(SDL_INIT_VIDEO);


	//display_id = SDL_GetPrimaryDisplay();
	//if (SDL_GetDisplayBounds(display_id, &display_bounds)) {
	//	g_MonitorWidth = display_bounds.w;
	//	g_MonitorHeight = display_bounds.h;
	//}


	SDL_CreateWindowAndRenderer("sdlTest", g_MonitorWidth, g_MonitorHeight, SDL_WINDOW_RESIZABLE, &window, &renderer);
	SDL_SetRenderLogicalPresentation(renderer, g_MonitorWidth, g_MonitorHeight, SDL_LOGICAL_PRESENTATION_LETTERBOX);

	return SDL_APP_CONTINUE;  /* carry on with the program! */
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
	if (event->type == SDL_EVENT_QUIT)
		return SDL_APP_SUCCESS;
	else if (event->type == SDL_EVENT_KEY_DOWN) {
		if (event->key.key == SDLK_ESCAPE)
			return SDL_APP_SUCCESS;
	}
		
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
	return SDL_APP_CONTINUE;
}



void SDL_AppQuit(void* appstate, SDL_AppResult result) {

}