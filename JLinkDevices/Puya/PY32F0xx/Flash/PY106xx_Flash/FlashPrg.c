/* -----------------------------------------------------------------------------
 * Copyright (c) 2014 - 2019 ARM Ltd.
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software. Permission is granted to anyone to use this
 * software for any purpose, including commercial applications, and to alter
 * it and redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software in
 *    a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 *
 *
 * $Date:        2021-7-1
 * $Revision:    V1.0.0
 *
 * Project:      Flash Programming Functions for Puya PY32F032xx Flash
 * --------------------------------------------------------------------------- */

/* History:
 *  Version 1.0.0
 *    Initial release
 */

#include "FlashOS.h" // FlashOS Structures

typedef volatile unsigned char vu8;
typedef unsigned char u8;
typedef volatile unsigned short vu16;
typedef unsigned short u16;
typedef volatile unsigned long vu32;
typedef unsigned long u32;

#define M8(adr) (*((vu8 *)(adr)))
#define M16(adr) (*((vu16 *)(adr)))
#define M32(adr) (*((vu32 *)(adr)))

#define SET_BIT(REG, BIT) ((REG) |= (BIT))
#define CLEAR_BIT(REG, BIT) ((REG) &= ~(BIT))
#define READ_BIT(REG, BIT) ((REG) & (BIT))
#define CLEAR_REG(REG) ((REG) = (0x0))
#define WRITE_REG(REG, VAL) ((REG) = (VAL))
#define READ_REG(REG) ((REG))
#define MODIFY_REG(REG, CLEARMASK, SETMASK) WRITE_REG((REG), (((READ_REG(REG)) & (~(CLEARMASK))) | (SETMASK)))

/* Peripheral Memory Map */
#define FLASH_BASE            (0x08000000UL)  /*!< FLASH base address */
#define FLASH_END             (0x0800FFFFUL)  /*!< FLASH end address */
#define FLASH_SIZE            (FLASH_END - FLASH_BASE + 1)
#define FLASH_PAGE_SIZE       0x00000080U     /*!< FLASH Page Size, 128 Bytes */
#define FLASH_PAGE_NB         (FLASH_SIZE / FLASH_PAGE_SIZE)
#define FLASH_SECTOR_SIZE     0x00001000U     /*!< FLASH Sector Size, 4096 Bytes */
#define FLASH_SECTOR_NB       (FLASH_SIZE / FLASH_SECTOR_SIZE)
#define SRAM_BASE             (0x20000000UL)  /*!< SRAM base address */
#define SRAM_END              (0x20001FFFUL)  /*!< SRAM end address */

#define RCC_BASE              0x40021000
#define WWDG_BASE             0x40002C00
#define IWDG_BASE             0x40003000
#define FLASH_R_BASE          0x40022000

#define OB_BASE               0x1FFF0F80UL       /*!< FLASH Option Bytes base address */
#define UID_BASE              0x1FFF0F00UL       /*!< Unique device ID register base address */
#define OTP_BASE              0x1FFF0E00UL

#define RCC ((RCC_TypeDef *)RCC_BASE)
#define WWDG ((WWDG_TypeDef *)WWDG_BASE)
#define IWDG ((IWDG_TypeDef *)IWDG_BASE)
#define FLASH ((FLASH_TypeDef *)FLASH_R_BASE)

typedef struct
{
    vu32 CR;    /*!< RCC Clock Sources Control Register,                                     Address offset: 0x00 */
    vu32 ICSCR; /*!< RCC Internal Clock Sources Calibration Register,                        Address offset: 0x04 */
} RCC_TypeDef;

/* Independent WATCHDOG */
typedef struct
{
    vu32 KR;  /*!< IWDG Key register,       Address offset: 0x00 */
    vu32 PR;  /*!< IWDG Prescaler register, Address offset: 0x04 */
    vu32 RLR; /*!< IWDG Reload register,    Address offset: 0x08 */
    vu32 SR;  /*!< IWDG Status register,    Address offset: 0x0C */
} IWDG_TypeDef;

/* WWDG Registers */
typedef struct
{
    vu32 CR;               /*!< WWDG CR Register,                   Address offset: 0x0  */
    vu32 CFR;              /*!< WWDG CFR Register,                  Address offset: 0x4  */
    vu32 SR;               /*!< WWDG SR Register,                   Address offset: 0x8  */
} WWDG_TypeDef;

