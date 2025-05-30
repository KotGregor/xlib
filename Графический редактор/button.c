#include "button.h"
#include <string.h>

void createButton(Display *display, Window parent, Button *button) {
    int screen = DefaultScreen(display);

    button->window = XCreateSimpleWindow(
        display, parent,
        button->x, button->y, button->width, button->height, 1,
        BlackPixel(display, screen), button->bg_color
    );

    XSizeHints size_hints;
    size_hints.flags = PPosition | PSize | PMinSize;
    size_hints.min_width = button->width;
    size_hints.min_height = button->height;

    XSetWMNormalHints(display, button->window, &size_hints);

    // Разрешаем события для кнопки
    XSelectInput(display, button->window,
                 ExposureMask | ButtonPressMask | ButtonReleaseMask);

    // Показываем окно кнопки
    XMapWindow(display, button->window);
}

void drawButton(Display *display, Window window, GC gc, Button *button) {
    int screen = DefaultScreen(display);

    Colormap colormap = DefaultColormap(display, screen);
    XColor light_gray, dark_gray, white;
    XAllocNamedColor(display, colormap, "#DCDCDC", &light_gray, &light_gray);
    XAllocNamedColor(display, colormap, "#808080", &dark_gray, &dark_gray);
    XAllocNamedColor(display, colormap, "#FFFFFF", &white, &white);

    XClearWindow(display, window);

    // Отрисовка фона
    XSetForeground(display, gc, button->bg_color);
    XFillRectangle(display, window, gc, 0, 0, button->width, button->height);

    if (button->is_pressed) {
        // Левая и верхняя границы
        XSetForeground(display, gc, dark_gray.pixel);
        XFillRectangle(display, window, gc, 0, 0, 2, button->height);
        XFillRectangle(display, window, gc, 0, 0, button->width, 2);

        // Правая и нижняя границы
        XSetForeground(display, gc, white.pixel);
        XFillRectangle(display, window, gc, button->width - 2, 0, 2, button->height);
        XFillRectangle(display, window, gc, 0, button->height - 2, button->width, 2);
    } else {
        // Левая и верхняя границы
        XSetForeground(display, gc, white.pixel);
        XFillRectangle(display, window, gc, 0, 0, 2, button->height);
        XFillRectangle(display, window, gc, 0, 0, button->width, 2);

        // Правая и нижняя границы 
        XSetForeground(display, gc, dark_gray.pixel);
        XFillRectangle(display, window, gc, button->width - 2, 0, 2, button->height);
        XFillRectangle(display, window, gc, 0, button->height - 2, button->width, 2);
    }

    // Отрисовка текста
    XSetForeground(display, gc, BlackPixel(display, screen));
    const char *text = button->is_pressed ? button->pressed_text : button->released_text;
    int text_x = 10 + (button->is_pressed ? 1 : 0); // Смещение текста
    int text_y = button->height / 2 + 5 + (button->is_pressed ? 1 : 0);
    XDrawString(display, window, gc, text_x, text_y, text, strlen(text));
}

void pressButton(Button *button, Display *display, GC gc) {
    button->is_pressed = 1;
    drawButton(display, button->window, gc, button);
}

void releaseButton(Button *button, Display *display, GC gc) {
    button->is_pressed = 0;
    drawButton(display, button->window, gc, button);
}