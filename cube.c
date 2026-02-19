#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#define ESC "\x1B"

#define FPS 30.0f

#define TERM_COLS 150
#define TERM_ROWS 80

#define VERTICAL_BORDER   "║"
#define HORIZONTAL_BORDER "═"
#define TOP_LEFT_BORDER   "╔"
#define TOP_RIGHT_BORDER  "╗"
#define BOT_LEFT_BORDER   "╚"
#define BOT_RIGHT_BORDER  "╝"

#define CIRCLE "⚪"
#define SQUARE "█" 
#define LEFT   -100.0f
#define RIGHT   100.0f
#define TOP    -100.0f
#define BOT     100.0f

static bool RUN = true;

typedef struct {
    float x, y, z;
} Point3;

typedef struct {
    float x, y;
} Point2;

typedef Point3 Cube[8];

#define CUBE_SIZE  50.0f

#define CUBE_LEFT  -CUBE_SIZE/2.0f
#define CUBE_TOP   -CUBE_SIZE/2.0f
#define CUBE_FRONT -CUBE_SIZE/2.0f

#define CUBE_RIGHT  CUBE_LEFT  + CUBE_SIZE
#define CUBE_BOTTOM CUBE_TOP   + CUBE_SIZE 
#define CUBE_BACK   CUBE_FRONT + CUBE_SIZE

static Cube THE_CUBE = {
    //TOP
    {CUBE_LEFT,  CUBE_TOP, CUBE_FRONT},    {CUBE_RIGHT, CUBE_TOP, CUBE_FRONT },
    {CUBE_RIGHT, CUBE_TOP, CUBE_BACK},     {CUBE_LEFT,  CUBE_TOP, CUBE_BACK  },
    //BOTTOM
    {CUBE_LEFT,  CUBE_BOTTOM, CUBE_FRONT}, {CUBE_RIGHT, CUBE_BOTTOM, CUBE_FRONT },
    {CUBE_RIGHT, CUBE_BOTTOM, CUBE_BACK},  {CUBE_LEFT,  CUBE_BOTTOM, CUBE_BACK  },
};

Point2 project(Point3 p)
{
    Point2 pp = (Point2) {
        .x = p.x,
        .y = p.y,
    };
    return pp;
}

void handle_sigint(int _)
{
    (void)_;
    RUN = false;
}

Point2 point_from_ints(int x, int y)
{
    return (Point2) {
        .x = (float)x,
        .y = (float)y,
    };
}

void term_clear()
{
    printf(ESC"[2J");                  // erase entire screen
    printf(ESC"[H");                   // move cursor to Home = (0,0)
}

void term_init()
{
    signal(SIGINT, handle_sigint);
    printf(ESC"[?47h");                // save screen
    printf(ESC"[?25l");                // make cursor invisible
    printf(ESC"[?1049h");              // enables the alternative buffer
}

void term_cleanup() 
{
    printf(ESC"[?25h");                // make cursor visible
    printf(ESC"[?1049l");              // disables the alternative buffer
    printf(ESC"[?47l");                // restore screen
}

void term_move(int col, int row)
{
    printf(ESC "[%d;%df", row, col);   // move cursor to (row, col)
}

void term_moveto(Point2 p)
{
    // map LEFT<->RIGHT to 0<->TERM_COLS
    int col = ((p.x + (LEFT - RIGHT) / 2.0f) / (LEFT - RIGHT)) * TERM_COLS;

    // map TOP<->BOT to 0<->TERM_ROWS
    int row = ((p.y + (TOP - BOT)    / 2.0f) / (TOP - BOT))    * TERM_ROWS;
    term_move(col, row);
}

void term_borders()
{
    // Top and Bottom Borders
    for (int i = 1; i < TERM_COLS - 1; ++i) {
         term_move(i, 0);
         printf(HORIZONTAL_BORDER);
         term_move(i, TERM_ROWS - 1);
         printf(HORIZONTAL_BORDER);
    }
    // Left and Right Borders
    for (int j = 1; j < TERM_ROWS - 1; ++j) {
         term_move(0, j);
         printf(VERTICAL_BORDER);
         term_move(TERM_COLS - 1, j);
         printf(VERTICAL_BORDER);
    }
    // Corners
    term_move(0, 0);
    printf(TOP_LEFT_BORDER);
    term_move(TERM_COLS-1, 0);
    printf(TOP_RIGHT_BORDER);
    term_move(0, TERM_ROWS-1);
    printf(BOT_LEFT_BORDER);
    term_move(TERM_COLS-1, TERM_ROWS-1);
    printf(BOT_RIGHT_BORDER);
}

