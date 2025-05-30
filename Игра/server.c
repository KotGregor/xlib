#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include <arpa/inet.h>
#include <limits.h>

#define PORT 1026
#define MAX_PLAYERS 4
#define MAZE_SIZE 10
#define ROOM_PLAYERS 1

typedef struct
{
    int x, y;
} Position;

typedef struct
{
    int socket;
    Position pos;
    int active;
    int alive;
} Player;

char maze[MAZE_SIZE][MAZE_SIZE + 1] = {
    "WWWWWWWWWW",
    "W        W",
    "W        W",
    "W        W",
    "W        W",
    "W        W",
    "W        W",
    "W        W",
    "W        W",
    "WWWWWWWWWW"};

Position enemy_pos = {5, 5};
Player players[MAX_PLAYERS];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int game_started = 0;

int count_alive_players()
{
    int alive = 0;
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (players[i].active && players[i].alive)
        {
            alive++;
        }
    }
    return alive;
}

void init_maze()
{
    for (int i = 0; i < MAZE_SIZE; i++)
    {
        for (int j = 0; j < MAZE_SIZE; j++)
        {
            if (i == 0 || i == MAZE_SIZE - 1 || j == 0 || j == MAZE_SIZE - 1)
            {
                maze[i][j] = 'W';
            }
            else
            {
                maze[i][j] = ' ';
            }
        }
    }
}

void send_maze(int sock)
{
    char buffer[4096];
    char *ptr = buffer;

    sprintf(ptr, "STATE:");
    ptr += 6;

    for (int i = 0; i < MAZE_SIZE; i++)
    {
        memcpy(ptr, maze[i], MAZE_SIZE);
        ptr += MAZE_SIZE;
        *ptr++ = '\n';
    }

    sprintf(ptr, "E:%d,%d\n", enemy_pos.x, enemy_pos.y);

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (players[i].active)
        {
            sprintf(ptr + strlen(ptr), "P%d:%d,%d:%d\n", i, players[i].pos.x, players[i].pos.y, players[i].alive);
        }
    }

    strcat(ptr, "\n");
    send(sock, buffer, strlen(buffer), 0);
}

int find_free_player_slot()
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (!players[i].active)
            return i;
    }
    return -1;
}

void move_enemy()
{
    int directions[4][2] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
    int valid_dirs[4];
    int valid_count = 0;

    int old_x = enemy_pos.x;
    int old_y = enemy_pos.y;

    for (int i = 0; i < 4; i++)
    {
        int new_x = old_x + directions[i][0];
        int new_y = old_y + directions[i][1];

        if (new_x >= 0 && new_x < MAZE_SIZE && new_y >= 0 && new_y < MAZE_SIZE)
        {
            valid_dirs[valid_count++] = i;
        }
    }

    if (valid_count > 0)
    {
        int chosen_dir = valid_dirs[rand() % valid_count];
        int new_x = old_x + directions[chosen_dir][0];
        int new_y = old_y + directions[chosen_dir][1];

        enemy_pos.x = new_x;
        enemy_pos.y = new_y;

        if (maze[old_x][old_y] == 'W')
        {
            maze[old_x][old_y] = ' ';
        }
        else
        {
            maze[old_x][old_y] = 'W';
        }

        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            if (players[i].active && players[i].alive &&
                players[i].pos.x == new_x && players[i].pos.y == new_y)
            {
                players[i].alive = 0;

                char off_msg[32];
                sprintf(off_msg, "CMD:OFF:%d\n\n", i);

                for (int j = 0; j < MAX_PLAYERS; j++)
                {
                    if (players[j].active)
                        send(players[j].socket, off_msg, strlen(off_msg), 0);
                }
            }
        }

        if (count_alive_players() == 0 && game_started)
        {
            printf("All players dead. Game over.\n");
            game_started = 0;
            init_maze();
            enemy_pos.x = 5;
            enemy_pos.y = 5;

            const char *game_over = "CMD:GAME_OVER\n\n";
            for (int i = 0; i < MAX_PLAYERS; i++)
            {
                if (players[i].active)
                    send(players[i].socket, game_over, strlen(game_over), 0);
            }
        }
    }
}

