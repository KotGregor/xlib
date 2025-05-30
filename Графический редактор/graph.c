#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

// Структура для хранения линии
typedef struct Line {
    int x1, y1;             // Начало линии
    int x2, y2;             // Конец линии
    unsigned long color;    // Цвет линии
    struct Line *next;      // Указатель на следующую линию
} Line;

// Глобальные переменные
Line *head = NULL;         // Начало связного списка
unsigned long current_color = 0x0000FF; // Текущий цвет (по умолчанию голубой)
int is_drawing = 0;         // Флаг рисования
int last_x = -1, last_y = -1; // Последняя точка

// Добавление линии в связный список
void add_line(int x1, int y1, int x2, int y2, unsigned long color) {
    Line *new_line = malloc(sizeof(Line));
    new_line->x1 = x1;
    new_line->y1 = y1;
    new_line->x2 = x2;
    new_line->y2 = y2;
    new_line->color = color;
    new_line->next = head;
    head = new_line;
}

// Отрисовка всех линий из связного списка
void draw_lines(Display *display, Window window, GC gc) {
    Line *current = head;
    while (current != NULL) {
        XSetForeground(display, gc, current->color);
        XDrawLine(display, window, gc, current->x1, current->y1, current->x2, current->y2);
        current = current->next;
    }
}

// Сохранение линий в файл
void save_to_file(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Cannot open file for writing\n");
        return;
    }

    Line *current = head;
    while (current != NULL) {
        fprintf(file, "%d %d %d %d %lu\n", current->x1, current->y1, current->x2, current->y2, current->color);
        current = current->next;
    }

    fclose(file);
    printf("Lines saved to %s\n", filename);
}

// Загрузка линий из файла
void load_from_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Cannot open file for reading\n");
        return;
    }

    int x1, y1, x2, y2;
    unsigned long color;
    while (fscanf(file, "%d %d %d %d %lu", &x1, &y1, &x2, &y2, &color) == 5) {
        add_line(x1, y1, x2, y2, color);
    }

    fclose(file);
    printf("Lines loaded from %s\n", filename);
}

int main() {
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }

    int screen = DefaultScreen(display);

    // Создание главного окна
    Window window = XCreateSimpleWindow(
        display, RootWindow(display, screen),
        100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, 1,
        BlackPixel(display, screen), WhitePixel(display, screen)
    );

    XSelectInput(display, window,
                 ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | KeyPressMask);

    XMapWindow(display, window);

    GC gc = XCreateGC(display, window, 0, NULL);

    // Кнопки для выбора цвета
    Window color_buttons[4];
    unsigned long button_colors[4] = {0x0000FF, 0x800080, 0xFFFF00, 0x000000};
    const char *button_texts[4] = {"Blue", "Purple", "Yellow", "Black"};

    for (int i = 0; i < 4; i++) {
        color_buttons[i] = XCreateSimpleWindow(
            display, window,
            10 + i * 70, 10, 60, 30, 1,
            BlackPixel(display, screen), button_colors[i]
        );
        XSelectInput(display, color_buttons[i], ButtonPressMask);
        XMapWindow(display, color_buttons[i]);

        // Отрисовка текста на кнопках
        XSetForeground(display, gc, WhitePixel(display, screen));
        XDrawString(display, color_buttons[i], gc, 10, 20, button_texts[i], strlen(button_texts[i]));
    }

    // Главный цикл событий
    while (1) {
        XEvent event;
        XNextEvent(display, &event);

        switch (event.type) {
            case Expose:
                // Перерисовка всех линий
                draw_lines(display, window, gc);
                break;

            case ButtonPress:
                if (event.xbutton.window == window) {
                    // Начало рисования
                    is_drawing = 1;
                    last_x = event.xbutton.x;
                    last_y = event.xbutton.y;
                } else {
                    // Проверка нажатия на кнопки
                    for (int i = 0; i < 4; i++) {
                        if (event.xbutton.window == color_buttons[i]) {
                            current_color = button_colors[i];
                            printf("Selected color: %s\n", button_texts[i]);
                        }
                    }
                }
                break;

            case MotionNotify:
                if (is_drawing && last_x != -1 && last_y != -1) {
                    // Рисование линии между последней точкой и текущей
                    int x = event.xmotion.x;
                    int y = event.xmotion.y;
                    add_line(last_x, last_y, x, y, current_color);
                    XSetForeground(display, gc, current_color);
                    XDrawLine(display, window, gc, last_x, last_y, x, y);
                    last_x = x;
                    last_y = y;
                }
                break;

            case ButtonRelease:
                // Конец рисования
                is_drawing = 0;
                last_x = -1;
                last_y = -1;
                break;

            case KeyPress:
                // Сохранение в файл при нажатии клавиши 's'
                if (XLookupKeysym(&event.xkey, 0) == XK_s) {
                    save_to_file("drawing.txt");
                }
                // Загрузка из файла при нажатии клавиши 'l'
                if (XLookupKeysym(&event.xkey, 0) == XK_l) {
                    load_from_file("drawing.txt");
                    draw_lines(display, window, gc);
                }
                break;
        }
    }

    // Освобождение памяти
    Line *current = head;
    while (current != NULL) {
        Line *temp = current;
        current = current->next;
        free(temp);
    }

    XFreeGC(display, gc);
    XDestroyWindow(display, window);
    XCloseDisplay(display);

    return 0;
}