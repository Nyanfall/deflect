//  Created by Artem Panchenko on 28/10/23. @nyanfall
//  Copyright (c) Punch&Co. All rights reserved.

#include <stdlib.h>
#include <math.h>
#include "game.h"
#include "../main.h"

//game state
/*#define*/ int maxEnemies = 6;
#define frameSpeed 0.15f

float globalTimer = 0;

int imagesLoaded = 0;
int enemyCount = 0;

//player info //TODO: Refactor it.
uint32_t score = 0;
uint32_t lvl = 0;
uint32_t currentScore = 0;
int health = 100;
int currentHealth = 0;
float playerSpeed = 3;
int axeDamage = 5;
float playerX = 200.0f;
float playerY = 120.0f;
int dodgeTime = 0;
int cantDodge = 0;
int dodgeDelay = 500;

LCDSprite* player = NULL;
LCDBitmap* skeleton = NULL;
LCDBitmap* zombie = NULL;

// background
LCDSprite* bgSprite = NULL;
LCDBitmap* bgImage = NULL;
int bgY = 0;
int bgX = 0;

//cached images
LCDBitmap** expImages = NULL;
LCDBitmap** playerImages = NULL;
LCDBitmap** skeletonImages = NULL;
LCDBitmap** zombieWalkImages = NULL;
LCDBitmap** zombieHitImages = NULL;
LCDBitmap** zombieDeathImages = NULL;

uint8_t playerFrames = 36;
uint8_t skeletonFrames = 1;
uint8_t zombieFrames = 8;

LCDSprite* goSprite = NULL;
LCDBitmap* goImage = NULL;

typedef enum {
    kPlayer = 0,
    kPlayerHitBox = 1,
    kEnemy = 2,
    kEnemyHitBox = 3,
    kAxe = 4
} SpriteType;

typedef enum {
    eZombie = 0,
    eSkeleton = 1
} EnemyType;

typedef enum {
    Walk = 0,
    Damaged = 1,
    Dodge = 2,
    Ability = 3,
    Dying = 4,
    Dead = 5
} enemyState;

typedef struct enemy {
    EnemyType type;
    enemyState state;
    int health;
    int damage;
    int scoreValue;
    float speed;
    LCDBitmap *image;
    LCDSprite *sprite;
    LCDSprite *hitBox;
    float x, y;
    int cantDodge;
    int dodgeTime;
    int dodgeDelay;
    float timer;
    float animationStartTime;
} enemy;

typedef struct enemies {
    int id;
    enemy data;
    struct enemies* next;
} enemies;

// Linked list guide credit: https://www.learnc.net/c-data-structures/c-linked-list/
enemies *firstEnemy = NULL;
typedef void (*callback)(enemies* data);

LCDBitmap* loadImageAtPath(const char *path)
{
    const char *outErr = NULL;
    LCDBitmap *img = pd->graphics->loadBitmap(path, &outErr);
    if ( outErr != NULL )
    {
        pd->system->logToConsole("Error loading image at path '%s': %s", path, outErr);
    }
    return img;
}

void preloadImages(void)
{
    bgImage = loadImageAtPath("images/generic-bg0");
    bgSprite = pd->sprite->newSprite();

    //xpImage = loadImageAtPath("images/xp");
    //xpSprite = pd->sprite->newSprite();

    goImage = loadImageAtPath("images/gameovertext");
    goSprite = pd->sprite->newSprite();

    playerImages = pd->system->realloc(NULL, sizeof(LCDBitmap *) * playerFrames);
    skeletonImages = pd->system->realloc(NULL, sizeof(LCDBitmap *) * skeletonFrames /* TODO: skeleton size*/);
    zombieWalkImages = pd->system->realloc(NULL, sizeof(LCDBitmap *) * 5 /* TODO: zombie size*/);
    zombieHitImages = pd->system->realloc(NULL, sizeof(LCDBitmap *) * 3 /* TODO: zombie size*/);
    zombieDeathImages = pd->system->realloc(NULL, sizeof(LCDBitmap *) * 7 /* TODO: zombie size*/);
    expImages = pd->system->realloc(NULL, sizeof(LCDBitmap *) * 4 /* TODO: zombie size*/);

    char *path = NULL;

    for ( uint8_t i = 0; i < playerFrames; i++ ) //TODO: make a method?
    {
        pd->system->formatString(&path, "images/player/char-%d", i);
        playerImages[i] = loadImageAtPath(path);
    }
    for ( uint8_t i = 0; i < skeletonFrames; i++ )
    {
        pd->system->formatString(&path, "images/enemies/skeleton/skeleton-%d", i);
        skeletonImages[i] = loadImageAtPath(path);
    }
    for ( uint8_t i = 0; i < 5; i++ )
    {
        pd->system->formatString(&path, "images/enemies/zombie/zombie-walk-%d", i);
        zombieWalkImages[i] = loadImageAtPath(path);
    }
    for ( uint8_t i = 0; i < 3; i++ )
    {
        pd->system->formatString(&path, "images/enemies/zombie/zombie-hit-%d", i);
        zombieHitImages[i] = loadImageAtPath(path);
    }
    for ( uint8_t i = 0; i < 7; i++ )
    {
        pd->system->formatString(&path, "images/enemies/zombie/zombie-death-%d", i);
        zombieDeathImages[i] = loadImageAtPath(path);
    }
    for ( uint8_t i = 0; i < 4; i++ )
    {
        pd->system->formatString(&path, "images/exp/exp-%d", i);
        expImages[i] = loadImageAtPath(path);
    }
    imagesLoaded = 1;
}

