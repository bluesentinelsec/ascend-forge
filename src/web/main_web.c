/**
 * @file main_web.c
 * @brief HTML5 canvas demo: a minimal Emscripten game loop.
 *
 * Per-frame updates use a real delta time. The browser owns frame pacing
 * through emscripten_set_main_loop (requestAnimationFrame).
 */

#include "ascend_forge/version.h"

#include <emscripten.h>
#include <emscripten/html5.h>

#include <stdio.h>

/* Renders one frame through the canvas 2D context. EM_JS defines the function
 * once in JavaScript; calls from C are ordinary function calls.
 * clang-format off
 * The EM_JS body is JavaScript: clang-format must not touch it (it splits
 * JS operators like !== into invalid `!= =`).
 */
EM_JS(void, ascend_forge_draw_frame, (double x, double y, double size, const char *hud), {
    const canvas = document.getElementById('canvas');
    const dpr = window.devicePixelRatio || 1;
    const width = canvas.clientWidth * dpr;
    const height = canvas.clientHeight * dpr;
    if (canvas.width !== width || canvas.height !== height) {
        canvas.width = width;
        canvas.height = height;
    }
    const g = canvas.getContext('2d');
    g.fillStyle = '#101418';
    g.fillRect(0, 0, canvas.width, canvas.height);
    g.fillStyle = '#4fc3f7';
    g.fillRect(x * canvas.width, y * canvas.height, size * dpr, size * dpr);
    g.fillStyle = '#e0e0e0';
    /* Note: no JS template literals here — clang tokenizes the EM_JS body and
     * rejects '$' under -Werror (-Wdollar-in-identifier-extension). */
    g.font = (14 * dpr) + 'px monospace';
    g.fillText(UTF8ToString(hud), 12 * dpr, 24 * dpr);
/* EM_JS is already a declaration in C. A semicolon here is -Wextra-semi. */
})
/* clang-format on */

typedef struct GameState {
    double x;
    double y;
    double vx;
    double vy;
    double last_ms;
    double frames;
    char hud[160];
} GameState;

static GameState g_state = {0.1, 0.1, 0.25, 0.2, 0.0, 0.0, ""};

static void updateState(GameState *state, double dt)
{
    state->x += state->vx * dt;
    state->y += state->vy * dt;
    if (state->x < 0.0 || state->x > 0.95) {
        state->vx = -state->vx;
    }
    if (state->y < 0.0 || state->y > 0.95) {
        state->vy = -state->vy;
    }
    state->frames += 1.0;
}

static void frame(void)
{
    const double now_ms = emscripten_get_now();
    double dt = (now_ms - g_state.last_ms) / 1000.0;
    int written = 0;

    if (g_state.last_ms == 0.0 || dt > 0.1) {
        dt = 1.0 / 60.0;
    }
    g_state.last_ms = now_ms;
    updateState(&g_state, dt);
    written = snprintf(g_state.hud, sizeof g_state.hud,
                       "ascend-forge %s — wasm game loop, frame %.0f",
                       Version_String(), g_state.frames);
    if (written < 0) {
        g_state.hud[0] = '\0';
    }
    ascend_forge_draw_frame(g_state.x, g_state.y, 48.0, g_state.hud);
}

int main(void)
{
    emscripten_set_main_loop(frame, 0, true);
    return 0;
}
