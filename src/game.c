//  Created by Artem Panchenko on 28/10/23. @nyanfall
//  Copyright (c) Punch&Co. All rights reserved.

#include <stdlib.h>
#include <math.h>
#include "game.h"
#include "main.h"

float globalTimer = 0;

int imagesLoaded = 0;

uint32_t score = 0;
uint32_t currentScore = 0;

float playerSpeed = 3;

LCDBitmap** playerImages = NULL;

LCDSprite* goSprite = NULL;
LCDBitmap* goImage = NULL;

LCDFont* hudFont = NULL;

void plusTenScore (void)
{
    currentScore += 10;
    pd->system->logToConsole("%d", currentScore);
}

LCDBitmap* loadImageAtPath(const char *path)
{
    const char *outErr = NULL;
    LCDBitmap *img = pd->graphics->loadBitmap(path, &outErr);
    if ( outErr != NULL )
        pd->system->logToConsole("Error loading image at path '%s': %s", path, outErr);
    return img;
}
LCDFont* loadFontAtPath(const char *path)
{
    const char *outErr = NULL;
    LCDFont *font = pd->graphics->loadFont(path, &outErr);
    if ( outErr != NULL )
        pd->system->logToConsole("Error loading font at path '%s': %s", path, outErr);
    return font;
}

void preloadImages(void)
{
    hudFont = loadFontAtPath("fonts/");

    goImage = loadImageAtPath("images/gameovertext");
    goSprite = pd->sprite->newSprite();

    playerImages = pd->system->realloc(NULL, sizeof(LCDBitmap *) * 1);

    //char *path = NULL;
    // for ( uint8_t i = 0; i < 1; i++ ) //TODO: make a method?
    // {
    //     pd->system->formatString(&path, "images/player/char-%d", i);
    //     playerImages[i] = loadImageAtPath(path);
    // }
    imagesLoaded = 1;
}

// static SpriteCollisionResponseType overlapCollisionResponse(LCDSprite* sprite, LCDSprite* otherSprite)
// {
//     return kCollisionTypeOverlap;
// }

int gameOverUpdate(void* userdata)
{
    pd->sprite->updateAndDrawSprites();

    pd->graphics->drawText("Score:", strlen("Score:"), kUTF8Encoding, 180, 130);
    char* scoreText;
    pd->system->formatString(&scoreText, "%d", currentScore);
    pd->graphics->drawText(scoreText, strlen(scoreText), kUTF8Encoding, 200, 150);

    pd->graphics->drawText("Press A/B to continue", strlen("Press A/B to continue"), kUTF8Encoding, 120, 220);

    PDButtons pushed;
    pd->system->getButtonState(NULL, &pushed, NULL);

    if ( pushed & kButtonA || pushed & kButtonB )
    {
        pd->sprite->removeSprite(goSprite);
        setupMainMenu();
        pd->system->setUpdateCallback(mainMenuUpdate, pd);
    }

    return 1;
}

void callGameOver(void)
{
    pd->system->resetElapsedTime();

    pd->sprite->removeAllSprites();
    pd->sprite->setImage(goSprite, goImage, kBitmapUnflipped);
    pd->system->logToConsole("[GAME OVER]");

    score = 0;
    //TODO: add high score here.

    pd->sprite->moveTo(goSprite, 200, 120);
    pd->sprite->setZIndex(goSprite, 2000);
    pd->sprite->addSprite(goSprite);

    pd->system->setUpdateCallback(gameOverUpdate, pd);
}

void setupFont(LCDFont *font)
{
    if (font != NULL)
    {
        pd->graphics->setFont(font);
    }
}

void updateHUD(LCDSprite* sprite)
{
    if (score != currentScore)
    {
        score = currentScore;
        pd->sprite->markDirty(sprite);
    }
}
void drawHUD(LCDSprite* sprite, PDRect bounds, PDRect drawrect)
{
    char* scoreText;
    pd->system->formatString(&scoreText, "%d", score); //TODO: Add if with spacing
    pd->graphics->drawText(scoreText, strlen(scoreText), kASCIIEncoding, 200, 18);
    pd->system->realloc(scoreText, 0);
}

static void setupHUD(void)
{
    LCDSprite *hud = pd->sprite->newSprite();
    setupFont(hudFont);

    pd->sprite->setUpdateFunction(hud, updateHUD);
    pd->sprite->setDrawFunction(hud, drawHUD);

    PDRect hudBounds = PDRectMake(10, 10, 400, 40);
    pd->sprite->setBounds(hud, hudBounds);

    pd->sprite->setZIndex(hud, 1010);
    pd->sprite->addSprite(hud);
}

void setupGame(void)
{
    srand(pd->system->getSecondsSinceEpoch(NULL));
    if (imagesLoaded != 1)
       preloadImages();

    currentScore = 0;

   /* PDMenuItem *m1 = */ //pd->system->addMenuItem("-10hp", (void (*)(void *)) minusTenHP, NULL);
   /* PDMenuItem *m2 = */ pd->system->addMenuItem("+10 Score", (void (*)(void *)) plusTenScore, NULL);

    setupHUD();
}


int update(void* ud)
{
    globalTimer = pd->system->getElapsedTime();

    pd->sprite->updateAndDrawSprites();
    pd->system->drawFPS(0,0);

    return 1;
}