static SpriteCollisionResponseType overlapCollisionResponse(LCDSprite* sprite, LCDSprite* otherSprite)
{
    return kCollisionTypeOverlap;
}
/*
static SpriteCollisionResponseType bounceCollisionResponse(LCDSprite* sprite, LCDSprite* otherSprite)
{
    return kCollisionTypeBounce;
}
static SpriteCollisionResponseType freezeCollisionResponse(LCDSprite* sprite, LCDSprite* otherSprite)
{
    return kCollisionTypeFreeze;
}
static SpriteCollisionResponseType slideCollisionResponse(LCDSprite* sprite, LCDSprite* otherSprite)
{
    return kCollisionTypeSlide;
}
*/

// -- Background Sprite -- //

static void drawBackgroundSprite(LCDSprite* sprite, PDRect bounds, PDRect drawrect)
{
    pd->graphics->drawBitmap(bgImage, -bgX-400, -bgY-240, kBitmapUnflipped);
}

static void updateBackgroundSprite(LCDSprite* sprite)
{
    PDButtons current;
    pd->system->getButtonState(&current, NULL, NULL);

    if ( current & kButtonUp )
    {
        bgY -= playerSpeed;
    } else if ( current & kButtonDown )
    {
        bgY += playerSpeed;
    }
    if ( current & kButtonLeft )
    {
        if (current & kButtonUp || current & kButtonDown)
            bgX -= playerSpeed / 2;
        else
            bgX -= playerSpeed;
    } else if ( current & kButtonRight )
    {
        if (current & kButtonUp || current & kButtonDown)
            bgX += playerSpeed / 2;
        else
            bgX += playerSpeed;
    }

    if ( bgY >= 240 || bgY <= -240 )
    {
        bgY = 0;
    }
    if (bgX >= 400 || bgX <= -400)
    {
        bgX = 0;
    }
    pd->sprite->markDirty(bgSprite);
}

static void setupBackground(void)
{
    pd->graphics->getBitmapData(bgImage, NULL, &bgX, NULL, NULL, NULL);
    pd->sprite->setUpdateFunction(bgSprite, updateBackgroundSprite);
    pd->sprite->setDrawFunction(bgSprite, drawBackgroundSprite);
    PDRect bgBounds = PDRectMake(0, 0, 400, 240);
    pd->sprite->setBounds(bgSprite, bgBounds);
    pd->sprite->setZIndex(bgSprite, 0);
    pd->sprite->addSprite(bgSprite);
}

// -- ENEMY -- //

// remove enemies from the front of list
enemies* remove_front(enemies* head)
{
    if(head == NULL)
        return NULL;
    enemies *front = head;
    head = head->next;
    front->next = NULL;
    //is this the last enemies in the list
    if(front == head)
        head = NULL;
    pd->system->realloc(front,0);
    //free(front);
    return head;
}
// remove enemies from the back of the list
enemies* remove_back(enemies* head)
{
    if(head == NULL)
        return NULL;
    enemies *cursor = head;
    enemies *back = NULL;
    while(cursor->next != NULL)
    {
        back = cursor;
        cursor = cursor->next;
    }
    if(back != NULL)
        back->next = NULL;
    //if this is the last enemies in the list
    if(cursor == head)
        head = NULL;

    pd->system->realloc(cursor,0);
    //free(cursor);
    return head;
}
// remove an enemies from the list
enemies* remove_any(enemies* head,enemies* nd) //TODO: debug seg fault here
//TODO: check (Is it actual?)
{
    if(nd == NULL)
        return NULL;
    //if the enemies is the first enemies
    if(nd == head)
        return remove_front(head);
    //if the enemies is the last enemies
    if(nd->next == NULL)
        return remove_back(head);
    //if the enemies is in the middle
    enemies* cursor = head;
    while(cursor != NULL)
    {
        if(cursor->next == nd)
            break;
        cursor = cursor->next;
    }

    if(cursor != NULL)
    {
        enemies* tmp = cursor->next;
        cursor->next = tmp->next;
        tmp->next = NULL;
        pd->system->realloc(tmp,0);
        //free(tmp);
    }
    return head;
}

