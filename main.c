#include <stdlib.h>
#include <unistd.h>
#include <argp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include "include/raylib.h"
#include "include/raymath.h"

/* window_t */

enum mode_t {
    MODE_BRUSH,
    MODE_SHAPE_TRIANGLE,
    MODE_DELETE,
    MODE_SHAPE_LINE,
    MODE_BEZIER,
};

typedef struct {
    int width, height;
} window_t;

typedef struct {
    Vector2 pos;
    int     height;
    int     items;
} toolbar_t;

typedef struct {
    Vector2   pos, lastpos;
    bool      left, right, wheel;
    bool      heldleft, heldright, heldwheel;
    int       wheelMove;
    Texture2D shape;
} cursor_t;

#define TOOLBARHEIGHT       80
#define TOOLBARITEMS        4
#define TOOLBARBC           0x999999EE
#define TOOLBARSEPERATORCOL 0x111111EE
#define TOOLBARSEPERATOR    2
#define WIDTH               1920
#define HEIGHT              1080


toolbar_t toolbar = {
    {0, 0},
    TOOLBARHEIGHT,
    TOOLBARITEMS,
};

typedef struct {
    Vector2 pos;
    int     height, width;
    int     closeHide; /* Hide - Close */
    bool    closed;
} menu_t;

static int toolbarDistance;

window_t window = {0};
cursor_t cursor = {0};
menu_t   menu = {0};
bool     palletOpen = false;

Texture2D simpleText, shapesText, linesText, bezierText;

/* */

mode_t Brush();
mode_t Delete();
mode_t Line();
mode_t Bezier();

bool collisionMenu;

void pop_brush_menu();
void mode_shape_line();

void  menu_update(bool isopen);
void  draw_cursor();
void  reset();
void  colors();
void  mode_bezier();
void  mode_delete();
void  populate_pallete();
void  loadIcons();
float chebyshevDistance(Vector2 p1, Vector2 p2);

#define MENUWIDTH 100

RenderTexture canvas, background;
Vector2       canvasPosition = {0, 0};
float         zoom = 1.0f;
Color         chosenColor;
bool          draw = true;

Rectangle pallete[256] = {0};
Texture2D recyclebin, brush, rubber, line, bezier, gear;

