#ifndef HEADER_FIGURE
#define HEADER_FIGURE

#include "typedefs.h"

void Figure_DrawActive(void);
void Figure_DropHard(void);
void Figure_FixRemoveSpawn(void);
void Figure_Spawn(void);
u16 Figure_GetRowsBlockToRemove(RowsBlock* rowsBlock);
u16 Figure_TryRemoveLines(void);
void Figure_Fix(void);
void Figure_ShuffleBag(void);
u8 Figure_GetNextType(void);
CollisionType Figure_IsCollided(const Figure* figPtr);
FigCollision Figure_GetCollision(const Figure* figPtr);
void Figure_TryMoveTo(s16 horDir);


#endif // HEADER_FIGURE