// Search for a specific enemies with input id, return the first matched enemies that stores the input data, otherwise return NULL
enemies* searchById(enemies* head,int id)
{
    enemies *cursor = head;
    while(cursor!=NULL)
    {
        if(cursor->id == id)
            return cursor;
        cursor = cursor->next;
    }
    return NULL;
}

// remove all element of the list
void dispose(enemies *head)
{
    enemies *cursor, *tmp;

    if(head != NULL)
    {
        cursor = head->next;
        head->next = NULL;
        while(cursor != NULL)
        {
            tmp = cursor->next;
            pd->system->realloc(cursor,0);
            //free(cursor);
            cursor = tmp;
        }
    }
}
//return the number of elements in the list
int count(enemies *head)
{
    enemies *cursor = head;
    int c = 0;
    while(cursor != NULL)
    {
        c++;
        cursor = cursor->next;
    }
    return c;
}

static void removeEnemy(enemies* e)
{
    currentScore += e->data.scoreValue;
    pd->sprite->removeSprite(e->data.sprite);
    pd->sprite->removeSprite(e->data.hitBox);
    pd->sprite->freeSprite(e->data.sprite);
    pd->sprite->freeSprite(e->data.hitBox);

    firstEnemy = remove_any(firstEnemy, searchById(firstEnemy, e->id));
}

static void updateEnemyHitBox(LCDSprite* sprite)
{
    enemies* ud = pd->sprite->getUserdata(sprite);

    if (ud->data.state == Dead)
    {
        if (searchById(firstEnemy, ud->id) != NULL)
        {
            removeEnemy(ud);
            enemyCount--;
        }
    }

    int len;
    float x, y;
    pd->sprite->getPosition(sprite, &x, &y);
    SpriteCollisionInfo *cInfo = pd->sprite->moveWithCollisions(sprite, x, y, NULL, NULL, &len);
    for (int i = 0; i < len; i++ )
    {
        if (cInfo != NULL)
        {
            SpriteCollisionInfo info = cInfo[i];
            if ( pd->sprite->getTag(info.other) == kAxe )
            {
                if (ud->data.cantDodge == 0)
                {
                    ud->data.dodgeTime = (int) pd->system->getCurrentTimeMilliseconds();
                    pd->sprite->setCollisionsEnabled(sprite, 0);
                    ud->data.health -= axeDamage;
                    ud->data.cantDodge = 1; //TODO: Refactor this to state data
                    if (ud->data.health > 0)
                    {
                        ud->data.state = Damaged;
                        ud->data.animationStartTime = globalTimer;
                        float pushbackDistance = 10;
                        float vec [2] = {playerX - ud->data.x, playerY - ud->data.y};
                        float normie = sqrtf( vec[0] * vec[0] + vec[1] * vec[1] );
                        ud->data.x -= vec[0] / normie * pushbackDistance;
                        ud->data.y -= vec[1] / normie * pushbackDistance;
                    }
                    else
                    {
                        //pd->system->resetElapsedTime();
                        ud->data.animationStartTime = globalTimer;
                        ud->data.state = Dying;
                    }
                }
            }
        }
    }
    if (ud->data.cantDodge == 1 && ud->data.dodgeTime != 0u && pd->system->getCurrentTimeMilliseconds() - ud->data.dodgeTime > ud->data.dodgeDelay )
    {
        pd->sprite->setCollisionsEnabled(sprite, 1);
        ud->data.cantDodge = 0;
    }
    pd->sprite->moveTo(sprite, ud->data.x, ud->data.y);
    if (cInfo != NULL)
        pd->system->realloc(cInfo, 0);
}