int main(int argc, char **argv)
{
    chosenColor = GetColor(0x000000FF);
    window.height = HEIGHT;
    window.width = WIDTH;

    InitWindow(window.width, window.height, "paxx");

    menu.pos.x = window.width - MENUWIDTH;
    menu.pos.y = 100;
    menu.height = 600;
    menu.width = MENUWIDTH;
    menu.closeHide = 50;
    menu.closed = false;

    simpleText = LoadTexture("cursor.png");
    brush = LoadTexture("assets/paintbrush.png");
    rubber = LoadTexture("assets/rubber.png");
    line = LoadTexture("assets/line.png");
    bezier = LoadTexture("assets/bezier.png");
    gear = LoadTexture("assets/gear.png");

    cursor.shape = simpleText;

    toolbarDistance = window.width / toolbar.items;

    mode_t mode = MODE_BRUSH;

    Rectangle toolbarBC = {
        toolbar.pos.x,
        toolbar.pos.y,
        window.width,
        toolbar.height,
    };

    mode_t (*toolbarItems[])() = {
        &Brush,
        &Delete,
        &Line,
        &Bezier,
    };

    SetTargetFPS(180);
    HideCursor();


    canvas = LoadRenderTexture(4 * window.width, 4 * window.height);
    background = LoadRenderTexture(window.width, window.height);

    Rectangle canvasSource = {0, 0, 4 * window.width, -4 * window.height};
    Rectangle canvasSourceWindow = {0, 0, window.width * 4, window.height * 4};

    Rectangle BCSource = {0, 0, window.width, -window.height};

    Rectangle BCSourceWindow = {-100, -100, 32 * window.width,
                                32 * window.height};

    Vector2 BCPosition = {0, 0};

    BeginTextureMode(canvas);
    ClearBackground(RAYWHITE);
    EndTextureMode();

    BeginTextureMode(background);
    ClearBackground(GetColor(0xAAAAAAEE));
    EndTextureMode();

    cursor.lastpos = GetMousePosition();

    float dx = 0, dy = 0;
    // float offsetx = 0, offsety = 0;
    bool exitWindow = true;
    SetConfigFlags(FLAG_MSAA_4X_HINT);

    populate_pallete();
    while (exitWindow) {
        Rectangle menuBoundingBox = {
            menu.pos.x,
            menu.pos.y,
            menu.width,
            menu.height,
        };

        if (IsKeyPressed(KEY_ESCAPE) || WindowShouldClose())
            exitWindow = false;

        ClearBackground(RED);

        cursor.pos = GetMousePosition();
        cursor.left = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        cursor.right = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);

        cursor.heldleft = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        cursor.heldright = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
        cursor.wheelMove = GetMouseWheelMove();

        BeginDrawing();


        collisionMenu = CheckCollisionPointRec(cursor.pos, menuBoundingBox);
        SetTextureWrap(canvas.texture, TEXTURE_WRAP_CLAMP);
        DrawTexturePro(background.texture, BCSource, BCSourceWindow, BCPosition,
                       0, WHITE);

        DrawTexturePro(canvas.texture, canvasSource, canvasSourceWindow,
                       canvasPosition, 0, WHITE);

        DrawText(TextFormat("FPS: %i", GetFPS()), WIDTH - 100, HEIGHT - 20, 20,
                 MAROON);



        if (cursor.heldright) {
            canvasPosition.x += dx;
            canvasPosition.y += dy;
        }

        if (cursor.wheelMove > 0) {
            zoom *= 1.1f;
            canvasSourceWindow.width *= 1.1f;
            canvasSourceWindow.height *= 1.1f;
        }
        else if (cursor.wheelMove < 0) {
            zoom /= 1.1f;
            canvasSourceWindow.width /= 1.1f;
            canvasSourceWindow.height /= 1.1f;
        }

        bool l = menu.closed;
        menu_update(menu.closed);
        if (l != menu.closed)
            draw = false;

        /* Get the tool bar ready */

        DrawRectangleRec(toolbarBC, GetColor(TOOLBARBC));

        {
            Vector2 pos = toolbar.pos;
            Vector2 temp = {pos.x, pos.y + toolbar.height};

            for (int i = 0; i < toolbar.items - 1; ++i) {
                pos.x += toolbarDistance;
                temp.x = pos.x;
                DrawLineEx(pos, temp, TOOLBARSEPERATOR,
                           GetColor(TOOLBARSEPERATORCOL));
            }

            /* Draw tool bar items */

            pos = toolbar.pos;

            for (int i = 0; i < toolbar.items; ++i) {
                Rectangle boundingBox = {pos.x, pos.y,
                                         toolbarDistance - TOOLBARSEPERATOR,
                                         toolbar.height};

                bool collision = CheckCollisionPointRec(cursor.pos,
                                                        boundingBox);

                if (collision) {
                    DrawRectangleRec(boundingBox, collision ? RED : BLACK);
                    if (cursor.left) {
                        mode = toolbarItems[i]();
                        draw = false;
                    }
                }
                pos.x += toolbarDistance;
            }
        }

        loadIcons();
        // printf("Draw %d \n", draw);
        switch (mode) {
            case MODE_BRUSH:
                // printf("MODE_BRUSH \n");
                pop_brush_menu();
                // mode_brush();
                break;

            case MODE_DELETE:
                // printf("MODE_SHAPE_TRIANGLE\n");
                mode_delete();
                break;

            case MODE_SHAPE_TRIANGLE:
                // printf("MODE_SHAPE_TRIANGLE\n");
                // mode_shape_triangle();
                break;

            case MODE_SHAPE_LINE:
                // printf("MODE_SHAPE_LINE \n");
                mode_shape_line();
                break;

            case MODE_BEZIER:
                // printf("MODE_BEZIER \n");
                mode_bezier();
                break;
        }
        reset();
        colors();

        draw_cursor();

        dx = cursor.lastpos.x - cursor.pos.x;
        dy = cursor.lastpos.y - cursor.pos.y;

        cursor.lastpos = cursor.pos;
        EndDrawing();
        draw = true;
    }
    // BeginTextureMode(canvas);
    UnloadTexture(gear);
    UnloadTexture(simpleText);
    UnloadTexture(line);
    UnloadTexture(bezier);
    UnloadTexture(brush);
    // EndTextureMode();
    CloseWindow();
    return 0;
}

mode_t Brush()
{
    return MODE_BRUSH;
}

