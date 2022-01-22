#ifndef STATE_H
#define STATE_H

u64 StateSize(void);
bool SaveState(TI83_t* TI83, void* buf);
bool LoadState(TI83_t* TI83, void* buf);

#endif
