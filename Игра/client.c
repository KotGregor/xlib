#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <pthread.h>

void flush_buffer_to_screen();

#define PORT 1026
#define MAZE_SIZE 10
#define TILE_SIZE 40
#define OFFSET_X 10
#define OFFSET_Y 10
#define MAX_PLAYERS 4

typedef struct
{
    int x, y;
} Position;

Display *display;
Window window;
GC gc;
Pixmap buffer_pixmap;
int sock;
char maze[MAZE_SIZE][MAZE_SIZE + 1];
Position player_pos = {1, 1};
Position enemy_pos = {5, 5};
typedef struct
{
    int id;
    Position pos;
    int alive;
} OtherPlayer;

OtherPlayer other_players[MAX_PLAYERS];
int num_other_players = 0;

int player_id = -1;
int is_alive = 1;
int is_game_over = 0;
int game_started_flag = 0;

void init_buffer()
{
    buffer_pixmap = XCreatePixmap(display, window, 500, 500, DefaultDepth(display, DefaultScreen(display)));
}

void clear_buffer()
{
    XSetForeground(display, gc, WhitePixel(display, DefaultScreen(display)));
    XFillRectangle(display, buffer_pixmap, gc, 0, 0, 500, 500);
}

// void draw_maze_to_buffer()
// {
//     clear_buffer();

//     for (int i = 0; i < MAZE_SIZE; i++)
//     {
//         for (int j = 0; j < MAZE_SIZE; j++)
//         {
//             int x = OFFSET_X + j * TILE_SIZE;
//             int y = OFFSET_Y + i * TILE_SIZE;

//             if (maze[i][j] == 'W')
//             {
//                 XSetForeground(display, gc, BlackPixel(display, DefaultScreen(display)));
//                 XFillRectangle(display, buffer_pixmap, gc, x, y, TILE_SIZE, TILE_SIZE);
//             }
//             else
//             {
//                 XSetForeground(display, gc, WhitePixel(display, DefaultScreen(display)));
//                 XFillRectangle(display, buffer_pixmap, gc, x, y, TILE_SIZE, TILE_SIZE);
//             }

//             XSetForeground(display, gc, BlackPixel(display, DefaultScreen(display)));
//             XDrawRectangle(display, buffer_pixmap, gc, x, y, TILE_SIZE, TILE_SIZE);
//         }
//     }

//     XSetForeground(display, gc, 0xFF0000);
//     XFillArc(display, buffer_pixmap, gc,
//              OFFSET_X + enemy_pos.y * TILE_SIZE,
//              OFFSET_Y + enemy_pos.x * TILE_SIZE,
//              TILE_SIZE, TILE_SIZE, 0, 360 * 64);

//     if (is_alive)
//     {
//         XSetForeground(display, gc, 0x00FF00);
//         XFillArc(display, buffer_pixmap, gc,
//                  OFFSET_X + player_pos.y * TILE_SIZE,
//                  OFFSET_Y + player_pos.x * TILE_SIZE,
//                  TILE_SIZE, TILE_SIZE, 0, 360 * 64);
//     }

//     XSetForeground(display, gc, 0x00AA00);
//     for (int i = 0; i < num_other_players; i++)
//     {
//         XFillArc(display, buffer_pixmap, gc,
//                  OFFSET_X + other_players[i].y * TILE_SIZE,
//                  OFFSET_Y + other_players[i].x * TILE_SIZE,
//                  TILE_SIZE, TILE_SIZE, 0, 360 * 64);
//     }
// }