mode_t Delete()
{
    return MODE_DELETE;
}

mode_t Line()
{
    return MODE_SHAPE_LINE;
}

mode_t Bezier()
{
    return MODE_BEZIER;
}

void draw_cursor()
{
    Rectangle textureSource = {
        0,
        0,
        simpleText.width,
        simpleText.height,
    };

    Rectangle textureDest = {
        cursor.pos.x,
        cursor.pos.y,
        20,
        20,
    };

    Vector2 origin = {6, 5};

    DrawTexturePro(simpleText, textureSource, textureDest, origin, 0, WHITE);
}

#define MENUBC           0xAAAAAAEE
#define MENUPANEL        0x00AAAAEE
#define MENUPANELTOUCHED 0x009999EE

void menu_update(bool open)
{
    if (open) {
        Rectangle main = {menu.pos.x, menu.pos.y, menu.width, menu.height};
        Rectangle panel = {menu.pos.x, menu.pos.y, menu.width, menu.closeHide};

        bool collision = CheckCollisionPointRec(cursor.pos, panel);

        DrawRectangleRec(main, GetColor(MENUBC));
        DrawRectangleRec(panel,
                         GetColor(collision ? MENUPANELTOUCHED : MENUPANEL));
        if (cursor.left && collision)
            menu.closed = false;
    }
    else {
        Rectangle hiddenPanel = {menu.pos.x + menu.width / 2, menu.pos.y,
                                 menu.width / 2, menu.closeHide * 2};

        bool collision = CheckCollisionPointRec(cursor.pos, hiddenPanel);

        DrawRectangleRec(hiddenPanel,
                         GetColor(collision ? MENUPANELTOUCHED : MENUPANEL));
        if (cursor.left && collision)
            menu.closed = true;
    }
}

Vector2 toMenu(Vector2 p)
{
    return (Vector2) {
        .x = p.x + menu.pos.x,
        .y = p.y + menu.pos.y + menu.closeHide,
    };
}

#define BRUSHCOL        0x333333EE
#define BRUSHCOLTOUCHED 0x555555EE

void pop_brush_menu()
{
    bool closed = 1 - menu.closed;

    int size1 = 5;
    int size2 = 10;
    int size3 = 20;
    int size4 = 30;

    static int brush = 5;

    if (!closed) {
        Vector2 center1 = {menu.width / 2, 30};
        Vector2 center2 = {menu.width / 2, center1.y + 20};
        Vector2 center3 = {menu.width / 2, center1.y + 60};
        Vector2 center4 = {menu.width / 2, center1.y + 120};

        bool col1 = CheckCollisionPointCircle(cursor.pos, toMenu(center1),
                                              size1);
        bool col2 = CheckCollisionPointCircle(cursor.pos, toMenu(center2),
                                              size2);
        bool col3 = CheckCollisionPointCircle(cursor.pos, toMenu(center3),
                                              size3);
        bool col4 = CheckCollisionPointCircle(cursor.pos, toMenu(center4),
                                              size4);
        DrawCircleV(toMenu(center1), size1,
                    GetColor(col1 ? BRUSHCOLTOUCHED : BRUSHCOL));

        DrawCircleV(toMenu(center2), size2,
                    GetColor(col2 ? BRUSHCOLTOUCHED : BRUSHCOL));

        DrawCircleV(toMenu(center3), size3,
                    GetColor(col3 ? BRUSHCOLTOUCHED : BRUSHCOL));

        DrawCircleV(toMenu(center4), size4,
                    GetColor(col4 ? BRUSHCOLTOUCHED : BRUSHCOL));

        if (col1 && cursor.left) {
            brush = size1;
            return;
        }

        if (col2 && cursor.left) {
            brush = size2;
            return;
        }

        if (col3 && cursor.left) {
            brush = size3;
            return;
        }

        if (col4 && cursor.left) {
            brush = size4;
            return;
        }
    }

    if (!closed)
        if (cursor.left && collisionMenu) {
            return;
        }

    Vector2 mapped = {
        (cursor.pos.x + canvasPosition.x) * 1 / zoom,
        (cursor.pos.y + canvasPosition.y) * 1 / zoom,
    };

    Vector2 lastmapped = {
        (cursor.lastpos.x + canvasPosition.x) * 1 / zoom,
        (cursor.lastpos.y + canvasPosition.y) * 1 / zoom,
    };

    if (cursor.left || cursor.heldleft) {
        BeginTextureMode(canvas);
        //  DrawLineEx(mapped, lastmapped, brush, chosenColor);
        for (int i = 0; i < 20; ++i) {
            DrawCircleV(Vector2Lerp(mapped, lastmapped, (float)i / 10.0f),
                        brush, chosenColor);
        }
        EndTextureMode();
    }
}