void point_print(Point2 p)
{
    term_moveto(p);
    printf(SQUARE);
}

void line_print(Point3 a, Point3 b) 
{
    Point2 pa = project(a);
    Point2 pb = project(b);

    if ((int)pa.x != (int)pb.x) {
        Point2 start = pa.x < pb.x ? pa : pb;
        Point2 end   = pa.x < pb.x ? pb : pa;
        float dy = end.y - start.y;
        float dx = end.x - start.x;
        float m  = dy / dx;
        for (int i = 0; start.x + i <= end.x; ++i) {
            Point2 p = (Point2) {
                .x = start.x + i,
                .y = start.y + m * i,
            };
            point_print(p);
        }
    } else {
        float start = pa.y < pb.y ? pa.y : pb.y;
        float end   = pa.y < pb.y ? pb.y : pa.y;
        for (int j = 0; start + j <= end; ++j) {
            Point2 p = (Point2) {
                .x = pa.x,
                .y = start + j,
            };
            point_print(p);
        }
    }
}

void cube_print(Cube c)
{
    for (int i = 0; i < 4; ++i) {
        line_print(c[i]    , c[(i + 1) % 4]);
        line_print(c[i + 4], c[(i + 1) % 4 + 4]);
        line_print(c[i]    , c[i + 4]);
    }
}


void cube_rotate_x(Cube cube, float angle)
{
    float s = sinf(angle);
    float c = cosf(angle);
    for (int i = 0; i < 8; ++i) {
        float old_x = cube[i].x;
        float old_y = cube[i].y;
        float old_z = cube[i].z;
        cube[i] = (Point3) {
            .x = old_x,
            .y =  c * old_y + -s * old_z, 
            .z =  s * old_y +  c * old_z, 
        };
    }
}

void cube_rotate_y(Cube cube, float angle)
{
    float s = sinf(angle);
    float c = cosf(angle);
    for (int i = 0; i < 8; ++i) {
        float old_x = cube[i].x;
        float old_y = cube[i].y;
        float old_z = cube[i].z;
        cube[i] = (Point3) {
            .x = c * old_x  + s * old_z, 
            .y = old_y,
            .z = -s * old_x + c * old_z, 
        };
    }
}

void cube_rotate_z(Cube cube, float angle)
{
    float s = sinf(angle);
    float c = cosf(angle);
    for (int i = 0; i < 8; ++i) {
        float old_x = cube[i].x;
        float old_y = cube[i].y;
        float old_z = cube[i].z;
        cube[i] = (Point3) {
            .x = c * old_x  + -s * old_y, 
            .y = s * old_x  +  c * old_y,
            .z = old_z, 
        };
    }
}
void axis()
{
    Point3 y_axis_top = (Point3){ 
        .x = 0,
        .y = TOP,
        .z = 0,
    };
    Point3 y_axis_bot = (Point3){ 
        .x = 0,
        .y = BOT,
        .z = 0,
    };
    line_print(y_axis_top, y_axis_bot);

    Point3 x_axis_top = (Point3){ 
        .x = LEFT,
        .y = 0,
        .z = 10,
    };
    Point3 x_axis_bot = (Point3){ 
        .x = RIGHT,
        .y = 0,
        .z = 10,
    };
    line_print(x_axis_top, x_axis_bot);

}

int main(void)
{
    term_init();

    float theta = M_PI / (float)100;
    while (RUN) {
        term_clear();
        term_borders();

        cube_rotate_x(THE_CUBE, theta);
        cube_rotate_y(THE_CUBE, theta);
        cube_rotate_z(THE_CUBE, theta);
        cube_print(THE_CUBE);

        fflush(stdout);
        usleep(1000000 / FPS);
    }
    term_cleanup();
    return 0;
}
