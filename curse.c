#include <curses.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define MIN_Y 2
#define CONTROLS 8        // Количество элементов в массиве управления
#define INITIAL_DELAY 100 //  начальная задержка

enum
{
    LEFT = 1,
    UP,
    RIGHT,
    DOWN,
    STOP_GAME = KEY_F(10)
};

enum
{
    MAX_TAIL_SIZE = 100,
    START_TAIL_SIZE = 3,
    MAX_FOOD_SIZE = 20,
    FOOD_EXPIRE_SECONDS = 10
};

// Здесь храним коды управления змейкой
struct control_buttons
{
    int down;
    int up;
    int left;
    int right;
    int down_wasd;
    int up_wasd;
    int left_wasd;
    int right_wasd;
};

struct control_buttons default_controls = {
    KEY_DOWN, KEY_UP, KEY_LEFT, KEY_RIGHT,
    's', 'w', 'a', 'd' // WSAD
};

/*
 Голова змейки содержит в себе
 x,y - координаты текущей позиции
 direction - направление движения
 tsize - размер хвоста
 *tail - ссылка на хвост
 */
typedef struct snake_t
{
    int x;
    int y;
    int direction;
    size_t tsize;
    struct tail_t *tail;
    struct control_buttons controls;
} snake_t;

/*
 Хвост это массив состоящий из координат
x,y
 */
typedef struct tail_t
{
    int x;
    int y;
} tail_t;

struct food
{
    int x;
    int y;
    time_t put_time;
    char point;
    uint8_t enable;
} food[MAX_FOOD_SIZE];

void initTail(struct tail_t t[], size_t size)
{
    struct tail_t init_t = {0, 0};
    for (size_t i = 0; i < size; i++)
    {
        t[i] = init_t;
    }
}

void initHead(struct snake_t *head, int x, int y)
{
    head->x = x;
    head->y = y;
    head->direction = RIGHT;
}

void initSnake(snake_t *head, size_t size, int x, int y)
{
    tail_t *tail = (tail_t *)
        malloc(MAX_TAIL_SIZE * sizeof(tail_t));
    initTail(tail, MAX_TAIL_SIZE);
    initHead(head, x, y);
    head->tail = tail; // прикрепляем к голове хвост
    head->tsize = size + 1;
    head->controls = default_controls;
}

void initFood(struct food f[], size_t size)
{
    struct food init = {0, 0, 0, 0, 0};
    int max_y = 0, max_x = 0;
    getmaxyx(stdscr, max_y, max_x);
    for (size_t i = 0; i < size; i++)
    {
        f[i] = init;
    }
}

/*
 Движение головы с учетом текущего направления движения
 */
void go(struct snake_t *head)
{
    char ch = '@';
    int max_x = 0, max_y = 0;
    getmaxyx(stdscr, max_y, max_x);  // macro - размер терминала
    mvprintw(head->y, head->x, " "); // очищаем один символ
    switch (head->direction)
    {
    case LEFT:
        if (head->x <= 0)
            head->x = max_x;
        mvprintw(head->y, --(head->x), "%c", ch);
        break;
    case RIGHT:
        if (head->x >= max_x)
            head->x = 0;
        mvprintw(head->y, ++(head->x), "%c", ch);
        break;
    case UP:
        if (head->y <= MIN_Y)
            head->y = max_y;
        mvprintw(--(head->y), head->x, "%c", ch);
        break;
    case DOWN:
        if (head->y >= max_y)
            head->y = MIN_Y;
        mvprintw(++(head->y), head->x, "%c", ch);
        break;
    default:
        break;
    }
    refresh();
}

/*
Движение хвоста с учетом движения головы
*/
void goTail(struct snake_t *head)
{
    char ch = '*';
    mvprintw(head->tail[head->tsize - 1].y, head->tail[head->tsize - 1].x, " ");
    for (size_t i = head->tsize - 1; i > 0; i--)
    {
        head->tail[i] = head->tail[i - 1];
        if (head->tail[i].y || head->tail[i].x)
            mvprintw(head->tail[i].y, head->tail[i].x, "%c", ch);
    }
    head->tail[0].x = head->x;
    head->tail[0].y = head->y;
}

int checkDirection(snake_t *snake, int32_t key)
{
    if ((snake->direction == RIGHT && key == snake->controls.left) ||
        (snake->direction == LEFT && key == snake->controls.right) ||
        (snake->direction == UP && key == snake->controls.down) ||
        (snake->direction == DOWN && key == snake->controls.up))
    {
        return 0; // Недопустимое направление
    }

    return 1; // Направление корректное
}

void changeDirection(snake_t *snake, const int32_t key)
{
    if (checkDirection(snake, key))
    {
        if (key == snake->controls.down)
            snake->direction = DOWN;
        else if (key == snake->controls.up)
            snake->direction = UP;
        else if (key == snake->controls.right)
            snake->direction = RIGHT;
        else if (key == snake->controls.left)
            snake->direction = LEFT;
        else if (key == snake->controls.down_wasd)
            snake->direction = DOWN;
        else if (key == snake->controls.up_wasd)
            snake->direction = UP;
        else if (key == snake->controls.left_wasd)
            snake->direction = LEFT;
        else if (key == snake->controls.right_wasd)
            snake->direction = RIGHT;
    }
}