#define RESETHEIGHT           30
#define RESETWIDTH            30
#define RESETBUTTONCOL        0xAA00AAEE
#define RESETBUTTONTOUCHEDCOL 0xCC00CCEE

void reset()
{
    static Rectangle resetButton = {
        0,
        HEIGHT - RESETHEIGHT,
        RESETWIDTH,
        RESETHEIGHT,
    };
    bool col = CheckCollisionPointRec(cursor.pos, resetButton);
    DrawRectangleRec(resetButton,
                     GetColor(col ? RESETBUTTONTOUCHEDCOL : RESETBUTTONCOL));

    if (cursor.left && col) {
        BeginTextureMode(canvas);
        ClearBackground(RAYWHITE);
        EndTextureMode();
    }
}

float chebyshevDistance(Vector2 p1, Vector2 p2)
{
    float dx = fabsf(p2.x - p1.x);
    float dy = fabsf(p2.y - p1.y);
    return fmaxf(dx, dy);
}

#define COLORPALLETEWIDTH      40
#define COLORPALLETECOL        0x00AA11EE
#define COLORPALLETETOUCHEDCOL 0x00DD33EE
#define COLORPALLETEHEIGHT     80
#define COLORPALLETEY          600
#define COLORPALLETEX          0

#define COLORPALLETEOPENCOL        0x00AA11FF
#define COLORPALLETEOPENTOUCHEDCOL 0x00DD33FF

#define COLORPALLETEOPENW 640
#define COLORPALLETEOPENH 160

#define COLORPALLETEDX 20
#define COLORPALLETEDY 20

void populate_pallete()
{
    for (int j = 0; j < 8; ++j) {
        for (int i = 0; i < 32; ++i) {
            pallete[j * 32 + i].x = COLORPALLETEX + i * COLORPALLETEDX;
            pallete[j * 32 + i].y = COLORPALLETEY + j * COLORPALLETEDY;
            pallete[j * 32 + i].width = COLORPALLETEDX;
            pallete[j * 32 + i].height = COLORPALLETEDY;
        }
    }
}

void colorsHUD()
{
    if (palletOpen) {
        Rectangle pallet = {
            COLORPALLETEX,
            COLORPALLETEY,
            COLORPALLETEOPENW,
            COLORPALLETEOPENH,
        };

        bool col = CheckCollisionPointRec(cursor.pos, pallet);
        DrawRectangleRec(pallet, GetColor(col ? COLORPALLETEOPENTOUCHEDCOL
                                              : COLORPALLETEOPENCOL));

        for (int i = 0; i < 256; ++i) {
            Color temp = ColorFromHSV(360.0f * (float)i / 256.0f, 0.9, 0.9);
            if (i == 0)
                temp = GetColor(0x000000FF);
            if (i == 255)
                temp = GetColor(0xFFFFFFFF);
            DrawRectangleRec(pallete[i], temp);
            DrawRectangleLinesEx(pallete[i], 2, GetColor(0x000000EE));
        }


        if (col) {
            for (int i = 0; i < 256; ++i) {
                bool z = CheckCollisionPointRec(cursor.pos, pallete[i]);

                Color temp = ColorFromHSV(360.0f * (float)i / 256.0f, 0.9, 0.9);
                if (i == 0)
                    temp = GetColor(0x000000FF);
                if (i == 255)
                    temp = GetColor(0xFFFFFFFF);

                if (cursor.left && z)
                    chosenColor = temp;
            }
        }

        if (col && cursor.left) {
            palletOpen = false;
        }
    }
    else {
        Rectangle pallet = {
            COLORPALLETEX,
            COLORPALLETEY,
            COLORPALLETEWIDTH,
            COLORPALLETEHEIGHT,
        };
        bool col = CheckCollisionPointRec(cursor.pos, pallet);
        DrawRectangleRec(
            pallet, GetColor(col ? COLORPALLETETOUCHEDCOL : COLORPALLETECOL));

        if (col && cursor.left) {
            palletOpen = true;
        }
    }

#define SQUAREX (RESETWIDTH + 20)
#define SQUAREY (HEIGHT - 20)
    DrawRectangle(SQUAREX, SQUAREY, 20, 20, chosenColor);
}

