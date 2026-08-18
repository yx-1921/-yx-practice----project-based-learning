#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;

constexpr int SCREEN_WIDTH = 640;
constexpr int SCREEN_HEIGHT = 480;
constexpr float PLAYER_SPEED = 200.0f; // 像素/秒
int g_MonitorWidth = SCREEN_WIDTH;
int g_MonitorHeight = SCREEN_HEIGHT;


SDL_AppResult SDL_AppInit(void** appstate, int agrc, char* argv[])
{
	SDL_SetAppMetadata("tinySDLPro", "v_1.0", "yx");
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

SDL_AppResult SDL_AppIterate(void* appstate) { // TEST
    SDL_FRect rect;

    /* as you can see from this, rendering draws over whatever was drawn before it. */
    SDL_SetRenderDrawColor(renderer, 33, 33, 33, SDL_ALPHA_OPAQUE);  /* dark gray, full alpha */
    SDL_RenderClear(renderer);  /* start with a blank canvas. */

    /* draw a filled rectangle in the middle of the canvas. */
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, SDL_ALPHA_OPAQUE);  /* blue, full alpha */
    rect.x = rect.y = 100;
    rect.w = 440;
    rect.h = 280;
    SDL_RenderFillRect(renderer, &rect);

    ///* draw a unfilled rectangle in-set a little bit. */
    //SDL_SetRenderDrawColor(renderer, 0, 255, 0, SDL_ALPHA_OPAQUE);  /* green, full alpha */
    //rect.x += 30;
    //rect.y += 30;
    //rect.w -= 60;
    //rect.h -= 60;
    //SDL_RenderRect(renderer, &rect);

    ///* draw two lines in an X across the whole canvas. */
    //SDL_SetRenderDrawColor(renderer, 255, 255, 0, SDL_ALPHA_OPAQUE);  /* yellow, full alpha */
    //SDL_RenderLine(renderer, 0, 0, 640, 480);
    //SDL_RenderLine(renderer, 0, 480, 640, 0);

    SDL_RenderPresent(renderer);  /* put it all on the screen! */

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}



void SDL_AppQuit(void* appstate, SDL_AppResult result) {

}