// void move_enemy()
// {
//     int directions[4][2] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
//     int best_dir = -1;
//     int min_distance = 10^3;

//     int old_x = enemy_pos.x;
//     int old_y = enemy_pos.y;

//     Position nearest_player;
//     int player_found = 0;

//     for (int i = 0; i < MAX_PLAYERS; i++) {
//         if (players[i].active && players[i].alive) {
//             int dx = players[i].pos.x - old_x;
//             int dy = players[i].pos.y - old_y;
//             int dist = dx * dx + dy * dy;

//             if (!player_found || dist < min_distance) {
//                 min_distance = dist;
//                 nearest_player = players[i].pos;
//                 player_found = 1;
//             }
//         }
//     }

//     if (!player_found) {
//         return;
//     }

//     for (int i = 0; i < 4; i++) {
//         int new_x = old_x + directions[i][0];
//         int new_y = old_y + directions[i][1];

//         if (new_x >= 0 && new_x < MAZE_SIZE && new_y >= 0 && new_y < MAZE_SIZE) {
//             int new_dist = (new_x - nearest_player.x) * (new_x - nearest_player.x) +
//                            (new_y - nearest_player.y) * (new_y - nearest_player.y);

//             if (new_dist < min_distance) {
//                 min_distance = new_dist;
//                 best_dir = i;
//             }
//         }
//     }

//     if (best_dir != -1) {
//         int new_x = old_x + directions[best_dir][0];
//         int new_y = old_y + directions[best_dir][1];

//         enemy_pos.x = new_x;
//         enemy_pos.y = new_y;

//         if (maze[old_x][old_y] == 'W') {
//             maze[old_x][old_y] = ' ';
//         } else {
//             maze[old_x][old_y] = 'W';
//         }

//         for (int i = 0; i < MAX_PLAYERS; i++) {
//             if (players[i].active && players[i].alive &&
//                 players[i].pos.x == new_x && players[i].pos.y == new_y) {
//                 players[i].alive = 0;

//                 char off_msg[32];
//                 sprintf(off_msg, "CMD:OFF:%d\n\n", i);

//                 for (int j = 0; j < MAX_PLAYERS; j++) {
//                     if (players[j].active)
//                         send(players[j].socket, off_msg, strlen(off_msg), 0);
//                 }
//             }
//         }

//         if (count_alive_players() == 0 && game_started) {
//             printf("All players dead. Game over.\n");
//             game_started = 0;
//             init_maze();
//             enemy_pos.x = 5;
//             enemy_pos.y = 5;

//             const char *game_over = "CMD:GAME_OVER\n\n";
//             for (int i = 0; i < MAX_PLAYERS; i++) {
//                 if (players[i].active)
//                     send(players[i].socket, game_over, strlen(game_over), 0);
//             }
//         }
//     }
// }

// void move_enemy()
// {
//     int directions[4][2] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
//     int best_dir = -1;
//     int old_x = enemy_pos.x;
//     int old_y = enemy_pos.y;

//     const int zone_size = 3;
//     const int zones_x = (MAZE_SIZE + zone_size - 1) / zone_size;
//     const int zones_y = (MAZE_SIZE + zone_size - 1) / zone_size;
//     int player_count[zones_x][zones_y];

//     for (int i = 0; i < zones_x; i++)
//         for (int j = 0; j < zones_y; j++)
//             player_count[i][j] = 0;

//     for (int i = 0; i < MAX_PLAYERS; i++) {
//         if (players[i].active && players[i].alive) {
//             int zone_x = players[i].pos.y / zone_size; // по X (горизонталь)
//             int zone_y = players[i].pos.x / zone_size; // по Y (вертикаль)