/* Flash Registers */
typedef struct
{
    vu32 ACR;              /*!< FLASH ACR Register,                 Address offset: 0x00  */
    vu32 RESERVED1;
    vu32 KEYR;             /*!< FLASH KEYR Register,                Address offset: 0x08  */
    vu32 OPTKEYR;          /*!< FLASH OPTKEYR Register,             Address offset: 0x0C  */
    vu32 SR;               /*!< FLASH SR Register,                  Address offset: 0x10  */
    vu32 CR;               /*!< FLASH CR Register,                  Address offset: 0x14  */
    vu32 RESERVED2[2];
    vu32 OPTR;             /*!< FLASH OPTR Register,                Address offset: 0x20  */
    vu32 RESERVED3;
    vu32 WRPR;             /*!< FLASH WRPR Register,                Address offset: 0x28  */
    vu32 PCROPR;           /*!< FLASH PCROPR Register,              Address offset: 0x2C  */
    vu32 SECR;             /*!< FLASH SECR Register,                Address offset: 0x30  */
    vu32 RESERVED5[23];
    vu32 LPCR;             /*!< FLASH LPCR Register,                Address offset: 0x90  */
    vu32 RESERVED6[27];
    vu32 TS0;              /*!< FLASH TS0 Register,                 Address offset: 0x100  */
    vu32 TS1;              /*!< FLASH TS1 Register,                 Address offset: 0x104  */
    vu32 TS2P;             /*!< FLASH TS2P Register,                Address offset: 0x108  */
    vu32 TPS3;             /*!< FLASH TPS3 Register,                Address offset: 0x10C  */
    vu32 TS3;              /*!< FLASH TS3 Register,                 Address offset: 0x110  */
    vu32 ERSTPE;           /*!< FLASH ERSTPE Register,              Address offset: 0x114  */
    vu32 PRGTPE;           /*!< FLASH PRGTPE Register,              Address offset: 0x118  */
    vu32 PRETPE;           /*!< FLASH PRETPE Register,              Address offset: 0x11C  */
    vu32 ACLK2PW;          /*!< FLASH ACLK2PW Register,             Address offset: 0x120  */
} FLASH_TypeDef;

/********************************* Bit definition for FLASH_ACR register ********************************************/
#define FLASH_ACR_LATENCY_Pos                     (0U)
#define FLASH_ACR_LATENCY_Msk                     (0x3UL<<FLASH_ACR_LATENCY_Pos)                    /*!< 0x00000003 */
#define FLASH_ACR_LATENCY                         FLASH_ACR_LATENCY_Msk                             /*!< Latency */

/********************************* Bit definition for FLASH_KEYR register *******************************************/
#define FLASH_KEY1_Pos                            (0U)
#define FLASH_KEY1_Msk                            (0x45670123UL << FLASH_KEY1_Pos)                  /*!< 0x45670123 */
#define FLASH_KEY1                                FLASH_KEY1_Msk                                    /*!< Flash program erase key1 */
#define FLASH_KEY2_Pos                            (0U)
#define FLASH_KEY2_Msk                            (0xCDEF89ABUL << FLASH_KEY2_Pos)                  /*!< 0xCDEF89AB */
#define FLASH_KEY2                                FLASH_KEY2_Msk                                    /*!< Flash program erase key2: used with FLASH_PEKEY1
                                                                                                         to unlock the write access to the FPEC. */
/********************************* Bit definition for FLASH_OPTKEYR register ****************************************/
#define FLASH_OPTKEY1_Pos                         (0U)
#define FLASH_OPTKEY1_Msk                         (0x08192A3BUL << FLASH_OPTKEY1_Pos)               /*!< 0x08192A3B */
#define FLASH_OPTKEY1                             FLASH_OPTKEY1_Msk                                 /*!< Flash option key1 */
#define FLASH_OPTKEY2_Pos                         (0U)
#define FLASH_OPTKEY2_Msk                         (0x4C5D6E7FUL << FLASH_OPTKEY2_Pos)               /*!< 0x4C5D6E7F */
#define FLASH_OPTKEY2                             FLASH_OPTKEY2_Msk                                 /*!< Flash option key2: used with FLASH_OPTKEY1 to
                                                                                                         unlock the write access to the option byte block */

