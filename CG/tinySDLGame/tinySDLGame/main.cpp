#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

/* Play page grid: 20x20 cells of 40x30 logical pixels; the block is one cell. */
#define CELL_W 40.0f
#define CELL_H 30.0f
#define GRID_COLS 20
#define GRID_ROWS 20

#define MOVE_SPEED 300.0f   /* px/s while holding a direction key */
#define GRAVITY 2600.0f     /* px/s^2, always pulls the block down */
#define JUMP_CELLS 3

#define NUM_COLORS 6

/* Which page is currently shown. */
enum class Page { Menu, Play, Setting };

/* What a button does when clicked. */
enum class Action { GoPlay, GoSetting, Exit, SetColor };

struct Button {
    SDL_FRect rect;
    const char* label;
    Action action;
    SDL_Color color;  /* used by SetColor swatches */
};

struct AppState {
    SDL_Window* window;
    SDL_Renderer* renderer;
    Page currentPage;
    Button menuButtons[3];
    Button colorButtons[NUM_COLORS];

    /* Play page state. */
    float blockX, blockY, blockVelY;
    bool blockGrounded;
    Uint64 lastTicks;
    SDL_Color blockColor;
};

static const SDL_Color SWATCH_COLORS[NUM_COLORS] = {
    { 150, 100, 60, 255 },  /* wood (default) */
    { 200, 60, 60, 255 },   /* red */
    { 60, 90, 200, 255 },   /* blue */
    { 60, 160, 60, 255 },   /* green */
    { 220, 190, 50, 255 },  /* yellow */
    { 150, 70, 180, 255 },  /* purple */
};

