//
// Created by nyanfall on 10/8/23.
//

#ifndef main_h
#define main_h

#include <stdio.h>

#include "pd_api.h"

extern PlaydateAPI* pd;
void setupMainMenu(void);
int mainMenuUpdate(void* userdata);

#endif /* main_h */