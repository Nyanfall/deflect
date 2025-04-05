//
// Created by nyanfall on 10/8/23.
//

#ifndef game_h
#define game_h

#include <stdio.h>

#include "pd_api.h"

int update(void* ud);
void setupGame(void);
void setupFont(LCDFont* f);
LCDBitmap *loadImageAtPath(const char *path);


#endif /* game_h */