void colors()
{
    colorsHUD();

    if (!palletOpen) {
        return;
    }
}

#define LINECOL        0x333333EE
#define LINECOLTOUCHED 0x330033EE

void mode_shape_line()
{
    static Vector2 p1 = {0};
    static bool    first = false;
    static bool    second = false;

    static int size = 2;

    bool closed = 1 - menu.closed;

    if (!closed) {
        int size1 = 2;
        int size2 = 4;
        int size3 = 8;
        int size4 = 16;
        int size5 = 32;

        Vector2 center1 = {5, 20};
        Vector2 center2 = {5, 30};
        Vector2 center3 = {5, 50};
        Vector2 center4 = {5, 80};

        Vector2 ccenter1 = {center1.x + MENUWIDTH - 10, 20};
        Vector2 ccenter2 = {center2.x + MENUWIDTH - 10, 30};
        Vector2 ccenter3 = {center3.x + MENUWIDTH - 10, 50};
        Vector2 ccenter4 = {center4.x + MENUWIDTH - 10, 80};

        Rectangle rec1 = {toMenu(center1).x, toMenu(center1).y, MENUWIDTH - 10,
                          size1};
        Rectangle rec2 = {toMenu(center1).x, toMenu(center2).y, MENUWIDTH - 10,
                          size2};
        Rectangle rec3 = {toMenu(center1).x, toMenu(center3).y, MENUWIDTH - 10,
                          size3};
        Rectangle rec4 = {toMenu(center1).x, toMenu(center4).y, MENUWIDTH - 10,
                          size4};

        bool col1 = CheckCollisionPointRec(cursor.pos, rec1);
        bool col2 = CheckCollisionPointRec(cursor.pos, rec2);
        bool col3 = CheckCollisionPointRec(cursor.pos, rec3);
        bool col4 = CheckCollisionPointRec(cursor.pos, rec4);

        DrawLineEx(toMenu(center1), toMenu(ccenter1), size1,
                   GetColor(!col1 ? LINECOL : LINECOLTOUCHED));
        DrawLineEx(toMenu(center2), toMenu(ccenter2), size2,
                   GetColor(!col2 ? LINECOL : LINECOLTOUCHED));
        DrawLineEx(toMenu(center3), toMenu(ccenter3), size3,
                   GetColor(!col3 ? LINECOL : LINECOLTOUCHED));
        DrawLineEx(toMenu(center4), toMenu(ccenter4), size4,
                   GetColor(!col4 ? LINECOL : LINECOLTOUCHED));

        if (col1 && cursor.left) {
            size = size1;
            return;
        }

        if (col2 && cursor.left) {
            size = size2;
            return;
        }

        if (col3 && cursor.left) {
            size = size3;
            return;
        }

        if (col4 && cursor.left) {
            size = size4;
            return;
        }
    }


    if (!first && cursor.heldleft) {
        p1.x = (cursor.pos.x + canvasPosition.x) / zoom;
        p1.y = (cursor.pos.y + canvasPosition.y) / zoom;
        first = true;
    }

    Vector2 mapped = {
        (cursor.pos.x + canvasPosition.x) / zoom,
        (cursor.pos.y + canvasPosition.y) / zoom,
    };

    if (first && cursor.heldleft) {
        Vector2 dp1 = {p1.x * zoom - canvasPosition.x,
                       p1.y * zoom - canvasPosition.y};

        Vector2 dp2 = {mapped.x * zoom - canvasPosition.x,
                       mapped.y * zoom - canvasPosition.y};

        DrawLineEx(dp1, dp2, size * zoom, chosenColor);
    }

    if (first && !cursor.heldleft)
        second = true;

    if (second && first && !cursor.heldleft) {
        Vector2 mapped = {
            (cursor.pos.x + canvasPosition.x) / zoom,
            (cursor.pos.y + canvasPosition.y) / zoom,
        };
        first = false;
        second = false;
        BeginTextureMode(canvas);
        DrawLineEx(p1, mapped, size, chosenColor);
        EndTextureMode();
    }
}