static void updateEnemy(LCDSprite* sprite)
{
    enemies* ud = pd->sprite->getUserdata(sprite);

    //ANIMATIONS:
    LCDBitmap** enemyImages = NULL;
    LCDBitmapFlip flip = kBitmapUnflipped;

    if (ud->data.x < 200)
        flip = kBitmapFlippedX;

    pd->sprite->setZIndex(sprite, ud->data.y + 40.0f); //Render priority from bottom to top (with delta 40)

    int t = pd->system->getElapsedTime() * 5; //TODO: FIND MORE CLEVER WAY TO ANIMATE IT PLS. (required unique timers... || calc diff from a one timer)
    switch (ud->data.state) {
        case Damaged:
            if (ud->data.type == eZombie)
            {
                float animationEnd = ud->data.animationStartTime + 0.33f; // TODO: animation length param?
                if (globalTimer >= animationEnd)
                {
                    ud->data.state = Walk;
                    break;
                }
                float f = globalTimer - ud->data.animationStartTime;
                f = f / 0.11f;  //TODO: frame length const
                pd->sprite->setImage(sprite, zombieHitImages[(int)f], flip);
                pd->sprite->markDirty(sprite);
            }
            else if (ud->data.type == eSkeleton)
            {
                enemyImages = skeletonImages;
                pd->sprite->setImage(sprite, enemyImages[1 % skeletonFrames], flip);
                ud->data.state = Walk;
            }
            break;
        case Dodge:
            //TODO: add immune phase here probably ? or remove state/bool
        case Ability:
            break;
        case Walk:
            switch (ud->data.type)
            {
            case eZombie:
                enemyImages = zombieWalkImages;
                pd->sprite->setImage(sprite, enemyImages[t % 5], flip);
                break;
            case eSkeleton:
                enemyImages = skeletonImages;
                pd->sprite->setImage(sprite, enemyImages[t % skeletonFrames], flip);
                break;
            }
        break;
        case Dying:
            switch (ud->data.type)
            {
            case eZombie:
                enemyImages = zombieDeathImages;
                ud->data.speed = 0;
                pd->sprite->setCollisionsEnabled(sprite, 0);
                pd->sprite->setCollisionsEnabled(ud->data.hitBox, 0);

                float animationEnd = ud->data.animationStartTime + 0.91f; // TODO: animation length param?
                if (globalTimer >= animationEnd)
                {
                    ud->data.state = Dead;
                    break;
                }
                float f = globalTimer - ud->data.animationStartTime;
                f = f / 0.13f;  //TODO: frame length const
                pd->sprite->setImage(sprite, enemyImages[(int)f], flip);

                break;
            case eSkeleton:
                enemyImages = skeletonImages;
                pd->sprite->setImage(sprite, enemyImages[t % skeletonFrames], flip);
                //removeEnemy(ud);
                break;
            }
            break;
        case Dead:
            removeEnemy(ud);
            // pd->system->resetElapsedTime();
            break;
    }
    //MOVEMENT:
    PDButtons current;
    pd->system->getButtonState(&current, NULL, NULL);

    //TODO: dx,dy -> next step from a*
    float dx = ud->data.x + playerX;
    float dy = ud->data.y + playerY;
    float rate = ud->data.speed / 2 / sqrtf(dx * dx + dy * dy);

    int len;
    float x, y;
    pd->sprite->getPosition(sprite, &x, &y);
    SpriteCollisionInfo *cInfo = pd->sprite->moveWithCollisions(sprite, x, y, NULL, NULL, &len);

    for (int i = 0; i < len; i++ )
    {
        if (cInfo != NULL)
        {
            SpriteCollisionInfo info = cInfo[i];
            if ( pd->sprite->getTag(info.other) == kPlayer && ud->data.state != Dying)
            {
                rate = 0;
                pd->sprite->setImage(sprite, zombieWalkImages[0], flip);
                //TODO: STOP HERE WHEN PLAYER BODY OCCURS
                pd->system->logToConsole("enemy collide with kPlayer");
            }
            else if (pd->sprite->getTag(info.other) == kEnemy)
            {
                //TODO: find other path
            }
        }
    }

    if ( current & kButtonLeft )
    {
        if ( current & kButtonDown && current & kButtonLeft )
        {
            ud->data.x -= (ud->data.speed - playerSpeed) / 2;
            ud->data.y += (ud->data.speed - playerSpeed) / 2;
        }
        else if ( current & kButtonUp && current & kButtonLeft )
        {
            ud->data.x -= (ud->data.speed - playerSpeed) / 2;
            ud->data.y -= (ud->data.speed - playerSpeed) / 2;
        }
        else
        {
            ud->data.x -= ud->data.speed - playerSpeed;
        }
    }
    else if ( current & kButtonRight )
    {
        if ( current & kButtonDown && current & kButtonRight )
        {
            ud->data.x += (ud->data.speed - playerSpeed) / 2;
            ud->data.y += (ud->data.speed - playerSpeed) / 2;
        }
        else if ( current & kButtonUp && current & kButtonRight )
        {
            ud->data.x += (ud->data.speed - playerSpeed) / 2;
            ud->data.y -= (ud->data.speed - playerSpeed) / 2;
        }
        else {
            ud->data.x += ud->data.speed - playerSpeed;
        }
    }
    else if ( current & kButtonUp )
    {
        if ( current & kButtonLeft && current & kButtonUp)
        {
            ud->data.y -= (ud->data.speed - playerSpeed) / 2;
            ud->data.x -= (ud->data.speed - playerSpeed) / 2;
        }
        else if ( current & kButtonRight && current & kButtonUp )
        {
            ud->data.y -= (ud->data.speed - playerSpeed) / 2;
            ud->data.x += (ud->data.speed - playerSpeed) / 2;
        }
        else {
            ud->data.y -= ud->data.speed - playerSpeed;
        }
    }
    else if ( current & kButtonDown )
    {
        if ( current & kButtonLeft && current & kButtonDown )
        {
            ud->data.y += (ud->data.speed - playerSpeed) / 2;
            ud->data.x -= (ud->data.speed - playerSpeed) / 2;
        }
        else if ( current & kButtonRight && current & kButtonDown )
        {
            ud->data.y += (ud->data.speed - playerSpeed) / 2;
            ud->data.x += (ud->data.speed - playerSpeed) / 2;
        }
        else {
            ud->data.y += ud->data.speed - playerSpeed;
        }
    }

    if (ud->data.x < playerX - 1) //left
        ud->data.x += dx * rate;
    else if (ud->data.x > playerX + 1) //right
        ud->data.x -= dx * rate;

    if (ud->data.y < playerY - 1) //up
        ud->data.y += dy * rate;
    else if (ud->data.y > playerY + 1) //down
        ud->data.y -= dy * rate; //TODO: fix bug here somewhat Speed reduced when going straight down.


    pd->sprite->moveTo(sprite, ud->data.x, ud->data.y);
    if (cInfo != NULL)
        pd->system->realloc(cInfo, 0);
}