/********************************* Bit definition for FLASH_SR register *********************************************/
#define FLASH_SR_EOP_Pos                          (0U)
#define FLASH_SR_EOP_Msk                          (0x1UL<<FLASH_SR_EOP_Pos)                         /*!< 0x00000001 */
#define FLASH_SR_EOP                              FLASH_SR_EOP_Msk                                  /*!< End of operation */
#define FLASH_SR_WRPERR_Pos                       (4U)
#define FLASH_SR_WRPERR_Msk                       (0x1UL<<FLASH_SR_WRPERR_Pos)                      /*!< 0x00000010 */
#define FLASH_SR_WRPERR                           FLASH_SR_WRPERR_Msk                               /*!< Write protection error */
#define FLASH_SR_RDERR_Pos                        (11U)
#define FLASH_SR_RDERR_Msk                        (0x1UL<<FLASH_SR_RDERR_Pos)                       /*!< 0x00000800 */
#define FLASH_SR_RDERR                            FLASH_SR_RDERR_Msk                                /*!< pcrop read error */
#define FLASH_SR_OPTVERR_Pos                      (15U)
#define FLASH_SR_OPTVERR_Msk                      (0x1UL<<FLASH_SR_OPTVERR_Pos)                     /*!< 0x00008000 */
#define FLASH_SR_OPTVERR                          FLASH_SR_OPTVERR_Msk                              /*!< Option and trimming bits loading validity error */
#define FLASH_SR_BSY_Pos                          (16U)
#define FLASH_SR_BSY_Msk                          (0x1UL<<FLASH_SR_BSY_Pos)                         /*!< 0x00010000 */
#define FLASH_SR_BSY                              FLASH_SR_BSY_Msk                                  /*!< Busy */
#define FLASH_SR_USERLOCK_Pos                     (28U)
#define FLASH_SR_USERLOCK_Msk                     (0x1UL<<FLASH_SR_USERLOCK_Pos)                    /*!< 0x10000000 */
#define FLASH_SR_USERLOCK                         FLASH_SR_USERLOCK_Msk