static bool pointInRect(float x, float y, const SDL_FRect& r)
{
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

static bool colorEquals(SDL_Color a, SDL_Color b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b;
}

/* Draw text centered on (cx, cy). Coordinates are in logical pixels. */
static void drawCenteredText(SDL_Renderer* renderer, float cx, float cy, float scale, const char* text)
{
    const float charPixels = (float)SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * scale;
    const float textWidth = charPixels * (float)SDL_strlen(text);

    /* RenderDebugText positions are in the current scaled space. */
    SDL_SetRenderScale(renderer, scale, scale);
    SDL_RenderDebugText(renderer, (cx - textWidth / 2.0f) / scale, (cy - charPixels / 2.0f) / scale, text);
    SDL_SetRenderScale(renderer, 1.0f, 1.0f);
}

static void drawButton(SDL_Renderer* renderer, const Button& b, bool hovered, bool selected)
{
    if (b.action == Action::SetColor) {
        SDL_SetRenderDrawColor(renderer, b.color.r, b.color.g, b.color.b, SDL_ALPHA_OPAQUE);
    } else if (b.action == Action::Exit) {
        SDL_SetRenderDrawColor(renderer, hovered ? 200 : 160, hovered ? 88 : 60, hovered ? 88 : 60, SDL_ALPHA_OPAQUE);
    } else {
        SDL_SetRenderDrawColor(renderer, hovered ? 96 : 66, hovered ? 128 : 92, hovered ? 196 : 156, SDL_ALPHA_OPAQUE);
    }
    SDL_RenderFillRect(renderer, &b.rect);

    if (selected) {
        SDL_SetRenderDrawColor(renderer, 255, 200, 0, SDL_ALPHA_OPAQUE);
    } else {
        SDL_SetRenderDrawColor(renderer, 210, 215, 230, SDL_ALPHA_OPAQUE);
    }
    SDL_RenderRect(renderer, &b.rect);
    if (selected) {
        const SDL_FRect inner = { b.rect.x + 2.0f, b.rect.y + 2.0f, b.rect.w - 4.0f, b.rect.h - 4.0f };
        SDL_RenderRect(renderer, &inner);
    }

    if (b.label[0] != '\0') {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
        drawCenteredText(renderer, b.rect.x + b.rect.w / 2.0f, b.rect.y + b.rect.h / 2.0f, 2.0f, b.label);
    }
}

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    SDL_SetAppMetadata("tinySDLGame", "1.0", "com.example.tinysdlgame");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    AppState* s = (AppState*)SDL_calloc(1, sizeof(AppState));
    if (!s) {
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("tinySDLGame", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE, &s->window, &s->renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        SDL_free(s);
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(s->renderer, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    const float buttonWidth = 260.0f;
    const float buttonHeight = 56.0f;
    const float buttonX = (WINDOW_WIDTH - buttonWidth) / 2.0f;
    const float firstButtonY = 220.0f;
    const float buttonGap = 24.0f;

    s->currentPage = Page::Menu;
    s->menuButtons[0] = { { buttonX, firstButtonY, buttonWidth, buttonHeight }, "PLAY", Action::GoPlay, {} };
    s->menuButtons[1] = { { buttonX, firstButtonY + (buttonHeight + buttonGap), buttonWidth, buttonHeight }, "SETTING", Action::GoSetting, {} };
    s->menuButtons[2] = { { buttonX, firstButtonY + 2.0f * (buttonHeight + buttonGap), buttonWidth, buttonHeight }, "EXIT", Action::Exit, {} };

    const float swatchW = 80.0f;
    const float swatchH = 60.0f;
    const float swatchGap = 20.0f;
    const float swatchTotal = NUM_COLORS * swatchW + (NUM_COLORS - 1) * swatchGap;
    const float swatchX0 = ((float)WINDOW_WIDTH - swatchTotal) / 2.0f;
    for (int i = 0; i < NUM_COLORS; i++) {
        s->colorButtons[i] = { { swatchX0 + i * (swatchW + swatchGap), 240.0f, swatchW, swatchH }, "", Action::SetColor, SWATCH_COLORS[i] };
    }

    /* Play page: block starts at the bottom center. */
    s->blockColor = SWATCH_COLORS[0];
    s->blockX = ((float)WINDOW_WIDTH - CELL_W) / 2.0f;
    s->blockY = (float)WINDOW_HEIGHT - CELL_H;
    s->blockVelY = 0.0f;
    s->blockGrounded = true;
    s->lastTicks = SDL_GetTicks();

    *appstate = s;
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    AppState* s = (AppState*)appstate;

    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* ends the program; SDL_AppQuit will clean up. */
    }

    if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE && !event->key.repeat) {
        if (s->currentPage != Page::Menu) {
            s->currentPage = Page::Menu;
        }
    }

    /* Space jumps 3 cells up, then gravity pulls the block back down. */
    if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_SPACE && !event->key.repeat &&
        s->currentPage == Page::Play && s->blockGrounded) {
        s->blockVelY = -SDL_sqrtf(2.0f * GRAVITY * (float)JUMP_CELLS * CELL_H);
        s->blockGrounded = false;
    }

    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_LEFT) {
        /* Event coordinates are in window pixels; convert them into our fixed
           logical size because the window uses letterbox presentation. */
        float x, y;
        SDL_RenderCoordinatesFromWindow(s->renderer, event->button.x, event->button.y, &x, &y);

        if (s->currentPage == Page::Menu) {
            for (const Button& b : s->menuButtons) {
                if (pointInRect(x, y, b.rect)) {
                    switch (b.action) {
                    case Action::GoPlay:
                        s->currentPage = Page::Play;
                        /* reset the block when entering the play page */
                        s->blockX = ((float)WINDOW_WIDTH - CELL_W) / 2.0f;
                        s->blockY = (float)WINDOW_HEIGHT - CELL_H;
                        s->blockVelY = 0.0f;
                        s->blockGrounded = true;
                        s->lastTicks = SDL_GetTicks();
                        break;
                    case Action::GoSetting:
                        s->currentPage = Page::Setting;
                        break;
                    case Action::Exit:
                        return SDL_APP_SUCCESS;  /* close the window; SDL_AppQuit destroys everything. */
                    default:
                        break;
                    }
                    break;
                }
            }
        } else if (s->currentPage == Page::Setting) {
            for (const Button& b : s->colorButtons) {
                if (pointInRect(x, y, b.rect)) {
                    s->blockColor = b.color;
                    break;
                }
            }
        }
    }

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void* appstate)
{
    AppState* s = (AppState*)appstate;

    switch (s->currentPage) {
    case Page::Menu: {
        SDL_SetRenderDrawColor(s->renderer, 25, 34, 56, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(s->renderer);

        SDL_SetRenderDrawColor(s->renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
        drawCenteredText(s->renderer, WINDOW_WIDTH / 2.0f, 110.0f, 3.0f, "TINY SDL GAME");

        float mouseX, mouseY, x, y;
        SDL_GetMouseState(&mouseX, &mouseY);
        SDL_RenderCoordinatesFromWindow(s->renderer, mouseX, mouseY, &x, &y);

        for (const Button& b : s->menuButtons) {
            drawButton(s->renderer, b, pointInRect(x, y, b.rect), false);
        }
        break;
    }
    case Page::Play: {
        /* --- physics --- */
        Uint64 now = SDL_GetTicks();
        float dt = (float)(now - s->lastTicks) / 1000.0f;
        s->lastTicks = now;
        if (dt > 0.1f) {
            dt = 0.1f;  /* avoid huge steps after a pause */
        }

        const bool* keys = SDL_GetKeyboardState(nullptr);
        const bool left = keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A];
        const bool right = keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D];
        const bool up = keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W];
        const bool down = keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S];

        if (left && !right) {
            s->blockX -= MOVE_SPEED * dt;
        }
        if (right && !left) {
            s->blockX += MOVE_SPEED * dt;
        }

        if (up && !down) {
            /* holding up lifts the block; gravity takes over on release */
            s->blockY -= MOVE_SPEED * dt;
            s->blockVelY = 0.0f;
            s->blockGrounded = false;
        } else {
            s->blockVelY += GRAVITY * (down && !up ? 2.0f : 1.0f) * dt;  /* holding down falls faster */
            s->blockY += s->blockVelY * dt;
        }

        const float floorY = (float)WINDOW_HEIGHT - CELL_H;
        if (s->blockY >= floorY) {
            s->blockY = floorY;
            s->blockVelY = 0.0f;
            s->blockGrounded = true;
        } else if (s->blockY < 0.0f) {
            s->blockY = 0.0f;
            if (s->blockVelY < 0.0f) {
                s->blockVelY = 0.0f;
            }
        }
        if (s->blockX < 0.0f) {
            s->blockX = 0.0f;
        }
        if (s->blockX > (float)WINDOW_WIDTH - CELL_W) {
            s->blockX = (float)WINDOW_WIDTH - CELL_W;
        }

        /* --- draw --- */
        SDL_SetRenderDrawColor(s->renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);  /* white page */
        SDL_RenderClear(s->renderer);

        SDL_SetRenderDrawColor(s->renderer, 215, 215, 215, SDL_ALPHA_OPAQUE);
        for (int col = 0; col <= GRID_COLS; col++) {
            const float gx = (float)col * CELL_W;
            SDL_RenderLine(s->renderer, gx, 0.0f, gx, (float)WINDOW_HEIGHT);
        }
        for (int row = 0; row <= GRID_ROWS; row++) {
            const float gy = (float)row * CELL_H;
            SDL_RenderLine(s->renderer, 0.0f, gy, (float)WINDOW_WIDTH, gy);
        }

        const SDL_FRect block = { s->blockX, s->blockY, CELL_W, CELL_H };
        SDL_SetRenderDrawColor(s->renderer, s->blockColor.r, s->blockColor.g, s->blockColor.b, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(s->renderer, &block);
        SDL_SetRenderDrawColor(s->renderer,
                               (Uint8)(s->blockColor.r * 0.6f),
                               (Uint8)(s->blockColor.g * 0.6f),
                               (Uint8)(s->blockColor.b * 0.6f),
                               SDL_ALPHA_OPAQUE);
        SDL_RenderRect(s->renderer, &block);

        SDL_SetRenderDrawColor(s->renderer, 130, 130, 130, SDL_ALPHA_OPAQUE);
        drawCenteredText(s->renderer, WINDOW_WIDTH / 2.0f, 15.0f, 1.5f, "ARROWS/WASD: MOVE   SPACE: JUMP 3 CELLS   ESC: BACK");
        break;
    }
    case Page::Setting: {
        SDL_SetRenderDrawColor(s->renderer, 0, 170, 0, SDL_ALPHA_OPAQUE);  /* green page */
        SDL_RenderClear(s->renderer);

        SDL_SetRenderDrawColor(s->renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
        drawCenteredText(s->renderer, WINDOW_WIDTH / 2.0f, 110.0f, 2.5f, "SETTING PAGE");
        drawCenteredText(s->renderer, WINDOW_WIDTH / 2.0f, 195.0f, 2.0f, "BLOCK COLOR");

        float mouseX, mouseY, x, y;
        SDL_GetMouseState(&mouseX, &mouseY);
        SDL_RenderCoordinatesFromWindow(s->renderer, mouseX, mouseY, &x, &y);

        for (const Button& b : s->colorButtons) {
            drawButton(s->renderer, b, pointInRect(x, y, b.rect), colorEquals(s->blockColor, b.color));
        }

        drawCenteredText(s->renderer, WINDOW_WIDTH / 2.0f, 560.0f, 2.0f, "PRESS ESC TO GO BACK");
        break;
    }
    }

    SDL_RenderPresent(s->renderer);

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. Destroy all allocated resources here. */
void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    AppState* s = (AppState*)appstate;
    if (s) {
        if (s->renderer) {
            SDL_DestroyRenderer(s->renderer);
        }
        if (s->window) {
            SDL_DestroyWindow(s->window);
        }
        /* SDL frees appstate itself and calls SDL_Quit() after this returns. */
    }
}
