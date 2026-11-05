#include "NuMicro.h"
#include "secure_update.h"

static uint32_t s_pressed_since;
static uint8_t s_pressed;
static uint8_t s_handled;
static volatile uint32_t s_requested;

void SecureUpdate_Request(void)
{
    s_requested = 1UL;
}

static uint32_t SecureUpdate_TakeRequest(void)
{
    uint32_t primask;
    uint32_t requested;

    primask = __get_PRIMASK();
    __disable_irq();
    requested = s_requested;
    s_requested = 0UL;
    __set_PRIMASK(primask);
    return requested;
}

/* The scatter file keeps all executable APROM code above the remapped page. */
__attribute__((noinline)) static void SecureUpdate_Transfer(void)
{
    uint32_t i;
    uint32_t irq_banks;

    __disable_irq();
    SysTick->CTRL = 0UL;
    TIMER_Stop(TIMER1);
    TIMER_DisableInt(TIMER1);
    UART0->INTEN = 0UL;

    irq_banks = (SCnSCB->ICTR & SCnSCB_ICTR_INTLINESNUM_Msk) + 1UL;
    for (i = 0UL; i < irq_banks; i++)
    {
        NVIC->ICER[i] = 0xFFFFFFFFUL;
        NVIC->ICPR[i] = 0xFFFFFFFFUL;
    }
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk | SCB_ICSR_PENDSVCLR_Msk;

    SYS_UnlockReg();
    CLK->AHBCLK |= CLK_AHBCLK_ISPCKEN_Msk;
    FMC_Open();

    if ((FMC_SetVectorPageAddr(FMC_LDROM_BASE) != 0) ||
        (FMC_GetVECMAP() != FMC_LDROM_BASE))
    {
        /* Restore the configured secure boot flow if remapping fails. */
        SYS_ResetChip();
        while (1)
        {
        }
    }

    __DSB();
    __ISB();
    /* CPU reset loads the LDROM MSP and reset vector and preserves VECMAP. */
    SYS_ResetCPU();
    while (1)
    {
    }
}

static int32_t SecureUpdate_Enter(void)
{
    uint32_t status;
    uint32_t stack;
    uint32_t reset;

    status = FMC->ISPSTS;
    /* Mask ROM boot enables IAP; direct APROM boot requires CBS[0] == 0. */
    if (((status & FMC_ISPSTS_MBS_Msk) != 0UL) &&
        ((status & (1UL << FMC_ISPSTS_CBS_Pos)) != 0UL))
    {
        return SECURE_UPDATE_ERROR_IAP;
    }

    stack = *(volatile uint32_t *)FMC_LDROM_BASE;
    reset = *(volatile uint32_t *)(FMC_LDROM_BASE + 4UL);
    if (((stack & 7UL) != 0UL) || (stack <= 0x20000000UL) ||
        (stack > 0x2000C000UL) || ((reset & 1UL) == 0UL) ||
        ((reset & ~1UL) < (FMC_LDROM_BASE + 0x200UL)) ||
        ((reset & ~1UL) >= (FMC_LDROM_BASE + FMC_LDROM_SIZE)))
    {
        return SECURE_UPDATE_ERROR_LOADER;
    }

    SecureUpdate_Transfer();
    return SECURE_UPDATE_OK;
}

void SecureUpdate_Init(void)
{
    uint32_t locked;

    locked = SYS_IsRegLocked();
    SYS_UnlockReg();
    CLK->AHBCLK |= CLK_AHBCLK_ISPCKEN_Msk;
    SYS->GPG_MFPH = (SYS->GPG_MFPH & ~SYS_GPG_MFPH_PG15MFP_Msk) |
                    SYS_GPG_MFPH_PG15MFP_GPIO;
    GPIO_SetMode(PG, BIT15, GPIO_MODE_INPUT);
    GPIO_SetPullCtl(PG, BIT15, GPIO_PUSEL_PULL_UP);
    PG->DINOFF &= ~(BIT15 << 16);
    if (locked != 0UL)
    {
        SYS_LockReg();
    }

    s_pressed_since = 0UL;
    s_pressed = 0U;
    s_handled = 0U;
    s_requested = 0UL;
}

int32_t SecureUpdate_Poll(uint32_t now_ms)
{
    if (SecureUpdate_TakeRequest() != 0UL)
    {
        return SecureUpdate_Enter();
    }

    if (PG15 != 0UL)
    {
        s_pressed = 0U;
        s_handled = 0U;
        return SECURE_UPDATE_OK;
    }

    if (s_pressed == 0U)
    {
        s_pressed = 1U;
        s_pressed_since = now_ms;
    }

    if ((s_handled == 0U) &&
        ((uint32_t)(now_ms - s_pressed_since) >= SECURE_UPDATE_DEBOUNCE_MS))
    {
        s_handled = 1U;
        return SecureUpdate_Enter();
    }
    return SECURE_UPDATE_OK;
}