/********************************* Bit definition for FLASH_CR register *********************************************/
#define FLASH_CR_PG_Pos                           (0U)
#define FLASH_CR_PG_Msk                           (0x1UL<<FLASH_CR_PG_Pos)                          /*!< 0x00000001 */
#define FLASH_CR_PG                               FLASH_CR_PG_Msk                                   /*!< Page Program */
#define FLASH_CR_PER_Pos                          (1U)
#define FLASH_CR_PER_Msk                          (0x1UL<<FLASH_CR_PER_Pos)                         /*!< 0x00000002 */
#define FLASH_CR_PER                              FLASH_CR_PER_Msk                                  /*!< Page Erase */
#define FLASH_CR_MER_Pos                          (2U)
#define FLASH_CR_MER_Msk                          (0x1UL<<FLASH_CR_MER_Pos)                         /*!< 0x00000004 */
#define FLASH_CR_MER                              FLASH_CR_MER_Msk                                  /*!< Mass Erase */
#define FLASH_CR_UPG_Pos                          (4U)
#define FLASH_CR_UPG_Msk                          (0x1UL<<FLASH_CR_UPG_Pos)                         /*!< 0x00000010 */
#define FLASH_CR_UPG                              FLASH_CR_UPG_Msk                                  /*!< Userdata Program */
#define FLASH_CR_UPER_Pos                         (5U)
#define FLASH_CR_UPER_Msk                         (0x1UL<<FLASH_CR_UPER_Pos)                        /*!< 0x00000020 */
#define FLASH_CR_UPER                             FLASH_CR_UPER_Msk                                 /*!< Userdata Page Erase */
#define FLASH_CR_SER_Pos                          (11U)
#define FLASH_CR_SER_Msk                          (0x1UL<<FLASH_CR_SER_Pos)                         /*!< 0x00000800 */
#define FLASH_CR_SER                              FLASH_CR_SER_Msk                                  /*!< Sector Erase */
#define FLASH_CR_OPTSTRT_Pos                      (17U)
#define FLASH_CR_OPTSTRT_Msk                      (0x1UL<<FLASH_CR_OPTSTRT_Pos)                     /*!< 0x00020000 */
#define FLASH_CR_OPTSTRT                          FLASH_CR_OPTSTRT_Msk                              /*!< Option bytes Programming Start */
#define FLASH_CR_UPGSTRT_Pos                      (18U)
#define FLASH_CR_UPGSTRT_Msk                      (0x1UL<<FLASH_CR_UPGSTRT_Pos)                     /*!< 0x00040000 */
#define FLASH_CR_UPGSTRT                          FLASH_CR_UPGSTRT_Msk                              /*!< Userdata Programming Start */
#define FLASH_CR_PGSTRT_Pos                       (19U)
#define FLASH_CR_PGSTRT_Msk                       (0x1UL<<FLASH_CR_PGSTRT_Pos)                      /*!< 0x00080000 */
#define FLASH_CR_PGSTRT                           FLASH_CR_PGSTRT_Msk                               /*!< Programming Start */
#define FLASH_CR_RDERRIE_Pos                      (22U)
#define FLASH_CR_RDERRIE_Msk                      (0x1UL<<FLASH_CR_RDERRIE_Pos)                     /*!< 0x00400000 */
#define FLASH_CR_RDERRIE                          FLASH_CR_RDERRIE_Msk                              /*!< pcrop read error interrupt enable */
#define FLASH_CR_EOPIE_Pos                        (24U)
#define FLASH_CR_EOPIE_Msk                        (0x1UL<<FLASH_CR_EOPIE_Pos)                       /*!< 0x01000000 */
#define FLASH_CR_EOPIE                            FLASH_CR_EOPIE_Msk                                /*!< End of operation interrupt enable */
#define FLASH_CR_ERRIE_Pos                        (25U)
#define FLASH_CR_ERRIE_Msk                        (0x1UL<<FLASH_CR_ERRIE_Pos)                       /*!< 0x02000000 */
#define FLASH_CR_ERRIE                            FLASH_CR_ERRIE_Msk                                /*!< Error interrupt enable */
#define FLASH_CR_OBL_LAUNCH_Pos                   (27U)
#define FLASH_CR_OBL_LAUNCH_Msk                   (0x1UL<<FLASH_CR_OBL_LAUNCH_Pos)                  /*!< 0x08000000 */
#define FLASH_CR_OBL_LAUNCH                       FLASH_CR_OBL_LAUNCH_Msk                           /*!< Force the option bytes loading */
#define FLASH_CR_SECPROT_Pos                      (28U)
#define FLASH_CR_SECPROT_Msk                      (0x1UL<<FLASH_CR_SECPROT_Pos)                     /*!< 0x08000000 */
#define FLASH_CR_SECPROT                          FLASH_CR_SECPROT_Msk
#define FLASH_CR_OPTLOCK_Pos                      (30U)
#define FLASH_CR_OPTLOCK_Msk                      (0x1UL<<FLASH_CR_OPTLOCK_Pos)                     /*!< 0x40000000 */
#define FLASH_CR_OPTLOCK                          FLASH_CR_OPTLOCK_Msk                              /*!< Option Lock */
#define FLASH_CR_LOCK_Pos                         (31U)
#define FLASH_CR_LOCK_Msk                         (0x1UL<<FLASH_CR_LOCK_Pos)                        /*!< 0x80000000 */
#define FLASH_CR_LOCK                             FLASH_CR_LOCK_Msk                                 /*!< Lock */

#define FLASH_OPTR_IWDG_STOP_Pos                  (13U)
#define FLASH_OPTR_IWDG_STOP_Msk                  (0x3UL<<FLASH_OPTR_IWDG_STOP_Pos)                 /*!< 0x00000800 */
#define FLASH_OPTR_IWDG_STOP                      FLASH_OPTR_IWDG_STOP_Msk                          /*!< NRST mode Count Control */
#define FLASH_OPTR_IWDG_SW_Pos                    (14U)
#define FLASH_OPTR_IWDG_SW_Msk                    (0x1UL<<FLASH_OPTR_IWDG_SW_Pos)                   /*!< 0x00001000 */
#define FLASH_OPTR_IWDG_SW                        FLASH_OPTR_IWDG_SW_Msk                            /*!< IWDG Software Enable */
#define FLASH_OPTR_WWDG_SW_Pos                    (15U)
#define FLASH_OPTR_WWDG_SW_Msk                    (0x1UL<<FLASH_OPTR_WWDG_SW_Pos)                   /*!< 0x00002000 */
#define FLASH_OPTR_WWDG_SW                        FLASH_OPTR_WWDG_SW_Msk                            /*!< WWDG Software Enable */

