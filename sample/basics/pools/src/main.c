// *****************************************************************************
// Pools example
//
// This example shows how you can use the SGDK object pooling system
// The pooling system allows to move the creation of a game object
// (sprites, custom structs etc.) from the main loop to the initialization
// phase and reuse them dynamically as needed, which provides two key benefits:
// - reduces CPU load by eliminating frequent allocation/deallocation
// - minimizes memory fragmentation by maintaining fixed memory blocks
//
// Written by werton playskin 04/2025
// GFX artwork by Luis Zuno (@ansimuz)
// SFX by JDSherbert jdsherbert.itch.io
// *****************************************************************************

#include <genesis.h>
#include "resources.h"

// Game constants
#define PROJECTILES_IN_POOL             6           // Number of player's projectiles available in the pool
#define EXPLOSION_IN_POOL               8           // Number of explosions available in the pool
#define ENEMIES_IN_POOL                 16          // Number of enemies available in the pool
#define PLAYER_SPEED                    FIX16(3)    // Player movement speed
#define PLAYER_HEIGHT                   24          // Player object height
#define PLAYER_WIDTH                    24          // Player object width
#define PLAYER_SPAWN_POS_X              0           // Player spawn x position
#define PLAYER_SPAWN_POS_Y              32          // Player spawn y position
#define PLAYER_RESPAWN_DELAY            60          // Frames before respawn
#define PROJECTILE_COOLDOWN_TIMER       12          // Frames between player shots
#define PROJECTILE_SPEED                FIX16(10)   // Projectile movement speed
#define PROJECTILE_HEIGHT               10          // Projectile object height
#define PROJECTILE_WIDTH                22          // Projectile object width
#define ENEMY_SPEED                     FIX16(2.5)  // Enemy movement speed
#define ENEMY_SIZE                      24          // Enemy object size
#define ENEMIES_IN_WAVE                 16          // Enemies per wave
#define ENEMY_DELAY                     10          // Delay between enemy spawns
#define ENEMY_NEXT_WAVE_DELAY           110         // Delay between waves
#define ENEMY_WAVE_OFFSET_Y             80          // Player wave y position offset
#define EXPLOSION_SIZE                  32          // Explosion object size

// Helper macro for iterating through all objects in a specific pool (used for initialisation object)
#define FOREACH_IN_POOL(ObjectType, object, pool) \
    for (ObjectType **ptr = (ObjectType **)pool->allocStack, *object = *ptr;  /* Initialize pointer to start of pool's allocation stack and get first object */  \
         ptr < (ObjectType **)pool->allocStack + pool->size;                  /* Continue while pointer is within pool bounds (start + size) */ \
         object = *++ptr)                                                     /* Move to next pointer in stack and dereference to get next object */

// Helper macro for iterating through all allocated objects in a specific pool (used in main cycle for updating active objects)
#define FOREACH_ALLOCATED_IN_POOL(ObjectType, object, pool) \
    for (ObjectType **ptr = (ObjectType **)POOL_getFirst(pool), *object = *ptr;   /* Initialize pointer to first allocated object in pool */ \
         ptr < (ObjectType **)POOL_getFirst(pool) + POOL_getNumAllocated(pool);   /* Continue while pointer is within allocated objects range */ \
         object = *++ptr)                                                         /* Move to next pointer and dereference to get next allocated object */

// Player state enumeration
typedef enum
{
    PL_STATE_DIED,      // Player is dead/waiting to respawn
    PL_STATE_NORMAL,    // Player is alive and active
} PlayerState;

// Basic game object
typedef struct
{
    Sprite *sprite;     // Sprite reference
    fix16 x, y;         // Position (fixed point)
    u16 w, h;           // Hitbox width and height
} GameObject;

// Player object
typedef struct
{
    GameObject;         // Inherits base GameObject properties
    PlayerState state;  // Current player state
    u16 coolDownTimer;  // Shooting cooldown counter
    u16 respawnTimer;   // Timer for respawning after death
} Player;

// Projectile object
typedef struct
{
    GameObject;         // Inherits base GameObject properties
    u8 damage;            // Not used in this example, needed just to show the usage of different objects in pools
} Projectile;

// Enemy object
typedef struct
{
    GameObject;         // Inherits base GameObject properties
    u16 hp;             // Not used in this example, needed just to show the usage of different objects in pools
} Enemy;

