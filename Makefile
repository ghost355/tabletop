# Определяем компилятор
CC = gcc

# Опции компилятора: 
# -Wall включает все предупреждения, 
# а -g добавляет отладочную информацию
CFLAGS = -Wall -g $(shell pkg-config --cflags $(LIBRARY))

# Опции линковщика
LDFLAGS = $(shell pkg-config --libs $(LIBRARY))

# Определение папок
SRCDIR := src
INCDIR := include
BUILDDIR := build

# Имя исполняемого файла
EXE := $(BUILDDIR)/tabletop

# Получаем список всех .c файлов в папке src
SRC := $(wildcard $(SRCDIR)/*.c)

# Получаем список всех .h файлов в папке include
HDR := $(wildcard $(INCDIR)/*.h)

# Преобразуем список исходных файлов в список объектных файлов
OBJ := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRC))

# Объявляем псевдоцели
.PHONY: all clean run

# Основная цель: создание исполняемого файла
all: $(EXE)

# Правило для создания исполняемого файла из объектных файлов
$(EXE): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

# Правило для компиляции каждого .c файла в соответствующий .o файл
$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(HDR)
	@mkdir -p $(dir $@) 				# Создаем нужную структуру папок
	$(CC) $(CFLAGS) -c $< -o $@ # Компиляция .c файла в .o файл

# Цель для очистки проекта
clean:
	rm -rf $(BUILDDIR)

# Цель для запуска исполняемого файла
run:
	$(EXE)

# Библиотека по-умолчанию
LIBRARY ?= "sdl3 sdl3-image sdl3-ttf"