//             if (zone_x >= 0 && zone_x < zones_x && zone_y >= 0 && zone_y < zones_y)
//                 player_count[zone_x][zone_y]++;
//         }
//     }

//     int max_players = 0;
//     int target_zone_x = 0;
//     int target_zone_y = 0;

//     for (int x = 0; x < zones_x; x++) {
//         for (int y = 0; y < zones_y; y++) {
//             if (player_count[x][y] > max_players) {
//                 max_players = player_count[x][y];
//                 target_zone_x = x;
//                 target_zone_y = y;
//             }
//         }
//     }

//     if (max_players == 0) return;

//     int center_x = target_zone_y * zone_size + zone_size / 2;
//     int center_y = target_zone_x * zone_size + zone_size / 2;

//     best_dir = -1;
//     int min_dist = INT_MAX;

//     for (int i = 0; i < 4; i++) {
//         int new_x = old_x + directions[i][0];
//         int new_y = old_y + directions[i][1];

//         if (new_x >= 0 && new_x < MAZE_SIZE && new_y >= 0 && new_y < MAZE_SIZE) {
//             int dx = new_x - center_x;
//             int dy = new_y - center_y;
//             int dist = dx * dx + dy * dy;

//             if (dist < min_dist) {
//                 min_dist = dist;
//                 best_dir = i;
//             }
//         }
//     }

//     if (best_dir != -1) {
//         int new_x = old_x + directions[best_dir][0];
//         int new_y = old_y + directions[best_dir][1];

//         enemy_pos.x = new_x;
//         enemy_pos.y = new_y;

//         if (maze[old_x][old_y] == 'W') {
//             maze[old_x][old_y] = ' ';
//         } else {
//             maze[old_x][old_y] = 'W';
//         }

//         for (int i = 0; i < MAX_PLAYERS; i++) {
//             if (players[i].active && players[i].alive &&
//                 players[i].pos.x == new_x && players[i].pos.y == new_y) {
//                 players[i].alive = 0;

//                 char off_msg[32];
//                 sprintf(off_msg, "CMD:OFF:%d\n\n", i);

//                 for (int j = 0; j < MAX_PLAYERS; j++) {
//                     if (players[j].active)
//                         send(players[j].socket, off_msg, strlen(off_msg), 0);
//                 }
//             }
//         }

//         if (count_alive_players() == 0 && game_started) {
//             printf("All players dead. Game over.\n");
//             game_started = 0;
//             init_maze();
//             enemy_pos.x = 5;
//             enemy_pos.y = 5;

//             const char *game_over = "CMD:GAME_OVER\n\n";
//             for (int i = 0; i < MAX_PLAYERS; i++) {
//                 if (players[i].active)
//                     send(players[i].socket, game_over, strlen(game_over), 0);
//             }
//         }
//     }
// }

