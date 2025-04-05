//
//  main.c
//  Extension
//
//  Created by Artem Panchenko on 28/10/23. @nyanfall
//  Copyright (c) Punch&Co. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "src/game.h"

PlaydateAPI* pd = NULL;

const char* fontpath = "/System/Fonts/Asheville-Sans-14-Bold.pft";
LCDFont* font = NULL;
int mbgY = 0;
int mbgX = 0;

#ifdef _WINDLL
__declspec(dllexport)
#endif

//sprites for menu:
char *scoreboardText = NULL;
LCDBitmap **splashImages = NULL;
LCDBitmap *mbgImage = NULL;
LCDSprite *mbgSprite = NULL;
LCDSprite *splashSprite = NULL;
LCDSprite *msbSprite = NULL;
LCDSprite *mcaSprite = NULL;

uint8_t isScoreBoardScreen = 0;
uint8_t isCrankTooltipActivated = 0;
int menuImagesLoaded = 0;

void setPDPtr(PlaydateAPI* p)
{
    pd = p;
}

void preloadMainMenuImages(void) //TODO: one method for preload everything
{
    char *path = NULL;
    splashImages = pd->system->realloc(NULL, sizeof(LCDBitmap *) * 9);
    for (int i = 0; i <= 8; i++ )
    {
        pd->system->formatString(&path, "images/punch_co/%d", i);
        splashImages[i] = loadImageAtPath(path);
    }

    mbgImage = loadImageAtPath("images/Menu");

    splashSprite = pd->sprite->newSprite();
    mbgSprite = pd->sprite->newSprite();
    msbSprite = pd->sprite->newSprite();
    mcaSprite = pd->sprite->newSprite();

    menuImagesLoaded = 1;
}

int splashUpdate (void* userdata)
{
    pd->sprite->updateAndDrawSprites();

    if (pd->system->getElapsedTime() > 2.5f)
    {
        setupMainMenu();
        pd->system->setUpdateCallback(mainMenuUpdate, pd);
    }
    return 1;
}

static void updateSplash(LCDSprite* sprite)
{

    pd->sprite->markDirty(sprite);
    pd->sprite->moveTo(sprite, 200, 120);
    float fr = pd->system->getElapsedTime() / 0.15f;
    if (fr < 9)
        pd->sprite->setImage(sprite, splashImages[(int)fr], kBitmapUnflipped);
}

void showSplash(void)
{
    pd->sprite->setImage(splashSprite, splashImages[0], kBitmapUnflipped);

    pd->sprite->setUpdateFunction(splashSprite, updateSplash);

    PDRect splashBounds = PDRectMake(200, 120, 400, 240);
    pd->sprite->setBounds(splashSprite, splashBounds);
    pd->sprite->moveTo(splashSprite, 200, 120);

    pd->sprite->setZIndex(splashSprite, 0);
    pd->sprite->addSprite(splashSprite);

    pd->system->resetElapsedTime();
}

static void drawScoreboardSprite(LCDSprite* sprite, PDRect bounds, PDRect drawrect)
{
    pd->graphics->clear(kColorBlack);
    pd->graphics->drawRect(10, 10, 380, 220, kColorWhite);
    pd->graphics->setDrawMode( kDrawModeFillWhite );

    pd->graphics->drawText("SCOREBOARD", strlen("SCOREBOARD"), kASCIIEncoding, 150, 15);
    //pd->scoreboards->getScores();
    //TODO:
    // ... render names ...
    pd->graphics->setDrawMode( kDrawModeCopy );
}


void setupScoreboard(void)
{
    mbgY = 0;
    pd->sprite->removeAllSprites();

    pd->sprite->setDrawFunction(msbSprite, drawScoreboardSprite);

    PDRect msbBounds = PDRectMake(0, 0, 400, 480);
    pd->sprite->setBounds(msbSprite, msbBounds);
    pd->sprite->setZIndex(msbSprite, 1);
    pd->sprite->addSprite(msbSprite);

}

void drawCrankUndock(LCDSprite* sprite, PDRect bounds, PDRect drawrect)
{
    int x = 240;
    int y = 210;

    pd->graphics->fillRect(x-1, y, 160, 30, kColorBlack);
    pd->graphics->drawRect(x, y, 160, 30, kColorWhite);
    pd->graphics->setDrawMode( kDrawModeFillWhite );
    pd->graphics->drawText("Crank Required ->", strlen("Crank Required ->"), kASCIIEncoding, x+15, y+6);
    pd->graphics->setDrawMode( kDrawModeCopy );
}