void draw_maze_to_buffer()
{
    clear_buffer();

    for (int i = 0; i < MAZE_SIZE; i++)
    {
        for (int j = 0; j < MAZE_SIZE; j++)
        {
            int x = OFFSET_X + j * TILE_SIZE;
            int y = OFFSET_Y + i * TILE_SIZE;

            if (maze[i][j] == 'W')
            {
                XSetForeground(display, gc, BlackPixel(display, DefaultScreen(display)));
                XFillRectangle(display, buffer_pixmap, gc, x, y, TILE_SIZE, TILE_SIZE);
            }
            else
            {
                XSetForeground(display, gc, WhitePixel(display, DefaultScreen(display)));
                XFillRectangle(display, buffer_pixmap, gc, x, y, TILE_SIZE, TILE_SIZE);
            }

            XSetForeground(display, gc, BlackPixel(display, DefaultScreen(display)));
            XDrawRectangle(display, buffer_pixmap, gc, x, y, TILE_SIZE, TILE_SIZE);
        }
    }

    unsigned long player_colors[MAX_PLAYERS] = {
        0x00FF00, // Зелёный для 0
        0x0000FF, // Синий для 1
        0xFFA500, // Оранжевый для 2
        0x800080  // Фиолетовый для 3
    };

    int PLAYER_RADIUS = TILE_SIZE;
    int RADIUS_OFFSET = TILE_SIZE / 2;

    int TEXT_OFFSET_X = 8;
    int TEXT_OFFSET_Y = 5;

    XSetForeground(display, gc, 0xFF0000); // Красный - злодей
    XFillArc(display, buffer_pixmap, gc,
             OFFSET_X + enemy_pos.y * TILE_SIZE + (TILE_SIZE - PLAYER_RADIUS) / 2,
             OFFSET_Y + enemy_pos.x * TILE_SIZE + (TILE_SIZE - PLAYER_RADIUS) / 2,
             PLAYER_RADIUS, PLAYER_RADIUS, 0, 360 * 64);

    if (is_alive)
    {
        XSetForeground(display, gc, player_colors[player_id]);
        XFillArc(display, buffer_pixmap, gc,
                 OFFSET_X + player_pos.y * TILE_SIZE + (TILE_SIZE - PLAYER_RADIUS) / 2,
                 OFFSET_Y + player_pos.x * TILE_SIZE + (TILE_SIZE - PLAYER_RADIUS) / 2,
                 PLAYER_RADIUS, PLAYER_RADIUS, 0, 360 * 64);

        char id_str[4];
        sprintf(id_str, "%d", player_id);
        XSetForeground(display, gc, WhitePixel(display, DefaultScreen(display))); // Белый текст
        XDrawString(display, buffer_pixmap, gc,
                    OFFSET_X + player_pos.y * TILE_SIZE + (TILE_SIZE - PLAYER_RADIUS) / 2 + PLAYER_RADIUS / 2 - 5,
                    5 + OFFSET_Y + player_pos.x * TILE_SIZE + (TILE_SIZE - PLAYER_RADIUS) / 2 + PLAYER_RADIUS / 2 + 5,
                    id_str, strlen(id_str));
    }

    // XSetForeground(display, gc, 0x00AA00); // Серо-зелёный
    for (int i = 0; i < num_other_players; i++)
    {
        OtherPlayer p = other_players[i];

        XSetForeground(display, gc, player_colors[p.id % MAX_PLAYERS]);

        XFillArc(display, buffer_pixmap, gc,
                 OFFSET_X + p.pos.y * TILE_SIZE + (TILE_SIZE - PLAYER_RADIUS) / 2,
                 OFFSET_Y + p.pos.x * TILE_SIZE + (TILE_SIZE - PLAYER_RADIUS) / 2,
                 PLAYER_RADIUS, PLAYER_RADIUS, 0, 360 * 64);

        char id_str[4];
        sprintf(id_str, "%d", p.id);

        XSetForeground(display, gc, WhitePixel(display, DefaultScreen(display))); // Белый текст
        XDrawString(display, buffer_pixmap, gc,
                    OFFSET_X + p.pos.y * TILE_SIZE + (TILE_SIZE - PLAYER_RADIUS) / 2 + PLAYER_RADIUS / 2 - 5,
                    5 + OFFSET_Y + p.pos.x * TILE_SIZE + (TILE_SIZE - PLAYER_RADIUS) / 2 + PLAYER_RADIUS / 2 + 5,
                    id_str, strlen(id_str));
    }

    flush_buffer_to_screen();
}