void *enemy_thread(void *arg)
{
    while (1)
    {
        sleep(1);

        pthread_mutex_lock(&mutex);
        if (!game_started)
        {
            pthread_mutex_unlock(&mutex);
            continue;
        }

        move_enemy();

        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            if (players[i].active)
                send_maze(players[i].socket);
        }

        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

void *client_handler(void *arg)
{
    int sock = *(int *)arg;
    int player_id = -1;
    pthread_mutex_lock(&mutex);

    player_id = find_free_player_slot();

    if (player_id == -1)
    {
        const char *msg = "CMD:GAME_FULL\n\n";
        send(sock, msg, strlen(msg), 0);
        close(sock);
        pthread_mutex_unlock(&mutex);
        return NULL;
    }

    players[player_id].socket = sock;
    players[player_id].active = 1;
    players[player_id].alive = 1;
    players[player_id].pos.x = 1;
    players[player_id].pos.y = 1;

    char id_msg[32];
    sprintf(id_msg, "CMD:ID:%d\n\n", player_id);
    send(sock, id_msg, strlen(id_msg), 0);

    int active_players = 0;
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (players[i].active)
            active_players++;
    }

    if (active_players >= ROOM_PLAYERS && !game_started)
    {
        game_started = 1;
        printf("Game started!\n");
        const char *game_start = "CMD:GAME_START\n\n";

        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            if (players[i].active)
                send(players[i].socket, game_start, strlen(game_start), 0);
        }
    }

    if (!game_started)
    {
        char wait_msg[32];
        sprintf(wait_msg, "CMD:WAITING:%d %d\n\n", active_players, ROOM_PLAYERS);
        send(sock, wait_msg, strlen(wait_msg), 0);
    }

    send_maze(sock);

    pthread_mutex_unlock(&mutex);

    char buffer[1024];
    int bytes_read;

    while ((bytes_read = recv(sock, buffer, sizeof(buffer), 0)) > 0)
    {
        buffer[bytes_read] = '\0';

        pthread_mutex_lock(&mutex);
        if (!game_started)
        {
            pthread_mutex_unlock(&mutex);
            continue;
        }

        if (strncmp(buffer, "MOVE:", 5) == 0)
        {
            int dx = 0, dy = 0;
            sscanf(buffer + 5, "%d,%d", &dx, &dy);

            Position new_pos = {
                players[player_id].pos.x + dx,
                players[player_id].pos.y + dy};

            int cell_occupied = 0;
            for (int i = 0; i < MAX_PLAYERS; i++)
            {
                if (i != player_id && players[i].active && players[i].alive &&
                    players[i].pos.x == new_pos.x && players[i].pos.y == new_pos.y)
                {
                    cell_occupied = 1;
                    break;
                }
            }

            if (!cell_occupied &&
                new_pos.x >= 0 && new_pos.x < MAZE_SIZE &&
                new_pos.y >= 0 && new_pos.y < MAZE_SIZE &&
                maze[new_pos.x][new_pos.y] != 'W')
            {
                players[player_id].pos = new_pos;

                if (new_pos.x == enemy_pos.x && new_pos.y == enemy_pos.y)
                {
                    players[player_id].alive = 0;

                    char off_msg[32];
                    sprintf(off_msg, "CMD:OFF:%d\n\n", player_id);

                    for (int j = 0; j < MAX_PLAYERS; j++)
                    {
                        if (players[j].active)
                            send(players[j].socket, off_msg, strlen(off_msg), 0);
                    }

                    if (count_alive_players() == 0 && game_started)
                    {
                        const char *game_over = "CMD:GAME_OVER\n\n";
                        for (int i = 0; i < MAX_PLAYERS; i++)
                        {
                            if (players[i].active)
                                send(players[i].socket, game_over, strlen(game_over), 0);
                        }

                        game_started = 0;
                        init_maze();
                        enemy_pos.x = 5;
                        enemy_pos.y = 5;
                    }
                }
            }
        }

        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            if (players[i].active)
            {
                send_maze(players[i].socket);
            }
        }

        pthread_mutex_unlock(&mutex);
    }

    pthread_mutex_lock(&mutex);
    players[player_id].active = 0;

    int remaining_players = 0;
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (players[i].active)
            remaining_players++;
    }

    if (remaining_players == 0)
    {
        printf("All players disconnected. Resetting game.\n");
        game_started = 0;
        enemy_pos.x = 5;
        enemy_pos.y = 5;
        init_maze();
    }

    pthread_mutex_unlock(&mutex);
    close(sock);
    return NULL;
}

int main()
{
    srand(time(NULL));
    init_maze();

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server started on port %d\n", PORT);

    pthread_t enemy_tid;
    pthread_create(&enemy_tid, NULL, enemy_thread, NULL);

    while (1)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            continue;
        }

        printf("New connection from %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
        pthread_t tid;
        int *client_sock = malloc(sizeof(int));
        *client_sock = new_socket;

        pthread_create(&tid, NULL, client_handler, (void *)client_sock);
    }

    return 0;
}