void setupCrankAlert(void)
{
    pd->sprite->setDrawFunction(mcaSprite, drawCrankUndock);

    pd->sprite->setZIndex(mcaSprite, 1);
    pd->sprite->addSprite(mcaSprite);

    PDRect mcaBounds = PDRectMake(240, 210, 160, 30);
    pd->sprite->setBounds(mcaSprite, mcaBounds);
}

void runGame(void)
{
    isCrankTooltipActivated = 0;
    pd->sprite->removeAllSprites();
    pd->system->setUpdateCallback(update, pd);
    setupGame();
    setupFont(font);
}

/* menu items:
PDMenuItem *menuItem = pd->system->addMenuItem("Item 1", menuItemCallback, NULL);
PDMenuItem *checkMenuItem = pd->system->addCheckmarkMenuItem("Item 2", 1, menuCheckmarkCallback, NULL);
const char *options[] = {"one", "two", "three"};
PDMenuItem *optionMenuItem = pd->system->addOptionsMenuItem("Item 3", options, 3, menuOptionsCallback, NULL);
*/

static void drawMenuSprite(LCDSprite* sprite, PDRect bounds, PDRect drawrect)
{
    pd->graphics->drawBitmap(mbgImage, mbgX-400, mbgY, kBitmapUnflipped);
}

static void mainMenuSpriteUpdate (LCDSprite* sprite)
{

    PDButtons current;
    pd->system->getButtonState(&current, NULL, NULL);
    float crank = pd->system->getCrankChange();

    if (current & kButtonDown || crank < 0.0f)
    {
        if (mbgY >= -240)
        {
            mbgY -= 10;
        }
    }
    else if (current & kButtonUp || crank > 0.0f)
    {
        if(mbgY < 0)
        {
            mbgY += 10;
        }
    }

    pd->sprite->markDirty(mbgSprite);
}

void setupMainMenu(void)
{
    pd->graphics->clear(kColorWhite);
    pd->sprite->removeAllSprites();
    mbgY = 0;

    pd->graphics->getBitmapData(mbgImage, &mbgX, NULL, NULL, NULL, NULL);

    pd->sprite->setUpdateFunction(mbgSprite, mainMenuSpriteUpdate);
    pd->sprite->setDrawFunction(mbgSprite, drawMenuSprite);

    PDRect mbgBounds = PDRectMake(0, 0, 400, 480);
    pd->sprite->setBounds(mbgSprite, mbgBounds);
    pd->sprite->setZIndex(mbgSprite, 0);
    pd->sprite->addSprite(mbgSprite);

}

int mainMenuUpdate(void* userdata)
{
    pd->sprite->updateAndDrawSprites();

    PDButtons pushed;
    pd->system->getButtonState(NULL, &pushed, NULL);

    uint8_t crankDocked = pd->system->isCrankDocked();

    if (isCrankTooltipActivated == 1 && crankDocked == 0 )
    {
        runGame();
    }

    if ( pushed & kButtonA && isScoreBoardScreen == 0 )
    {
        if (crankDocked == 0)
        {
            runGame();
        }
        else if (crankDocked == 1)
        {
            setupCrankAlert();
            isCrankTooltipActivated = 1;
        }
    }
    else if ( pushed & kButtonB && isScoreBoardScreen == 0 )
    {
        pd->graphics->clear(kColorClear);
        isScoreBoardScreen = 1;
        setupScoreboard();
    }
    else if ((pushed & kButtonB || pushed & kButtonA) && isScoreBoardScreen == 1 )
    {
        pd->graphics->clear(kColorBlack);
        isScoreBoardScreen = 0;
        setupMainMenu();
    }
    return 1;
}

int eventHandler(PlaydateAPI* playdate, PDSystemEvent event, uint32_t arg)
{
	(void)arg; // arg is currently only used for event = kEventKeyPressed

switch (event)
{
    case kEventInit:
        setPDPtr(playdate);
        pd->display->setRefreshRate(30);

        const char* err;
        font = pd->graphics->loadFont(fontpath, &err);
        if ( font == NULL )
            pd->system->error("%s:%i Couldn't load font %s: %s", __FILE__, __LINE__, fontpath, err);

        if (menuImagesLoaded != 1)
            preloadMainMenuImages();

        showSplash();
        playdate->system->setUpdateCallback(splashUpdate, playdate);
        break;
    case kEventTerminate:
    case kEventLowPower:
        // TODO: Save state.
        break;
    default:
        break;
    }

	return 0;
}
