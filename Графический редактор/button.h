#ifndef BUTTON_H
#define BUTTON_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>

typedef struct {
    Window window; // Дескриптор окна кнопки
    int x, y;
    int width, height;
    int is_pressed;
    const char *pressed_text;
    const char *released_text;
    unsigned long bg_color; // Цвет фона кнопки
} Button;

// Функция для создания кнопки
void createButton(Display *display, Window parent, Button *button);

// Функция для отрисовки кнопки
void drawButton(Display *display, Window window, GC gc, Button *button);

// Функции для изменения состояния кнопки
void pressButton(Button *button, Display *display, GC gc);
void releaseButton(Button *button, Display *display, GC gc);

#endif // BUTTON_H