/********************************* Bit definition for FLASH_WRPR0 register ******************************************/
#define FLASH_WRPR_BK_WRPX_Pos                   (0U)
#define FLASH_WRPR_BK_WRPX_Msk                   (0xFFFFUL<<FLASH_WRPR_BK_WRPX_Pos)                 /*!< 0x0000FFFF */
#define FLASH_WRPR_BK_WRPX                       FLASH_WRPR_BK_WRPX_Msk                             /*!< Sector Write Protection */

#define WWDG_CR_T_Pos           (0U)
#define WWDG_CR_T_Msk           (0x7FUL << WWDG_CR_T_Pos)                      /*!< 0x0000007F */
#define WWDG_CR_T               WWDG_CR_T_Msk                                  /*!<T[6:0] bits (7-Bit counter (MSB to LSB)) */

#define WWDG_CFR_WDGTB_Pos                        (7U)
#define WWDG_CFR_WDGTB_Msk                        (0x3UL<<WWDG_CFR_WDGTB_Pos)                       /*!< 0x00000180 */
#define WWDG_CFR_WDGTB                            WWDG_CFR_WDGTB_Msk                                /*!< time base bit 1 */


u32 RCC_ICSCR_HSI_FS_RESTORE;
int ErasePage(unsigned long adr);

/*
 *  Initialize Flash Programming Functions
 *    Parameter:      adr:  Device Base Address
 *                    clk:  Clock Frequency (Hz)
 *                    fnc:  Function Code (1 - Erase, 2 - Program, 3 - Verify)
 *    Return Value:   0 - OK,  1 - Failed
 */

int Init(unsigned long adr, unsigned long clk, unsigned long fnc)
{
    FLASH->KEYR = FLASH_KEY1; /* Unlock Flash */
    FLASH->KEYR = FLASH_KEY2;

#ifdef FLASH_OB
    FLASH->OPTKEYR = FLASH_OPTKEY1; /* Unlock Option Bytes */
    FLASH->OPTKEYR = FLASH_OPTKEY2;
#endif

    FLASH->ACR &= ~FLASH_ACR_LATENCY;   /* Zero Wait State, no Prefetch */
    FLASH->SR |= FLASH_SR_EOP; /* Reset FLASH_EOP */
    while (FLASH->SR & FLASH_SR_BSY);                     /* Check FLASH_SR_BSY */

    if ((FLASH->OPTR & FLASH_OPTR_IWDG_SW) == 0x00) /* Test if IWDG is running (IWDG in HW mode) */
    {
        /* Set IWDG time out to ~32.768 second */
        IWDG->KR = 0xAAAA;                                  /* Reload IWDG */
        IWDG->KR = 0x5555; /* Enable write access to IWDG_PR and IWDG_RLR */
        IWDG->PR = 0x06;   /* Set prescaler to 256 */
        IWDG->RLR = 0xFFF; /* Set reload value to 4095 */
    }
    if (FLASH_OPTR_WWDG_SW != READ_BIT(FLASH->OPTR, FLASH_OPTR_WWDG_SW)) /* Test if WWDG is running (WWDG in HW mode) */
    {
        /* Set WWDG time out to maximum */
        SET_BIT(WWDG->CFR, WWDG_CFR_WDGTB);               /* Set prescaler to maximum */
        SET_BIT(WWDG->CR, WWDG_CR_T);                     /* Reload WWDG */
    }
    return (0);
}


/*
 *  De-Initialize Flash Programming Functions
 *    Parameter:      fnc:  Function Code (1 - Erase, 2 - Program, 3 - Verify)
 *    Return Value:   0 - OK,  1 - Failed
 */

int UnInit(unsigned long fnc)
{
#ifdef FLASH_OB
    FLASH->CR |= FLASH_CR_OPTLOCK; /* Lock Option Bytes */
#endif
    FLASH->CR |= FLASH_CR_LOCK;    /* Lock Flash */

    return (0);
}