enemies* createEnemy(int id, enemies* next)
{
    enemies* newEnemy = pd->system->realloc(NULL, sizeof(enemies));
    if(newEnemy == NULL)
    {
        pd->system->logToConsole("Error creating a new enemy.");
        return NULL;
    }
    newEnemy->id = id;
    newEnemy->next = next;

    newEnemy->data.state = Walk;
    newEnemy->data.cantDodge = 0;
    newEnemy->data.dodgeTime = 0;
    newEnemy->data.dodgeDelay = 500;

    newEnemy->data.type = 0;//rand() % 2; //TODO: remove
    newEnemy->data.x = rand() % 400;
    newEnemy->data.y = rand() % 240;
    //side's: top = 0, right = 1, bottom = 2, left = 3
    switch (rand()%4)
    {
        default:
        case 0:
            newEnemy->data.y = -15;
            break;
        case 1:
            newEnemy->data.x = 415;
            break;
        case 2:
            newEnemy->data.y = 255;
            break;
        case 3:
            newEnemy->data.x = -15;
            break;
    }
    newEnemy->data.speed = 0.0f;
    //enemyCharacter.image = pd->graphics->freeBitmap();
    newEnemy->data.sprite = pd->sprite->newSprite();
    newEnemy->data.hitBox = pd->sprite->newSprite();

    switch (newEnemy->data.type)
    {
        case eZombie:
            newEnemy->data.health = 15;
            newEnemy->data.scoreValue = 15;
            newEnemy->data.damage = 5;
            newEnemy->data.speed = 1.0f;
            newEnemy->data.image = zombieWalkImages[0];
            break;
        case eSkeleton:
            newEnemy->data.health = 10;
            newEnemy->data.scoreValue = 10;
            newEnemy->data.damage = 3;
            newEnemy->data.speed = 1.5f;
            newEnemy->data.image = skeletonImages[0];
            break;
    }

    int w,h;

    pd->graphics->getBitmapData(newEnemy->data.image, &w, &h, NULL, NULL, NULL);
    //Default collider rect sizes
    PDRect enemyRect = PDRectMake(10,5,(float)w-15.0f,(float)h-10.0f);
    PDRect enemyHitBox = PDRectMake(-5,-10,(float)w-20.0f,(float)h-15.0f);

    pd->sprite->setUpdateFunction(newEnemy->data.sprite, updateEnemy);
    pd->sprite->setUpdateFunction(newEnemy->data.hitBox, updateEnemyHitBox);

    pd->sprite->setImage(newEnemy->data.sprite, newEnemy->data.image, kBitmapUnflipped);

    pd->sprite->setCollideRect(newEnemy->data.sprite, enemyRect);
    pd->sprite->setCollideRect(newEnemy->data.hitBox, enemyHitBox);

    pd->sprite->setCollisionResponseFunction(newEnemy->data.sprite, overlapCollisionResponse);
    pd->sprite->setCollisionResponseFunction(newEnemy->data.hitBox, overlapCollisionResponse);

    pd->sprite->moveTo(newEnemy->data.sprite, newEnemy->data.x, newEnemy->data.y);
    pd->sprite->moveTo(newEnemy->data.hitBox, newEnemy->data.x, newEnemy->data.y);

    pd->sprite->setZIndex(newEnemy->data.sprite, 1000);
    pd->sprite->setZIndex(newEnemy->data.hitBox, 1000);
    pd->sprite->addSprite(newEnemy->data.sprite);
    pd->sprite->addSprite(newEnemy->data.hitBox);

    pd->sprite->setTag(newEnemy->data.sprite, kEnemy);
    pd->sprite->setTag(newEnemy->data.hitBox, kEnemyHitBox);

    pd->sprite->setUserdata(newEnemy->data.sprite, (void*) (size_t) newEnemy);
    pd->sprite->setUserdata(newEnemy->data.hitBox, (void*) (size_t) newEnemy);


    //pd->sprite->setDrawFunction(newEnemy->data.sprite, drawEnemy); //TODO: animations or smth? still idk...

    enemyCount++;
    return newEnemy;
}
enemies* prepend(enemies* head,int id)
{
    enemies* new_enemies = createEnemy(id, NULL);
    head = new_enemies;
    return head;
}
// add a new enemies at the end of the list
enemies* append(enemies* head, int id)
{
    if(head == NULL)
        return prepend(head,id); //return NULL;
    /* go to the last enemies */
    enemies *cursor = head;
    while(cursor->next != NULL)
        cursor = cursor->next;

    /* create a new enemies */
    enemies* new_enemies = createEnemy(id,NULL);
    cursor->next = new_enemies;

    return head;
}
// insert a new enemies after the prev enemies
enemies* insert_after(enemies *head, int id, enemies* prev)
{
    if(head == NULL || prev == NULL)
        return NULL;
    /* find the prev enemies, starting from the first enemies*/
    enemies *cursor = head;
    while(cursor != prev)
        cursor = cursor->next;

    if(cursor != NULL)
    {
        enemies* new_enemies = createEnemy(id,cursor->next);
        cursor->next = new_enemies;
        return head;
    }
    else
    {
        return NULL;
    }
}
// insert a new enemies before the nxt enemies
enemies* insert_before(enemies *head, int id, enemies* nxt)
{
    if(nxt == NULL || head == NULL)
        return NULL;
    if(head == nxt)
    {
        head = prepend(head,id);
        return head;
    }
    //find the prev enemies, starting from the first enemies
    enemies *cursor = head;
    while(cursor != NULL)
    {
        if(cursor->next == nxt)
            break;
        cursor = cursor->next;
    }
    if(cursor != NULL)
    {
        enemies* new_enemies = createEnemy(id,cursor->next);
        cursor->next = new_enemies;
        return head;
    }
    else
    {
        return NULL;
    }
}
// traverse the linked list
void traverse(enemies* head,callback f)
{
    enemies* cursor = head;
    while(cursor != NULL)
    {
        f(cursor);
        cursor = cursor->next;
    }
}
// display a enemies id's //TODO: remove from prod.
void display(enemies* n)
{
    if(n != NULL)
        pd->system->logToConsole("%d ", n->id);
}

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
    currentHealth = 100;
    score = 0;
    dispose(firstEnemy);
    //TODO: add high score here.

    pd->sprite->moveTo(goSprite, 200, 120);
    pd->sprite->setZIndex(goSprite, 2000);
    pd->sprite->addSprite(goSprite);

    pd->system->setUpdateCallback(gameOverUpdate, pd);
}