// Current enemy wave state
typedef struct
{
    u16 enemyCount;     // Total enemies remaining in current wave
    u16 waveDelay;      // Current delay counter before next wave
    u16 enemyDelay;     // Current delay counter between enemy spawns
} EnemyWave;

// Main game state structure
typedef struct
{
    Player player;                  // Player object
    EnemyWave wave;                 // Current enemy wave state
    
    // Declaring pools for game objects
    Pool *projectilePool;           // Projectiles object pool
    Pool *enemyPool;                // Enemies object pool
    Pool *explosionPool;            // Explosion effects pool
} GameState;

// Global game state
GameState game;

// Function Prototypes
void Game_Init();
void Game_MainLoop();
void Game_UpdateExplosions();
void Game_CreateObjectsPools();
void Game_UpdateEnemies();
void Game_UpdateProjectiles();
void Game_UpdateProjectilesCollision();
void Game_DrawUI();
void GameObject_Create(GameObject *object, const SpriteDefinition *spriteDef, u16 pal, s16 x, s16 y, u16 w, u16 h);
void GameObject_Init(GameObject *object, s16 x, s16 y);
void GameObject_Release(GameObject *gameObject, Pool *pool);
void GameObject_ReleaseWithExplode(GameObject *object, Pool *pool);
bool GameObject_IsCollided(GameObject *obj1, GameObject *obj2);
void Player_Create();
void Player_Spawn();
void Player_Update();
void Player_Explode();
void Player_TryShoot();
void Player_UpdateInput();
void Player_TryRespawn();
void Player_UpdatePosition();
void Player_UpdateCollision();
void Enemy_Spawn(s16 x, s16 y);
void EnemyWave_Reset();
void EnemyWave_Update();
void Projectile_Spawn(s16 x, s16 y);
void Explosion_Spawn(s16 x, s16 y);

// Entry Point
int main(bool hardReset)
{
    if (!hardReset)
        SYS_hardReset();
    
    Game_Init();
    Game_MainLoop();
    return 0;
}

// Main game loop - updates all game systems each frame
void Game_MainLoop()
{
    while (TRUE)
    {
        Player_Update();
        Game_UpdateProjectiles();
        EnemyWave_Update();
        Game_UpdateEnemies();
        Game_UpdateExplosions();
        Game_UpdateProjectilesCollision();
        Game_DrawUI();
        SPR_update();
        SYS_doVBlankProcess();
    }
}

// Initialize all game systems and resources
void Game_Init()
{
    // Initialize systems
    Z80_loadDriver(Z80_DRIVER_XGM2, TRUE);
    JOY_init();
    SPR_init();
    
    // Load sprite palettes and set the text palette
    PAL_setPalette(PAL1, player_sprite.palette->data, DMA);
    PAL_setPalette(PAL2, explosion_sprite.palette->data, DMA);
    PAL_setPalette(PAL3, enemy_sprite.palette->data, DMA);
    VDP_setTextPalette(PAL1);
    
    // Initialize game objects
    Game_CreateObjectsPools();
    Player_Create();
    EnemyWave_Reset();
}

// Create object pools for game entities
void Game_CreateObjectsPools()
{
    // ================================== Pool usage =======================================
    // Create object pool for each type of game objects
    game.enemyPool = POOL_create(ENEMIES_IN_POOL, sizeof(Enemy));
    game.projectilePool = POOL_create(PROJECTILES_IN_POOL, sizeof(Projectile));
    game.explosionPool = POOL_create(EXPLOSION_IN_POOL, sizeof(GameObject));
    
    // ================================== Pool usage =======================================
    // Here we iterate through all object in each pool, and initialize objects with initial
    // data (including creating sprites for each object in the pool, so as not to do this
    // in the main loop)
    // =====================================================================================
    // Iterate through all objects in projectilePool and init projectile
    FOREACH_IN_POOL(Projectile, projectile, game.projectilePool)
    {
        GameObject_Create((GameObject *) projectile, &projectile_sprite, PAL1, 0, 0, PROJECTILE_WIDTH, PROJECTILE_HEIGHT);
        SPR_setVisibility(projectile->sprite, HIDDEN);
    }
    
    // Iterate through all objects in enemyPool and init enemy
    FOREACH_IN_POOL(Enemy, enemy, game.enemyPool)
    {
        GameObject_Create((GameObject *) enemy, &enemy_sprite, PAL3, 0, 0, ENEMY_SIZE, ENEMY_SIZE);
        SPR_setVisibility(enemy->sprite, HIDDEN);
    }
    
    // Iterate through all objects in explosionPool and init explosion
    FOREACH_IN_POOL(GameObject, explosion, game.explosionPool)
    {
        GameObject_Create(explosion, &explosion_sprite, PAL2, 0, 0, 0, 0);
        SPR_setVisibility(explosion->sprite, HIDDEN);
    }
    // =====================================================================================
}