/*
 *  Erase complete Flash Memory
 *    Return Value:   0 - OK,  1 - Failed
 */


int EraseChip(void)
{
#ifdef FLASH_OTP
    ErasePage(OTP_BASE);
#endif    /* FLASH_OTP */

#ifdef FLASH_MEM
    FLASH->SR |= FLASH_SR_EOP; /* Reset FLASH_EOP */

    FLASH->CR |= FLASH_CR_MER; /* Mass Erase Enabled */
    FLASH->CR |= FLASH_CR_EOPIE;
    M32(FLASH_BASE) = 0xFF;
    __asm("DSB");

    while (FLASH->SR & FLASH_SR_BSY)
    {
        SET_BIT(WWDG->CR, WWDG_CR_T);     /* Reload WWDG */
        IWDG->KR = 0xAAAA; /* Reload IWDG */
    }

    if (FLASH_SR_EOP != (FLASH->SR & FLASH_SR_EOP))       /* Check for FLASH_SR_EOP */
    {
        FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
        return (1);                                       /* Failed */
    }
    FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
    FLASH->CR &= ~FLASH_CR_MER;   /* Mass Erase Disabled */
    FLASH->CR &= ~FLASH_CR_EOPIE; /* Reset FLASH_EOPIE */
#endif
    return (0); /* Done */
}

/*
 *  Erase Sector in Flash Memory
 *    Parameter:      adr:  Sector Address
 *    Return Value:   0 - OK,  1 - Failed
 */

#ifdef FLASH_MEM
int EraseSector(unsigned long adr)
{
    FLASH->SR |= FLASH_SR_EOP;          /* Reset FLASH_EOP */

    FLASH->CR |= FLASH_CR_SER;          /* Sector Erase Enabled */
    FLASH->CR |= FLASH_CR_EOPIE;
    M32(adr) = 0xFF;                    /* Sector Address */
    __asm("DSB");

    while (FLASH->SR & FLASH_SR_BSY)
    {
        SET_BIT(WWDG->CR, WWDG_CR_T);     /* Reload WWDG */
        IWDG->KR = 0xAAAA;                /* Reload IWDG */
    }

    if (FLASH_SR_EOP != (FLASH->SR & FLASH_SR_EOP))       /* Check for FLASH_SR_EOP */
    {
        FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
        return (1);                                       /* Failed */
    }
    FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
    FLASH->CR &= ~FLASH_CR_SER;           /* Sector Erase Disabled */
    FLASH->CR &= ~FLASH_CR_EOPIE;         /* Reset FLASH_EOPIE */

    return (0); /* Done */
}
#endif

#ifdef FLASH_OTP
int EraseSector(unsigned long adr)
{
    ErasePage(OTP_BASE);

    return (0); /* Done */
}
#endif /* FLASH_OTP */


#ifdef FLASH_OB
int EraseSector(unsigned long adr)
{
    /* erase sector is not needed for
       - Flash Option bytes
       - Flash One Time Programmable bytes
    */
    return (0); /* Done */
}
#endif

#ifdef FLASH_OTP
int ErasePage(unsigned long adr)
{
    FLASH->SR |= FLASH_SR_EOP;
    FLASH->CR |= FLASH_CR_UPER;
    FLASH->CR |= FLASH_CR_EOPIE;
    M32(adr) = 0xFF;
    __asm("DSB");

    while (FLASH->SR & FLASH_SR_BSY)
    {
        SET_BIT(WWDG->CR, WWDG_CR_T);     /* Reload WWDG */
        IWDG->KR = 0xAAAA;                /* Reload IWDG */
    }

    if (FLASH_SR_EOP != (FLASH->SR & FLASH_SR_EOP))       /* Check for FLASH_SR_EOP */
    {
        FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
        return (1);                                       /* Failed */
    }
    
    FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
    FLASH->CR &= ~FLASH_CR_UPER;
    FLASH->CR &= ~FLASH_CR_EOPIE;

    return (0); /* Done */
}
#endif /* FLASH_OTP */

/*
 *  Blank Check Checks if Memory is Blank
 *    Parameter:      adr:  Block Start Address
 *                    sz:   Block Size (in bytes)
 *                    pat:  Block Pattern
 *    Return Value:   0 - OK,  1 - Failed
 */