void flush_buffer_to_screen()
{
    XCopyArea(display, buffer_pixmap, window, gc, 0, 0, 500, 500, 0, 0);
    XFlush(display);
}

void parse_server_message(char *message)
{
    // printf("%s", message);
    static int initialized = 0;
    if (strncmp(message, "CMD:ID:", 7) == 0)
    {
        sscanf(message + 7, "%d", &player_id);
        printf("Assigned player ID: %d\n", player_id);
        initialized = 1;
        return;
    }

    if (strncmp(message, "CMD:WAITING:", 12) == 0)
    {
        int active_players, room_players;
        sscanf(message + 12, "%d %d", &active_players, &room_players);
        printf("SERVER: Ожидание игроков...%d/%d\n", active_players, room_players);
        return;
    }

    if (strncmp(message, "CMD:GAME_OVER", 13) == 0)
    {

        is_game_over = 1;

        printf("SERVER: Игра окончена. Все игроки погибли.\n");
        return;
    }

    if (strcmp(message, "CMD:GAME_FULL") == 0)
    {
        printf("места нет\n");
        return;
    }

    if (strcmp(message, "CMD:GAME_START") == 0)
    {
        printf("игра началась\n");
        game_started_flag = 1;
    }

    if (strncmp(message, "CMD:OFF:", 8) == 0)
    {
        int p_id;
        sscanf(message + 8, "%d", &p_id);
        if (p_id == player_id)
        {
            is_alive = 0;
            printf("вы умерли\n");
        }
        else
        {
            for (int i = 0; i < num_other_players; i++)
            {
                if (other_players[i].id == p_id)
                {
                    other_players[i].alive = 0;
                }
            }
            printf("игрок %d умер\n", p_id);
        }
        return;
    }

    num_other_players = 0;
    if (strncmp(message, "STATE:", 6) == 0)
    {
        char *data = message + 6;

        char *lines[100];
        int line_count = 0;
        char *line = strtok(data, "\n");

        num_other_players = 0;

        while (line != NULL && line_count < 100)
        {
            lines[line_count++] = line;
            line = strtok(NULL, "\n");
        }

        for (int i = 0; i < MAZE_SIZE && i < line_count; i++)
        {
            strncpy(maze[i], lines[i], MAZE_SIZE);
            maze[i][MAZE_SIZE] = '\0';
        }

        enemy_pos.x = 5;
        enemy_pos.y = 5;

        for (int i = 0; i < line_count; i++)
        {
            if (lines[i][0] == 'E')
            {
                sscanf(lines[i] + 2, "%d,%d", &enemy_pos.x, &enemy_pos.y);
            }
            else if (lines[i][0] == 'P')
            {
                int id;
                Position pos;
                int alive;
                if (sscanf(lines[i] + 1, "%d:%d,%d:%d", &id, &pos.x, &pos.y, &alive) == 4)
                {
                    if (id == player_id)
                    {
                        is_alive = alive;
                        if (alive)
                            player_pos = pos;
                    }
                    else
                    {
                        if (alive && num_other_players < MAX_PLAYERS)
                            other_players[num_other_players++] = (OtherPlayer){.id = id, .pos = pos, .alive = alive};
                    }
                }
            }
        }

        draw_maze_to_buffer();
        flush_buffer_to_screen();
    }
}

