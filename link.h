#ifndef LINK_H
#define LINK_H

#include "ti83.h"

u8 LinkState(TI83_t* TI83);
void UpdateLinkPort(TI83_t* TI83);
void SendNextLinkFile(TI83_t* TI83);

#endif