// Update all active enemies movement
void Game_UpdateEnemies()
{
    // ================================== Pool usage =======================================
    // Process all active(allocated) enemies
    FOREACH_ALLOCATED_IN_POOL(Enemy, enemy, game.enemyPool)
    {
        if (!enemy)
            continue;
        
        // Move enemy left
        enemy->x -= ENEMY_SPEED;
        SPR_setPosition(enemy->sprite, F16_toInt(enemy->x), F16_toInt(enemy->y));
        
        // Remove if off-screen left
        if (F16_toInt(enemy->x) < -ENEMY_SIZE)
            GameObject_Release((GameObject *) enemy, game.enemyPool);
    }
}

// Update all active explosions
void Game_UpdateExplosions()
{
    // ================================== Pool usage =======================================
    // Process all active(allocated) explosions
    FOREACH_ALLOCATED_IN_POOL(GameObject, explosion, game.explosionPool)
    {
        if (!explosion)
            continue;
        
        // Update position and check animation completion
        SPR_setPosition(explosion->sprite, F16_toInt(explosion->x), F16_toInt(explosion->y));
        explosion->x += ENEMY_SPEED;
        
        // Return object back to pool if animation finished
        if (SPR_isAnimationDone(explosion->sprite))
            GameObject_Release(explosion, game.explosionPool);
    }
}

// Update all active projectiles
void Game_UpdateProjectiles()
{
    // ================================== Pool usage =======================================
    // Process all active(allocated) projectiles
    FOREACH_ALLOCATED_IN_POOL(Projectile, projectile, game.projectilePool)
    {
        if (!projectile)
            continue;
        
        // Move projectile right
        projectile->x += PROJECTILE_SPEED;
        SPR_setPosition(projectile->sprite, F16_toInt(projectile->x), F16_toInt(projectile->y));
        
        // Remove if off-screen
        if (projectile->x > FIX16(VDP_getScreenWidth()))
            GameObject_Release((GameObject *) projectile, game.projectilePool);
    }
}

// Check collisions between projectiles and enemies
void Game_UpdateProjectilesCollision()
{
    // ================================== Pool usage =======================================
    // Iterate through all active(allocated) enemies
    FOREACH_ALLOCATED_IN_POOL(Enemy, enemy, game.enemyPool)
    {
        if (!enemy)
            continue;
        
        // ================================== Pool usage =======================================
        // Iterate through all activ(allocated) projectiles
        FOREACH_ALLOCATED_IN_POOL(Projectile, projectile, game.projectilePool)
        {
            if (!projectile)
                continue;
            
            // Skip if no collision
            if (!GameObject_IsCollided((GameObject *) projectile, (GameObject *) (GameObject *) enemy))
                continue;
            
            // Handle projectile collision with enemy
            GameObject_ReleaseWithExplode((GameObject *) enemy, game.enemyPool);
            GameObject_Release((GameObject *) projectile, game.projectilePool);
            break;
        }
    }
}

// Draw UI text
void Game_DrawUI()
{
    static char str1[30];
    static char str2[30];
    static char str3[30];
    
    // ================================== Pool usage =======================================
    // Draw the number of allocated / free objects for each pool
    sprintf(str1, "projectiles  %02d / %02d", POOL_getNumAllocated(game.projectilePool),
            POOL_getFree(game.projectilePool));
    sprintf(str2, "enemies      %02d / %02d", POOL_getNumAllocated(game.enemyPool),
            POOL_getFree(game.enemyPool));
    sprintf(str3, "explosions   %02d / %02d", POOL_getNumAllocated(game.explosionPool),
            POOL_getFree(game.explosionPool));
    
    VDP_drawTextBG(BG_B, "objects allocated / free", 15, 0);
    VDP_drawTextBG(BG_B, str2, 17, 1);
    VDP_drawTextBG(BG_B, str3, 17, 2);
    VDP_drawTextBG(BG_B, str1, 17, 3);
}

