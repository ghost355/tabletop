# Определяем компилятор
CC = clang

# Имя приложения (без .app)
APPNAME = Tabletop

# Пути для .app bundle
APPDIR = $(APPNAME).app
CONTENTSDIR = $(APPDIR)/Contents
MACOSDIR = $(CONTENTSDIR)/MacOS
RESOURCESDIR = $(CONTENTSDIR)/Resources

# Остальные настройки
CFLAGS = -Wall -g $(shell pkg-config --cflags $(LIBRARY)) -I$(INCDIR)
LDFLAGS = $(shell pkg-config --libs $(LIBRARY)) -framework Metal -framework Cocoa

SRCDIR := src
INCDIR := include
BUILDDIR := build
ASSETSDIR := assets

EXE := $(BUILDDIR)/$(APPNAME)
SRC := $(wildcard $(SRCDIR)/*.c)
HDR := $(wildcard $(INCDIR)/*.h)
OBJ := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRC))

.PHONY: all clean app app-run

# Основная цель: обычная сборка без .app
all: $(EXE)

# Сборка .app bundle
app: $(EXE) Info.plist
	@echo "Создаем структуру .app..."
	@mkdir -p $(MACOSDIR)
	@mkdir -p $(RESOURCESDIR)
	@cp $(EXE) $(MACOSDIR)/$(APPNAME)
	@cp Info.plist $(CONTENTSDIR)
	@cp -r $(ASSETSDIR)/* $(RESOURCESDIR)
	@touch $(APPDIR)

# Запуск .app bundle
app-run: app
	@echo "Запуск приложения..."
	@open $(APPDIR)

# Правила для основного исполняемого файла
$(EXE): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(HDR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Очистка
clean:
	rm -rf $(BUILDDIR)
	rm -rf $(APPDIR)

# Библиотеки
LIBRARY ?= sdl3 sdl3-image sdl3-ttf