int BlankCheck(unsigned long adr, unsigned long sz, unsigned char pat)
{
    return (1); /* Always Force Erase */
}

/*
 *  Program Page in Flash Memory
 *    Parameter:      adr:  Page Start Address
 *                    sz:   Page Size
 *                    buf:  Page Data
 *    Return Value:   0 - OK,  1 - Failed
 */

#ifdef FLASH_MEM
int ProgramPage(unsigned long adr, unsigned long sz, unsigned char *buf)
{
    sz = (sz + (FLASH_PAGE_SIZE - 1)) & ~(FLASH_PAGE_SIZE - 1); /* Adjust size for 32 Words */

    FLASH->SR |= FLASH_SR_EOP; /* Reset FLASH_EOP */

    while (sz)
    {
        FLASH->CR |= FLASH_CR_PG; /* Programming Enabled */
        FLASH->CR |= FLASH_CR_EOPIE;

        for (u8 i = 0; i < (FLASH_PAGE_SIZE / 4); i++)
        {

            M32(adr + i * 4) = *((u32 *)(buf + i * 4)); /* Program the first word of the Double Word */

            if (i == (FLASH_PAGE_SIZE / 4 - 2))
            {
                FLASH->CR |= FLASH_CR_PGSTRT;
            }
        }
        __asm("DSB");

        while (FLASH->SR & FLASH_SR_BSY)
        {
            SET_BIT(WWDG->CR, WWDG_CR_T);       /* Reload WWDG */
            IWDG->KR = 0xAAAA;                  /* Reload IWDG */
        }

        if (FLASH_SR_EOP != (FLASH->SR & FLASH_SR_EOP))       /* Check for FLASH_SR_EOP */
        {
            FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
            return (1);                                       /* Failed */
        }
        FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
        FLASH->CR &= ~FLASH_CR_PG;    /* Programming Disabled */
        FLASH->CR &= ~FLASH_CR_EOPIE; /* Reset FLASH_EOPIE */

        adr += FLASH_PAGE_SIZE; /* Go to next Page */
        buf += FLASH_PAGE_SIZE;
        sz -= FLASH_PAGE_SIZE;
    }

    return (0); /* Done */
}
#endif /* FLASH_MEM */

#ifdef FLASH_OTP
int ProgramPage(unsigned long adr, unsigned long sz, unsigned char *buf)
{
    sz = (sz + (FLASH_PAGE_SIZE - 1)) & ~(FLASH_PAGE_SIZE - 1); /* Adjust size for 32 Words */

    SET_BIT(FLASH->SR, FLASH_SR_EOP); /* Clear EOP flag */

    while (sz)
    {
        FLASH->CR |= FLASH_CR_UPG; /* Programming Enabled */

        for (u8 i = 0; i < FLASH_PAGE_SIZE / 4; i++)
        {

            M32(adr + i * 4) = *((u32 *)(buf + i * 4)); /* Program the first word of the Double Word */

            if (i == (FLASH_PAGE_SIZE / 4 - 2))
            {
                FLASH->CR |= FLASH_CR_PGSTRT;
            }
        }

        while (FLASH->SR & FLASH_SR_BSY)
        {
            SET_BIT(WWDG->CR, WWDG_CR_T);       /* Reload WWDG */
            IWDG->KR = 0xAAAA;                  /* Reload IWDG */
        }

        if (FLASH_SR_EOP != (FLASH->SR & FLASH_SR_EOP))       /* Check for FLASH_SR_EOP */
        {
            FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
            return (1);                                       /* Failed */
        }
        FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
        FLASH->CR &= ~FLASH_CR_UPG; /* Programming Disabled */


        adr += FLASH_PAGE_SIZE; /* Go to next Page */
        buf += FLASH_PAGE_SIZE;
        sz -= FLASH_PAGE_SIZE;
    }

    return (0); /* Done */
}
#endif /* FLASH_OTP */