void *receive_thread(void *arg)
{
    char buffer[4096];
    static char message_buffer[65536] = {0};
    int bytes_read;
    // сохранять между вызовами
    while ((bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0)
    {
        buffer[bytes_read] = '\0';
        strncat(message_buffer, buffer, sizeof(message_buffer) - strlen(message_buffer) - 1);

        char *end;
        while ((end = strstr(message_buffer, "\n\n")) != NULL)
        {
            end[0] = '\0';
            parse_server_message(message_buffer);
            memmove(message_buffer, end + 2, strlen(end + 2) + 1);
        }
    }

    // printf("Disconnected from server\n");
    close(sock);
    is_game_over = 1;
    return NULL;
}

void send_move(int dx, int dy)
{
    if (!game_started_flag)
        return;
    char buffer[32];
    sprintf(buffer, "MOVE:%d,%d", dx, dy);
    send(sock, buffer, strlen(buffer), 0);
}

int main(int argc, char const *argv[])
{
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, /* "127.0.0.1" */"192.168.56.1", &serv_addr.sin_addr) <= 0)
    {
        printf("\nInvalid address / Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }

    display = XOpenDisplay(NULL);
    if (!display)
    {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }

    int screen = DefaultScreen(display);
    window = XCreateSimpleWindow(display, RootWindow(display, screen),
                                 10, 10, 500, 500, 1,
                                 BlackPixel(display, screen),
                                 WhitePixel(display, screen));

    XSelectInput(display, window, ExposureMask | KeyPressMask);
    XMapWindow(display, window);
    gc = XCreateGC(display, window, 0, NULL);
    XFontStruct *font = XLoadQueryFont(display, "*-helvetica-bold-r-*-*-24-*-*-*-*-*-*-*");
    if (!font)
    {
        printf("Font not found, using default\n");
        font = XLoadQueryFont(display, "fixed");
    }

    XSetFont(display, gc, font->fid);
    init_buffer();

    pthread_t tid;
    pthread_create(&tid, NULL, receive_thread, NULL);

    XEvent event;
    while (1)
    {

        if (is_game_over)
        {

            clear_buffer();

            XSetForeground(display, gc, WhitePixel(display, DefaultScreen(display)));
            XFillRectangle(display, buffer_pixmap, gc, 0, 0, 500, 500);

            XSetForeground(display, gc, BlackPixel(display, DefaultScreen(display)));

            int text_x = 70;
            int text_y = 250;
            XDrawString(display, buffer_pixmap, gc, text_x, text_y,
                        "Игра окончена. Закрытие...", strlen("Игра окончена. Закрытие..."));
            flush_buffer_to_screen();

            sleep(3);

            XFreePixmap(display, buffer_pixmap);
            XCloseDisplay(display);
            close(sock);
            exit(0);
        }

        if(XPending(display))
        {
            XNextEvent(display, &event);

        switch (event.type)
        {
        case Expose:
            draw_maze_to_buffer();
            flush_buffer_to_screen();
            break;

        case KeyPress:
        {
            KeySym key = XLookupKeysym(&event.xkey, 0);
            int dx = 0, dy = 0;

            switch (key)
            {
            case XK_Up:
                dx = -1;
                break;
            case XK_Down:
                dx = 1;
                break;
            case XK_Left:
                dy = -1;
                break;
            case XK_Right:
                dy = 1;
                break;
            case XK_Escape:
                close(sock);
                XFreePixmap(display, buffer_pixmap);
                XCloseDisplay(display);
                return 0;
            }

            if (dx != 0 || dy != 0)
            {
                send_move(dx, dy);
            }
            break;
        }
    }
        }
        else
        {
            usleep(100000); // Не забиваем процессор
        }
    }

    XFreePixmap(display, buffer_pixmap);
    XCloseDisplay(display);
    close(sock);
    return 0;
}

// хочу подключиться:
// не набралось достоточно игроков: белоко окно с надписью: ожадайте, кол-во игроков из кол-ва нужного
// набралось игроков: начинаем играть - показывает лабиринт
// не набраось - таймер на две минуты, если не набраось за это время - окно с надписью: игра отменена, либо сами закрываем, либо через две минуты само закрывается
// если все набрались, то больше никто не может - окно с надписью - вас слишком много
//
// я умираю - окно с надписью, что я умер, смотрите на остальных учатсников
// закрыть - смотреть, не закрыть, все равно двигается, после окончания игры само загрывается
// я умираю, или кто-то умирает последним - окно с надписью, что игра окончена, и кто победил( по логике, наверное, тот, кого последним убили)
// я выхожу из игры - никто больше не может подключиться, ни я, ни кто-то другой
// все выходят из игры - игра прекращается, сервер открыт, мир заново инициализируется и ждет новых игроков в течение двух минут
// я хочу подключиться, но мест нет, уже подключилось нужное количество участников - окно с надписью, что мест нет