// -- PLAYER -- //

static void updatePlayer(LCDSprite* sprite)
{
    if (currentHealth <= 0 )
        callGameOver();

    float angle = pd->system->getCrankAngle();

    //TODO: think about more correct way to load playerImages[(int)angle/10];
    pd->sprite->setImage(sprite, playerImages[(int)angle/10], kBitmapUnflipped);

    pd->sprite->getPosition(sprite, &playerX, &playerY);
    int len;
    SpriteCollisionInfo *cInfo = pd->sprite->moveWithCollisions(sprite, playerX, playerY, NULL, NULL, &len);
     for (int i = 0; i < len; i++ )
     {
         SpriteCollisionInfo info = cInfo[i];
         if ( pd->sprite->getTag(info.other) == kEnemy )
         {
             pd->system->logToConsole("we are colliding rn");
         }
     }
     if (cInfo != NULL)
        pd->system->realloc(cInfo, 0);
}

static void rotateAxe(float angle, float * x, float * y)
{
    const float baseX = 200.f;
    const float baseY = 120.f;
    const float radius = 35.f;

    //TODO: reset x0 y0 each revolution. And possibly fix jump from 350 to 10;
    //why 54 ? (full circle plus half)
    *x = baseX + cosf(angle/54 - 90) * radius;
    *y = baseY + sinf(angle/54 - 90) * radius;
}