void mode_delete()
{
    bool closed = 1 - menu.closed;

    int size1 = 5;
    int size2 = 10;
    int size3 = 20;
    int size4 = 30;

    static int brush = 5;

    if (!closed) {
        Vector2 center1 = {menu.width / 2, 30};
        Vector2 center2 = {menu.width / 2, center1.y + 20};
        Vector2 center3 = {menu.width / 2, center1.y + 60};
        Vector2 center4 = {menu.width / 2, center1.y + 120};

        bool col1 = CheckCollisionPointCircle(cursor.pos, toMenu(center1),
                                              size1);
        bool col2 = CheckCollisionPointCircle(cursor.pos, toMenu(center2),
                                              size2);
        bool col3 = CheckCollisionPointCircle(cursor.pos, toMenu(center3),
                                              size3);
        bool col4 = CheckCollisionPointCircle(cursor.pos, toMenu(center4),
                                              size4);
#define DELETECOLTOUCHED 0xFF00AAEE
#define DELETECOL        0xBB0088EE

        DrawCircleV(toMenu(center1), size1,
                    GetColor(col1 ? DELETECOLTOUCHED : DELETECOL));

        DrawCircleV(toMenu(center2), size2,
                    GetColor(col2 ? DELETECOLTOUCHED : DELETECOL));

        DrawCircleV(toMenu(center3), size3,
                    GetColor(col3 ? DELETECOLTOUCHED : DELETECOL));

        DrawCircleV(toMenu(center4), size4,
                    GetColor(col4 ? DELETECOLTOUCHED : DELETECOL));

        if (col1 && cursor.left) {
            brush = size1;
            return;
        }

        if (col2 && cursor.left) {
            brush = size2;
            return;
        }

        if (col3 && cursor.left) {
            brush = size3;
            return;
        }

        if (col4 && cursor.left) {
            brush = size4;
            return;
        }
    }
    if (!closed)
        if (cursor.left && collisionMenu) {
            return;
        }

    Vector2 mapped = {
        (cursor.pos.x + canvasPosition.x) * 1 / zoom,
        (cursor.pos.y + canvasPosition.y) * 1 / zoom,
    };

    Vector2 lastmapped = {
        (cursor.lastpos.x + canvasPosition.x) * 1 / zoom,
        (cursor.lastpos.y + canvasPosition.y) * 1 / zoom,
    };

    BeginTextureMode(canvas);
    if (cursor.left || cursor.heldleft) {
        for (int i = 0; i < 40; ++i) {
            DrawCircleV(Vector2Lerp(mapped, lastmapped, (float)i / 10.0f),
                        brush, WHITE);
        }
    }
    EndTextureMode();
}