#ifdef FLASH_OB
int ProgramPage(unsigned long adr, unsigned long sz, unsigned char *buf)
{
    u32 optr0;
    u32 optr1;
    u32 wrpr;
    u32 pcropsr;
    u32 pcroper;
    u32 secsize;

    optr0 = *((u32 *)(buf + 0x00));
    optr1 = *((u32 *)(buf + 0x04));
    wrpr = *((u32 *)(buf + 0x10));
    pcropsr = *((u32 *)(buf + 0x14));
    pcroper = *((u32 *)(buf + 0x18));
    secsize = *((u32 *)(buf + 0x1C));

    FLASH->SR |= FLASH_SR_EOP;    /* Reset FLASH_EOP*/

    FLASH->OPTR = ((optr0 & 0x0000FFFF) | ((optr1 & 0x0000FFFF) << 16));   /*Write OPTR values*/
    FLASH->WRPR = (wrpr & 0x0000FFFF);                                      /* Write WRPR values*/
    FLASH->PCROPR = ((pcropsr & 0x0000FFFF) | ((pcroper & 0x0000FFFF) << 16)); /*Write PCROPR values*/
    FLASH->SECR = (secsize & 0x0000FFFF);                                   /* Write SECR values*/

    FLASH->CR |= FLASH_CR_OPTSTRT;
    FLASH->CR |= FLASH_CR_EOPIE;
    M32(OB_BASE) = 0xFF;
    __asm("DSB");

    while (FLASH->SR & FLASH_SR_BSY)
    {
        SET_BIT(WWDG->CR, WWDG_CR_T);       /* Reload WWDG */
        IWDG->KR = 0xAAAA;                  /* Reload IWDG */
    }

    if (FLASH_SR_EOP != (FLASH->SR & FLASH_SR_EOP))       /* Check for FLASH_SR_EOP */
    {
        FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
        return (1);                                       /* Failed */
    }
    FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
    FLASH->CR &= ~FLASH_CR_OPTSTRT;       /* Programming Disabled */
    FLASH->CR &= ~FLASH_CR_EOPIE;         /* Reset FLASH_EOPIE */

    return (0); /* Done */
}
#endif /* FLASH_OB */

#ifdef FLASH_OB
unsigned long Verify(unsigned long adr, unsigned long sz, unsigned char *buf)
{
    u32 optr0;
    u32 optr1;
    u32 wrpr;
    u32 pcropsr;
    u32 pcroper;
    u32 secsize;

    optr0 = *((u32 *)(buf + 0x00));
    optr1 = *((u32 *)(buf + 0x04));
    wrpr = *((u32 *)(buf + 0x10));
    pcropsr = *((u32 *)(buf + 0x14));
    pcroper = *((u32 *)(buf + 0x18));
    secsize = *((u32 *)(buf + 0x1C));

    if (M32(adr + 0x00) != optr0)
    {
        return (adr + 0x00);
    }
    if (M32(adr + 0x04) != optr1)
    {
        return (adr + 0x04);
    }
    if (M32(adr + 0x10) != wrpr)
    {
        return (adr + 0x10);
    }
    if (M32(adr + 0x14) != pcropsr)
    {
        return (adr + 0x14);
    }
    if (M32(adr + 0x18) != pcroper)
    {
        return (adr + 0x18);
    }
    if (M32(adr + 0x1C) != secsize)
    {
        return (adr + 0x1C);
    }
    
    return (adr + sz);
}
#endif /*  FLASH_OB */


#ifdef FLASH_MEM
int ErasePage(unsigned long adr)
{
    FLASH->SR |= FLASH_SR_EOP; /* Reset FLASH_EOP */

    FLASH->CR |= FLASH_CR_PER; /* page Erase Enabled */
    FLASH->CR |= FLASH_CR_EOPIE;
    M32(adr) = 0xFF; /* Sector Address */
    __asm("DSB");

    while (FLASH->SR & FLASH_SR_BSY)
    {
        SET_BIT(WWDG->CR, WWDG_CR_T);       /* Reload WWDG */
        IWDG->KR = 0xAAAA;                  /* Reload IWDG */
    }

    if (FLASH_SR_EOP != (FLASH->SR & FLASH_SR_EOP))       /* Check for FLASH_SR_EOP */
    {
        FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
        return (1);                                       /* Failed */
    }
    FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
    FLASH->CR &= ~FLASH_CR_PER;   /* page Erase Disabled */
    FLASH->CR &= ~FLASH_CR_EOPIE; /* Reset FLASH_EOPIE */

    return (0); /* Done */
}
#endif /* FLASH_MEM */