// Create game object with specified parameters
void GameObject_Create(GameObject *object, const SpriteDefinition *spriteDef, u16 pal,
                       s16 x, s16 y, u16 w, u16 h)
{
    // Create new sprite
    object->sprite = SPR_addSprite(spriteDef, x, y, TILE_ATTR(pal, FALSE, FALSE, FALSE));
    
    // Set object properties
    object->w = w;
    object->h = h;
    
    GameObject_Init(object, x, y);
}

// Initialize game object with specified parameters
void GameObject_Init(GameObject *object, s16 x, s16 y)
{
    SPR_setPosition(object->sprite, x, y);
    
    // Reset sprite animation state
    SPR_setVisibility(object->sprite, VISIBLE);
    SPR_setFrame(object->sprite, 0);
    
    // Set object properties
    object->x = FIX16(x);
    object->y = FIX16(y);
}

// Release game object back to pool
void GameObject_Release(GameObject *gameObject, Pool *pool)
{
    SPR_setVisibility(gameObject->sprite, HIDDEN);
    
    // ================================== Pool usage =======================================
    // Return object back to pool
    POOL_release(pool, gameObject, TRUE);
    // =====================================================================================
}

// Release object with explosion effect
void GameObject_ReleaseWithExplode(GameObject *object, Pool *pool)
{
    // Create explosion before releasing object
    Explosion_Spawn(F16_toInt(object->x), F16_toInt(object->y));
    GameObject_Release(object, pool);
}

// Check if two game objects are colliding
FORCE_INLINE bool GameObject_IsCollided(GameObject *obj1, GameObject *obj2)
{
    if (obj1->y > obj2->y + FIX16(obj2->h) || obj1->y + FIX16(obj1->h) < obj2->y)
        return FALSE;
    
    if (obj1->x + FIX16(obj1->w) < obj2->x || obj1->x > obj2->x + FIX16(obj2->w))
        return FALSE;
    
    return TRUE;
}

// Create player
void Player_Create()
{
    // Just create and spawn Player
    GameObject_Create((GameObject *) &game.player, &player_sprite, PAL1,
                      PLAYER_SPAWN_POS_X, PLAYER_SPAWN_POS_Y, PLAYER_WIDTH, PLAYER_HEIGHT);
    Player_Spawn();
}

// Spawn player at starting position
void Player_Spawn()
{
    GameObject_Init((GameObject *) &game.player, PLAYER_SPAWN_POS_X, PLAYER_SPAWN_POS_Y);
    game.player.state = PL_STATE_NORMAL;
}

// Update player based on current state
void Player_Update()
{
    switch (game.player.state)
    {
        // Normal gameplay state
        case PL_STATE_NORMAL:
            Player_UpdateInput();
            Player_UpdatePosition();
            Player_UpdateCollision();
            break;
            
            // Dead/respawning state
        case PL_STATE_DIED:
            Player_TryRespawn();
            break;
    }
}

// Handle player explosion and death state
void Player_Explode()
{
    // Create explosion effect and hide player
    Explosion_Spawn(F16_toInt(game.player.x), F16_toInt(game.player.y));
    SPR_setVisibility(game.player.sprite, HIDDEN);
    
    // Set respawn timer and change state
    game.player.respawnTimer = PLAYER_RESPAWN_DELAY;
    game.player.state = PL_STATE_DIED;
}

// Attempt to shoot projectiles if cooldown allows
void Player_TryShoot()
{
    // Check if still on cooldown
    if (game.player.coolDownTimer--)
        return;
    
    // Reset cooldown and spawn two projectiles
    game.player.coolDownTimer = PROJECTILE_COOLDOWN_TIMER;
    Projectile_Spawn(F16_toInt(game.player.x), F16_toInt(game.player.y));
    Projectile_Spawn(F16_toInt(game.player.x), F16_toInt(game.player.y) + PLAYER_WIDTH / 2);
    
    // Play shooting sound
    XGM2_playPCM(xpcm_shoot, sizeof(xpcm_shoot), SOUND_PCM_CH2);
}

// Update respawn timer and spawn player when ready
void Player_TryRespawn()
{
    if (--game.player.respawnTimer == 0)
        Player_Spawn();
}