static void updateAxe(LCDSprite* sprite)
{
    float angle = pd->system->getCrankAngle();
    float axeX, axeY;

    pd->sprite->getPosition(sprite, &axeX, &axeY);
    rotateAxe(angle, &axeX, &axeY);

    int len;
    SpriteCollisionInfo *cInfo = pd->sprite->moveWithCollisions(sprite, axeX, axeY, NULL, NULL, &len);
    if (cInfo != NULL)
        pd->system->realloc(cInfo, 0);
}
static void updateCharacterHitBox(LCDSprite* sprite)
{
    int len;
    float x, y;
    pd->sprite->getPosition(sprite, &x, &y);
    SpriteCollisionInfo *cInfo = pd->sprite->moveWithCollisions(sprite, x, y, NULL, NULL, &len);
    for (int i = 0; i < len; i++ )
    {
        SpriteCollisionInfo info = cInfo[i];

        if ( pd->sprite->getTag(info.other) == kEnemy )
        {
            if (cantDodge == 0)
            {
                dodgeTime = (int) pd->system->getCurrentTimeMilliseconds();
                pd->sprite->setCollisionsEnabled(sprite, 0);
                cantDodge = 1;

                enemies* ud = pd->sprite->getUserdata(info.other);
                currentHealth -= ud->data.damage;
            }
        }
    }
    if (cantDodge == 1 && dodgeTime != 0u && pd->system->getCurrentTimeMilliseconds() - dodgeTime > dodgeDelay )
    {
        pd->sprite->setCollisionsEnabled(sprite, 1);
        cantDodge = 0;
    }
    if (cInfo != NULL)
        pd->system->realloc(cInfo, 0);
}

