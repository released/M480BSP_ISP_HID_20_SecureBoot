#ifndef SECURE_UPDATE_H
#define SECURE_UPDATE_H

#include <stdint.h>

#define SECURE_UPDATE_DEBOUNCE_MS       30UL
#define SECURE_UPDATE_OK                0
#define SECURE_UPDATE_ERROR_IAP         (-1)
#define SECURE_UPDATE_ERROR_LOADER      (-2)

void SecureUpdate_Init(void);
/* Queue an update from a button, UART ISR or application command. */
void SecureUpdate_Request(void);
/* Call from the privileged application main loop, not an interrupt handler. */
int32_t SecureUpdate_Poll(uint32_t now_ms);

#endif