/*
 Обновить/разместить текущее зерно на поле
 */
void putFoodSeed(struct food *fp)
{
    int max_x = 0, max_y = 0;
    char spoint[2] = {0};
    getmaxyx(stdscr, max_y, max_x);
    mvprintw(fp->y, fp->x, " ");
    fp->x = rand() % (max_x - 1);
    fp->y = rand() % (max_y - 2) + 1; // Не занимаем верхнюю строку
    fp->put_time = time(NULL);
    fp->point = '$';
    fp->enable = 1;
    spoint[0] = fp->point;
    mvprintw(fp->y, fp->x, "%s", spoint);
}
/*
 Разместить еду на поле
 */
void putFood(struct food f[], size_t
                                  number_seeds)
{
    for (size_t i = 0; i < number_seeds; i++)
    {
        putFoodSeed(&f[i]);
    }
}

void refreshFood(struct food f[], int nfood)
{
    int max_x = 0, max_y = 0;
    char spoint[2] = {0};
    getmaxyx(stdscr, max_y, max_x);
    for (size_t i = 0; i < nfood; i++)
    {
        if (f[i].put_time)
        {
            if (!f[i].enable || (time(NULL) - f[i].put_time) > FOOD_EXPIRE_SECONDS)
            {
                putFoodSeed(&f[i]);
            }
        }
    }
}

/*
 поедание зерна змейкой
 */
_Bool haveEat(struct snake_t *head, struct food f[])
{
    for (size_t i = 0; i < MAX_FOOD_SIZE; i++)
        if (f[i].enable && head->x == f[i].x && head->y == f[i].y)
        {
            f[i].enable = 0;
            return 1;
        }
    return 0;
}

/*
 Увеличение хвоста на 1 элемент
 */
void addTail(struct snake_t *head)
{
    if (head == NULL || head->tsize > MAX_TAIL_SIZE)
    {
        mvprintw(0, 0, "Can't add tail");
        return;
    }
    head->tsize++;
}

int DELAY = INITIAL_DELAY; //  текущая задержка
int level = 0;             //  текущий уровень
_Bool is_paused = 0;       //  статус паузы

void printLevel(snake_t *head)
{
    mvprintw(1, 0, "Level: %d", level);
    refresh();
}

void printExit(snake_t *head)
{
    mvprintw(0, 0, "Game Over! Final Level: %d", level);
    refresh();
}

void togglePause()
{
    is_paused = !is_paused;
    if (is_paused)
    {
        mvprintw(0, 0, "Game Paused. Press 'P' to resume.");
    }
    else
    {
        mvprintw(0, 0, "Game resumed. Use arrows or WSAD for control. Press 'P' to pause.");
    }
    refresh();
}

void increaseSpeed()
{
    if (DELAY > 10)
    {
        DELAY -= 10;
    }
}

int main()
{
    // Инициализация змейки
    snake_t *snake = (snake_t *)malloc(sizeof(snake_t));
    initSnake(snake, START_TAIL_SIZE, 10, 10);

    // Инициализация еды
    initFood(food, MAX_FOOD_SIZE);
    putFood(food, MAX_FOOD_SIZE);

    // Инициализация curses
    initscr();
    keypad(stdscr, TRUE); // Включаем F1, F2, стрелки и т.д.
    raw();                // Отключаем line buffering
    noecho();             // Отключаем echo() режим при вызове getch
    curs_set(FALSE);      // Отключаем курсор
    mvprintw(0, 0, "Use arrows or WSAD for control. Press 'P' to pause. Press 'F10' for EXIT");
    timeout(0); // Отключаем таймаут после нажатия клавиши в цикле

    printLevel(snake); //  Отобразить начальный уровень

    // Основной цикл игры
    int key_pressed = 0;
    while (key_pressed != STOP_GAME)
    {
        key_pressed = getch(); // Считываем клавишу

        if (key_pressed == 'P' || key_pressed == 'p') //  Обработка паузы
        {
            togglePause();
            continue;
        }

        if (!is_paused) //  Игровая логика только если игра не на паузе
        {
            go(snake);
            goTail(snake);

            // Проверка на поедание еды
            if (haveEat(snake, food))
            {
                addTail(snake);  // Увеличиваем хвост при съедании еды
                level++;         // Увеличиваем уровень
                increaseSpeed(); // Увеличиваем скорость
                printLevel(snake);
            }

            refreshFood(food, MAX_FOOD_SIZE); // Обновляем еду
            timeout(DELAY);                   // Задержка при отрисовке
            changeDirection(snake, key_pressed);
        }
    }

    // Завершение программы
    printExit(snake); // Финальный результат
    free(snake->tail);
    free(snake);
    endwin(); // Завершаем режим curses mod
    return 0;
}