void mode_bezier()
{
    static Vector2 p1, p2, p3;
    static bool    f1 = false, f2 = false, f3 = false;
    static int     size = 4;
    bool           closed = 1 - menu.closed;

    if (!closed) {
        int size1 = 2;
        int size2 = 4;
        int size3 = 8;
        int size4 = 16;
        int size5 = 32;

        Vector2 center1 = {5, 20};
        Vector2 center2 = {5, 30};
        Vector2 center3 = {5, 50};
        Vector2 center4 = {5, 80};

        Vector2 ccenter1 = {center1.x + MENUWIDTH - 10, 20};
        Vector2 ccenter2 = {center2.x + MENUWIDTH - 10, 30};
        Vector2 ccenter3 = {center3.x + MENUWIDTH - 10, 50};
        Vector2 ccenter4 = {center4.x + MENUWIDTH - 10, 80};

        Rectangle rec1 = {toMenu(center1).x, toMenu(center1).y, MENUWIDTH - 10,
                          size1};
        Rectangle rec2 = {toMenu(center1).x, toMenu(center2).y, MENUWIDTH - 10,
                          size2};
        Rectangle rec3 = {toMenu(center1).x, toMenu(center3).y, MENUWIDTH - 10,
                          size3};
        Rectangle rec4 = {toMenu(center1).x, toMenu(center4).y, MENUWIDTH - 10,
                          size4};

        bool col1 = CheckCollisionPointRec(cursor.pos, rec1);
        bool col2 = CheckCollisionPointRec(cursor.pos, rec2);
        bool col3 = CheckCollisionPointRec(cursor.pos, rec3);
        bool col4 = CheckCollisionPointRec(cursor.pos, rec4);

#define BEZIERCOL        0xFF00AAEE
#define BEZIERCOLTOUCHED 0xBB0077EE

        DrawLineEx(toMenu(center1), toMenu(ccenter1), size1,
                   GetColor(!col1 ? BEZIERCOL : BEZIERCOLTOUCHED));
        DrawLineEx(toMenu(center2), toMenu(ccenter2), size2,
                   GetColor(!col2 ? BEZIERCOL : BEZIERCOLTOUCHED));
        DrawLineEx(toMenu(center3), toMenu(ccenter3), size3,
                   GetColor(!col3 ? BEZIERCOL : BEZIERCOLTOUCHED));
        DrawLineEx(toMenu(center4), toMenu(ccenter4), size4,
                   GetColor(!col4 ? BEZIERCOL : BEZIERCOLTOUCHED));

        if (col1 && cursor.left) {
            size = size1;
            return;
        }

        if (col2 && cursor.left) {
            size = size2;
            return;
        }

        if (col3 && cursor.left) {
            size = size3;
            return;
        }

        if (col4 && cursor.left) {
            size = size4;
            return;
        }
    }

    if (cursor.left) {
        if (!f1) {
            p1 = Vector2Scale(Vector2Add(cursor.pos, canvasPosition), 1 / zoom);
            f1 = true;
            return;
        }

        if (!f2 && f1) {
            p2 = Vector2Scale(Vector2Add(cursor.pos, canvasPosition), 1 / zoom);
            f2 = true;
            return;
        }

        if (f1 && f2 && !f3) {
            p3 = Vector2Scale(Vector2Add(cursor.pos, canvasPosition), 1 / zoom);
            f3 = true;
            return;
        }
    }

    if (f1) {
        Vector2 dp1 = {p1.x * zoom - canvasPosition.x,
                       p1.y * zoom - canvasPosition.y};
        DrawCircleV(dp1, size * zoom, chosenColor);
    }

    if (f2) {
        Vector2 dp2 = {p2.x * zoom - canvasPosition.x,
                       p2.y * zoom - canvasPosition.y};
        DrawCircleV(dp2, size * zoom, chosenColor);
    }

    if (f1 && f2 && f3) {
        BeginTextureMode(canvas);
        DrawSplineSegmentBezierQuadratic(p1, p3, p2, size, chosenColor);
        f1 = false, f2 = false, f3 = false;
        EndTextureMode();
    }
}

void loadIcons()
{
    Rectangle brushSource = {
        0,
        0,
        brush.width,
        brush.height,
    };

    Rectangle brushDest = {
        0.5 * WIDTH / TOOLBARITEMS * 1,
        TOOLBARHEIGHT / 2,
        50,
        50,
    };

    Vector2 origin = {20, 20};

    DrawTexturePro(brush, brushSource, brushDest, origin, 0, WHITE);

    brushDest.x += WIDTH / TOOLBARITEMS;
    brushSource.width = rubber.width;
    brushSource.height = rubber.height;

    DrawTexturePro(rubber, brushSource, brushDest, origin, 0, WHITE);

    brushDest.x += WIDTH / TOOLBARITEMS;
    brushSource.width = line.width;
    brushSource.height = line.height;

    DrawTexturePro(line, brushSource, brushDest, origin, 0, WHITE);

    brushDest.x += WIDTH / TOOLBARITEMS;
    brushSource.width = bezier.width;
    brushSource.height = bezier.height;

    DrawTexturePro(bezier, brushSource, brushDest, origin, 0, WHITE);

    if (!menu.closed) {
        brushDest.x = menu.pos.x;
        brushDest.y = menu.pos.y;

        brushDest.width = MENUWIDTH / 2.0f;
        brushDest.height = MENUWIDTH / 2.0f;

        brushSource.width = gear.width;
        brushSource.height = gear.height;

        origin.x = -50;
        origin.y = -20;
    }
    else {
        brushDest.x = menu.pos.x;
        brushDest.y = menu.pos.y;

        brushDest.width = MENUWIDTH / 2.0f;
        brushDest.height = MENUWIDTH / 2.0f;

        brushSource.width = gear.width;
        brushSource.height = gear.height;

        origin.x = -30;
        origin.y = 0;
    }
    DrawTexturePro(gear, brushSource, brushDest, origin, 0, WHITE);
}
