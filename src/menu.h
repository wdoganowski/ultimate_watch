#pragma once

#include "pebble.h"

// Menu structure definition

typedef struct MenuCell {
  char* label;
  char* subtitle;
  GBitmap* icon;
  void (*execute) (void);
} MenuCell;

typedef struct MenuSection {
  uint16_t length; // number of cells in the section
  char* label; // label of the section 
  MenuCell* cells;
} MenuSection;

typedef struct MenuInfo {
  uint16_t length; // number of sections
  MenuSection* sections;
} MenuInfo;

void menu_window_show(void);
void menu_window_init(void);
void menu_window_deinit(void);