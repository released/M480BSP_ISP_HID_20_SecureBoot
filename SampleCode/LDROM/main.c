#include "NuMicro.h"
#include "BootIspLib.h"

static void Loader_Stop(void)
{
    __disable_irq();
    while (1)
    {
        __WFI();
    }
}

/* Supply the startup hook without linking the UART retarget implementation. */
uint32_t ProcessHardFault(uint32_t lr, uint32_t msp, uint32_t psp)
{
    (void)lr;
    (void)msp;
    (void)psp;
    Loader_Stop();
    return 0UL;
}

/* The shared startup references this hook even when semihosting is unused. */
int32_t SH_Return(int32_t in_r0, int32_t in_r1, int32_t *out_r0)
{
    (void)in_r0;
    (void)in_r1;
    (void)out_r0;
    return 0;
}

void USBD20_IRQHandler(void)
{
    HSUSBD_ISR();
}

static void Loader_Init(void)
{
    volatile uint32_t delay;

    SYS_UnlockReg();
    PF->MODE &= ~(GPIO_MODE_MODE2_Msk | GPIO_MODE_MODE3_Msk);
    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
    if (CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk) == 0UL)
    {
        Loader_Stop();
    }
    if (CLK_SetCoreClock(FREQ_192MHZ) != FREQ_192MHZ)
    {
        Loader_Stop();
    }
    CLK->PCLKDIV = CLK_PCLKDIV_APB0DIV_DIV2 | CLK_PCLKDIV_APB1DIV_DIV2;

    CLK_EnableModuleClock(HSUSBD_MODULE);
    SYS_ResetModule(HSUSBD_RST);
    SYS->USBPHY = (SYS->USBPHY &
                   ~(SYS_USBPHY_HSUSBROLE_Msk | SYS_USBPHY_HSUSBACT_Msk)) |
                  SYS_USBPHY_HSUSBEN_Msk;
    for (delay = 0UL; delay < 0x1000UL; delay++)
    {
    }
    SYS->USBPHY |= SYS_USBPHY_HSUSBACT_Msk;

    SCB->VTOR = FMC_LDROM_BASE;
    NVIC_ClearPendingIRQ(USBD20_IRQn);
    __DSB();
    __ISB();
}

int main(void)
{
    Loader_Init();
    __set_BASEPRI(0UL);
    __set_FAULTMASK(0UL);
    __enable_irq();
    HSUSBD_ISP();

    /* Follow the BSP flow: reset the board after the host reports success. */
    while (1)
    {
        __WFI();
    }
}