// Update player position and keep within screen bounds
void Player_UpdatePosition()
{
    // Clamp position to screen boundaries
    game.player.x = clamp(game.player.x, FIX16(0), FIX16(VDP_getScreenWidth() - game.player.w));
    game.player.y = clamp(game.player.y, FIX16(0), FIX16(VDP_getScreenHeight() - game.player.h));
    
    // Update sprite position
    SPR_setPosition(game.player.sprite, F16_toInt(game.player.x), F16_toInt(game.player.y));
}

// Check collisions between player and enemies
void Player_UpdateCollision()
{
    // ================================== Pool usage =======================================
    // Iterate through all active(allocated) enemies
    FOREACH_ALLOCATED_IN_POOL(Enemy, enemy, game.enemyPool)
    {
        if (!enemy)
            continue;
        
        // Skip if no collision detected
        if (!GameObject_IsCollided((GameObject *) &game.player, (GameObject *) enemy))
            continue;
        
        // Handle player collision with enemy
        GameObject_ReleaseWithExplode((GameObject *) enemy, game.enemyPool);
        Player_Explode();
    }
}

// Handle player input and movement
void Player_UpdateInput()
{
    u16 input = JOY_readJoypad(0);
    
    // Horizontal movement
    if (input & BUTTON_LEFT)
        game.player.x -= PLAYER_SPEED;
    else if (input & BUTTON_RIGHT)
        game.player.x += PLAYER_SPEED;
    
    // Vertical movement with animation
    if (input & BUTTON_UP)
        game.player.y -= PLAYER_SPEED;
    else if (input & BUTTON_DOWN)
        game.player.y += PLAYER_SPEED;
    
    // Shooting input
    if (input & BUTTON_A)
        Player_TryShoot();
}

// Spawn enemy at specified position
void Enemy_Spawn(s16 x, s16 y)
{
    // ================================== Pool usage =======================================
    // Allocate enemy from pool
    Enemy *enemy = (Enemy *) POOL_allocate(game.enemyPool);
    
    // Exit function if no avaliable objects
    if (!enemy)
        return;
    
    // Initialize enemy properties
    GameObject_Init((GameObject *) enemy, x, y);
}

// Reset enemy wave state for new wave
void EnemyWave_Reset()
{
    game.wave.waveDelay = ENEMY_NEXT_WAVE_DELAY;
    game.wave.enemyCount = ENEMIES_IN_WAVE;
}

// Update enemy wave logic
void EnemyWave_Update()
{
    // Wait if wave delay active
    if (game.wave.waveDelay)
    {
        game.wave.waveDelay--;
        return;
    }
    
    // Wait if between enemy spawns
    if (game.wave.enemyDelay--)
        return;
    
    // Reset spawn delay and spawn two new enemies
    game.wave.enemyDelay = ENEMY_DELAY;
    Enemy_Spawn(VDP_getScreenWidth(), ENEMY_WAVE_OFFSET_Y);
    Enemy_Spawn(VDP_getScreenWidth(), VDP_getScreenHeight() - ENEMY_SIZE - ENEMY_WAVE_OFFSET_Y);
    
    // Decrease remaining enemies and reset wave if complete
    game.wave.enemyCount -= 2;
    if (!game.wave.enemyCount)
        EnemyWave_Reset();
}

// Spawn new projectile at specified position
void Projectile_Spawn(s16 x, s16 y)
{
    // ================================== Pool usage =======================================
    // Allocate projectile from pool
    Projectile *projectile = (Projectile *) POOL_allocate(game.projectilePool);
    
    // Exit function if no available objects
    if (!projectile)
        return;
    
    // Initialize projectile properties
    GameObject_Init((GameObject *) projectile, x, y);
    SPR_setAlwaysOnTop(projectile->sprite);
}

// Create explosion effect at specified position
void Explosion_Spawn(s16 x, s16 y)
{
    // ================================== Pool usage =======================================
    // Allocate explosion object from pool
    GameObject *explosion = (GameObject *) POOL_allocate(game.explosionPool);
    
    // Exit function if no available objects
    if (!explosion)
        return;
    
    // Initialize explosion properties
    GameObject_Init(explosion, x - EXPLOSION_SIZE / 2, y);
    SPR_setAlwaysOnTop(explosion->sprite);
    SPR_setAnimationLoop(explosion->sprite, FALSE);
    
    // Play explosion sound
    XGM2_playPCM(xpcm_explosion, sizeof(xpcm_explosion), SOUND_PCM_CH3);
}