static LCDSprite* createPlayer(float centerX, float centerY)
{
    LCDSprite *character = pd->sprite->newSprite();
    LCDSprite *characterHitbox = pd->sprite->newSprite();
    LCDSprite *axe = pd->sprite->newSprite();

    pd->sprite->setUpdateFunction(character, updatePlayer);
    pd->sprite->setUpdateFunction(characterHitbox, updateCharacterHitBox);
    pd->sprite->setUpdateFunction(axe, updateAxe);

    int w, h;
    pd->graphics->getBitmapData(playerImages[0], &w, &h, NULL, NULL, NULL);

    pd->sprite->setImage(character, playerImages[0], kBitmapUnflipped);

    PDRect characterRect = PDRectMake(30,30,(float)w-60,(float)h-60);
    PDRect hitboxRect = PDRectMake(-15,-15,(float)w-50,(float)h-50);
    PDRect axeCollider = PDRectMake(-5,-5, 15,15);

    pd->sprite->setCollideRect(character, characterRect);
    pd->sprite->setCollideRect(characterHitbox, hitboxRect);
    pd->sprite->setCollideRect(axe, axeCollider);

    pd->sprite->setCollisionResponseFunction(character, overlapCollisionResponse);
    pd->sprite->setCollisionResponseFunction(characterHitbox, overlapCollisionResponse);
    pd->sprite->setCollisionResponseFunction(axe, overlapCollisionResponse);

    pd->sprite->moveTo(character, centerX, centerY);
    pd->sprite->moveTo(characterHitbox, centerX, centerY);
    pd->sprite->moveTo(axe, centerX, centerY);

    pd->sprite->setZIndex(character, 1001);
    pd->sprite->setZIndex(characterHitbox, 1001);
    pd->sprite->setZIndex(axe, 1001);

    pd->sprite->addSprite(character);
    pd->sprite->addSprite(characterHitbox);
    pd->sprite->addSprite(axe);

    pd->sprite->setTag(character, kPlayer);
    pd->sprite->setTag(characterHitbox, kPlayerHitBox);
    pd->sprite->setTag(axe, kAxe);

    return character;
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
    if (currentHealth != health)
    {
        health = currentHealth;
        pd->sprite->markDirty(sprite);
    }

    if (score != currentScore)
    {
        score = currentScore;
        pd->sprite->markDirty(sprite);
    }
}
void drawHUD(LCDSprite* sprite, PDRect bounds, PDRect drawrect)
{
    pd->graphics->fillRect(1, 222, 400, 20, kColorWhite);
    pd->graphics->drawRect(0, 222, 400, 20, kColorBlack);

    char* healthText;
    //sprintf(healthText, "%d", health);
    pd->system->formatString(&healthText, "%d", health);
    pd->graphics->drawText("HP", strlen("HP"), kASCIIEncoding, 8, 224);
    pd->graphics->drawText(healthText, strlen(healthText), kASCIIEncoding, 32, 224);
    pd->system->realloc(healthText, 0);

    char* scoreText;
    pd->system->formatString(&scoreText, "%d", score); //TODO: Add if with spacing
    pd->graphics->drawText("SCORE", strlen("SCORE"), kASCIIEncoding, 70, 224);
    pd->graphics->drawText(scoreText, strlen(scoreText), kASCIIEncoding, 130, 224);
    pd->system->realloc(scoreText, 0);

    char* lvlText;
    pd->system->formatString(&lvlText, "%d", lvl);
    pd->graphics->drawText("Lv.", strlen("Lv."), kASCIIEncoding, 350, 224);
    pd->graphics->drawText(lvlText, strlen(lvlText), kASCIIEncoding, 380, 224);
    pd->system->realloc(lvlText, 0);

    //xp bar:
    pd->graphics->drawRect(195, 225, 150, 14, kColorBlack);


}

static void setupHUD(void)
{
    LCDSprite *hud = pd->sprite->newSprite();

    pd->sprite->setUpdateFunction(hud, updateHUD);
    pd->sprite->setDrawFunction(hud, drawHUD);

    PDRect hudBounds = PDRectMake(0, 222, 400, 20);
    pd->sprite->setBounds(hud, hudBounds);

    pd->sprite->setZIndex(hud, 1010);
    pd->sprite->addSprite(hud);
}

void setupGame(void)
{
    srand(pd->system->getSecondsSinceEpoch(NULL));
    if (imagesLoaded != 1)
       preloadImages();

    enemyCount = 0;
    currentHealth = health;
    currentScore = 0;

   /* PDMenuItem *m1 = */ //pd->system->addMenuItem("-10hp", (void (*)(void *)) minusTenHP, NULL);
   /* PDMenuItem *m2 = */ //pd->system->addMenuItem("+10 Score", (void (*)(void *)) plusTenScore, NULL);

    setupBackground();
    player = createPlayer(playerX, playerY);

    setupHUD();
}

static void spawnEnemyIfNeeded(void)
{
    //spd->system->logToConsole("%d", maxEnemies);
    if ( count(firstEnemy) < maxEnemies )
    {
        //pd->system->logToConsole("%d", rand() % maxEnemies);
        if ( rand() % maxEnemies || maxEnemies == 1)
        {
            int id = rand() % 100000;
            if (count(firstEnemy) != 0)
                firstEnemy = append(firstEnemy, id);
            else
                firstEnemy = prepend(firstEnemy, id);

            if ((int)globalTimer % 3 == 0 && maxEnemies <= 64 ) //TODO: remove temp enemy scaling
            {
                maxEnemies++;
            }
        }
    }
}

int update(void* ud)
{
    globalTimer = pd->system->getElapsedTime();

    spawnEnemyIfNeeded();
    pd->sprite->updateAndDrawSprites();
    pd->system->drawFPS(0,0);

    return 1;
}