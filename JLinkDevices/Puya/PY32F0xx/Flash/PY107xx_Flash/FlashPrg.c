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
 * Project:      Flash Programming Functions for Puya PY32F002Cxx Flash
 * --------------------------------------------------------------------------- */

/* History:
 *  Version 1.0.0
 *    Initial release
 */

#include "FlashOS.h" /* FlashOS Structures */

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

#define FLASH_BASE            (0x08000000UL)  /*!< FLASH base address */
#define FLASH_END             (0x08003FFFUL)  /*!< FLASH end address */
#define FLASH_SIZE            (FLASH_END - FLASH_BASE + 1)
#define FLASH_PAGE_SIZE       0x00000040U     /*!< FLASH Page Size, 64 Bytes */
#define FLASH_PAGE_NB         (FLASH_SIZE / FLASH_PAGE_SIZE)
#define FLASH_SECTOR_SIZE     0x00001000U     /*!< FLASH Sector Size, 4096 Bytes */
#define FLASH_SECTOR_NB       (FLASH_SIZE / FLASH_SECTOR_SIZE)

/* Peripheral Memory Map */
#define OTP_BASE              0x1FFF01C0
#define RCC_BASE              0x40021000
#define OB_BASE               0x1FFF0040                 /*!< FLASH Option Bytes base address */
#define IWDG_BASE             0x40003000
#define FLASH_R_BASE          0x40022000
#define FLASHSIZE_BASE        (0x1FFF01FCUL)                  /*!< FLASH Size register base address */

#define RCC                 ((RCC_TypeDef *)RCC_BASE)
#define IWDG                ((IWDG_TypeDef *)IWDG_BASE)
#define FLASH               ((FLASH_TypeDef *)FLASH_R_BASE)
#define OB                  ((OB_TypeDef *) OB_BASE)
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

/* Flash Registers */
typedef struct
{
  vu32 RESERVED1[2];    /*!< Reserved1,                                      Address offset: 0x00 */
  vu32 KEYR;         /*!< FLASH Key register,                                Address offset: 0x08 */
  vu32 OPTKEYR;      /*!< FLASH Option Key register,                         Address offset: 0x0C */
  vu32 SR;           /*!< FLASH Status register,                             Address offset: 0x10 */
  vu32 CR;           /*!< FLASH Control register,                            Address offset: 0x14 */
  vu32 RESERVED2[2]; /*!< Reserved2,                                         Address offset: 0x18-0x1C */
  vu32 OPTR;         /*!< FLASH Option register,                             Address offset: 0x20 */
  vu32 SDKR;         /*!< FLASH SDK address register,                        Address offset: 0x24 */
  vu32 BTCR;         /*!< FLASH boot control                                 Address offset: 0x28 */
  vu32 WRPR;         /*!< FLASH WRP address register,                        Address offset: 0x2C */
  vu32 RESERVED4[(0x90 - 0x2C) / 4 - 1];
  vu32 STCR; /*!< FLASH sleep time config register,                  Address offset: 0x90 */
  vu32 RESERVED5[(0x100 - 0x90) / 4 - 1];
  vu32 TS0;     /*!< FLASH TS0 register,                                Address offset: 0x100 */
  vu32 TS1;     /*!< FLASH TS1 register,                                Address offset: 0x104 */
  vu32 TS2P;    /*!< FLASH TS2P register,                               Address offset: 0x108 */
  vu32 TPS3;    /*!< FLASH TPS3 register,                               Address offset: 0x10C */
  vu32 TS3;     /*!< FLASH TS3 register,                                Address offset: 0x110 */
  vu32 PERTPE;  /*!< FLASH PERTPE register,                             Address offset: 0x114 */
  vu32 SMERTPE; /*!< FLASH SMERTPE register,                            Address offset: 0x118 */
  vu32 PRGTPE;  /*!< FLASH PRGTPE register,                             Address offset: 0x11C */
  vu32 PRETPE;  /*!< FLASH PRETPE register,                             Address offset: 0x120 */
} FLASH_TypeDef;

/******************  FLASH Keys  **********************************************/
#define FLASH_KEY1_Pos                  (0U)
#define FLASH_KEY1_Msk                  (0x45670123UL << FLASH_KEY1_Pos)     /*!< 0x45670123 */
#define FLASH_KEY1                      FLASH_KEY1_Msk                       /*!< Flash program erase key1 */
#define FLASH_KEY2_Pos                  (0U)
#define FLASH_KEY2_Msk                  (0xCDEF89ABUL << FLASH_KEY2_Pos)     /*!< 0xCDEF89AB */
#define FLASH_KEY2                      FLASH_KEY2_Msk                       /*!< Flash program erase key2: used with FLASH_PEKEY1
                                                                              to unlock the write access to the FPEC. */

#define FLASH_OPTKEY1_Pos               (0U)
#define FLASH_OPTKEY1_Msk               (0x08192A3BUL << FLASH_OPTKEY1_Pos)  /*!< 0x08192A3B */
#define FLASH_OPTKEY1                   FLASH_OPTKEY1_Msk                    /*!< Flash option key1 */
#define FLASH_OPTKEY2_Pos               (0U)
#define FLASH_OPTKEY2_Msk               (0x4C5D6E7FUL << FLASH_OPTKEY2_Pos)  /*!< 0x4C5D6E7F */
#define FLASH_OPTKEY2                   FLASH_OPTKEY2_Msk                    /*!< Flash option key2: used with FLASH_OPTKEY1 to
                                                                              unlock the write access to the option byte block */

/* Flash Control Register definitions */
#define FLASH_CR_PG_Pos (0U)
#define FLASH_CR_PG_Msk (0x1UL << FLASH_CR_PG_Pos) /*!< 0x00000001 */
#define FLASH_CR_PG FLASH_CR_PG_Msk
#define FLASH_CR_PER_Pos (1U)
#define FLASH_CR_PER_Msk (0x1UL << FLASH_CR_PER_Pos) /*!< 0x00000002 */
#define FLASH_CR_PER FLASH_CR_PER_Msk
#define FLASH_CR_MER_Pos (2U)
#define FLASH_CR_MER_Msk (0x1UL << FLASH_CR_MER_Pos) /*!< 0x00000004 */
#define FLASH_CR_MER FLASH_CR_MER_Msk
#define FLASH_CR_SER_Pos (11U)
#define FLASH_CR_SER_Msk (0x1UL << FLASH_CR_SER_Pos) /*!< 0x00000800 */
#define FLASH_CR_SER FLASH_CR_SER_Msk
#define FLASH_CR_OPTSTRT_Pos (17U)
#define FLASH_CR_OPTSTRT_Msk (0x1UL << FLASH_CR_OPTSTRT_Pos) /*!< 0x00020000 */
#define FLASH_CR_OPTSTRT FLASH_CR_OPTSTRT_Msk
#define FLASH_CR_PGSTRT_Pos (19U)
#define FLASH_CR_PGSTRT_Msk (0x1UL << FLASH_CR_PGSTRT_Pos) /*!< 0x00080000 */
#define FLASH_CR_PGSTRT FLASH_CR_PGSTRT_Msk
#define FLASH_CR_EOPIE_Pos (24U)
#define FLASH_CR_EOPIE_Msk (0x1UL << FLASH_CR_EOPIE_Pos) /*!< 0x01000000 */
#define FLASH_CR_EOPIE FLASH_CR_EOPIE_Msk
#define FLASH_CR_ERRIE_Pos (25U)
#define FLASH_CR_ERRIE_Msk (0x1UL << FLASH_CR_ERRIE_Pos) /*!< 0x02000000 */
#define FLASH_CR_ERRIE FLASH_CR_ERRIE_Msk
#define FLASH_CR_OBL_LAUNCH_Pos (27U)
#define FLASH_CR_OBL_LAUNCH_Msk (0x1UL << FLASH_CR_OBL_LAUNCH_Pos) /*!< 0x08000000 */
#define FLASH_CR_OBL_LAUNCH FLASH_CR_OBL_LAUNCH_Msk
#define FLASH_CR_OPTLOCK_Pos (30U)
#define FLASH_CR_OPTLOCK_Msk (0x1UL << FLASH_CR_OPTLOCK_Pos) /*!< 0x40000000 */
#define FLASH_CR_OPTLOCK FLASH_CR_OPTLOCK_Msk
#define FLASH_CR_LOCK_Pos (31U)
#define FLASH_CR_LOCK_Msk (0x1UL << FLASH_CR_LOCK_Pos) /*!< 0x80000000 */
#define FLASH_CR_LOCK FLASH_CR_LOCK_Msk

/* Flash Status Register definitions */
#define FLASH_SR_EOP_Pos (0U)
#define FLASH_SR_EOP_Msk (0x1UL << FLASH_SR_EOP_Pos) /*!< 0x00000001 */
#define FLASH_SR_EOP FLASH_SR_EOP_Msk
#define FLASH_SR_WRPERR_Pos (4U)
#define FLASH_SR_WRPERR_Msk (0x1UL << FLASH_SR_WRPERR_Pos) /*!< 0x00000010 */
#define FLASH_SR_WRPERR FLASH_SR_WRPERR_Msk
#define FLASH_SR_OPTVERR_Pos (15U)
#define FLASH_SR_OPTVERR_Msk (0x1UL << FLASH_SR_OPTVERR_Pos) /*!< 0x00008000 */
#define FLASH_SR_OPTVERR FLASH_SR_OPTVERR_Msk
#define FLASH_SR_BSY_Pos (16U)
#define FLASH_SR_BSY_Msk (0x1UL << FLASH_SR_BSY_Pos) /*!< 0x00010000 */
#define FLASH_SR_BSY FLASH_SR_BSY_Msk

#define FLASH_OPTR_IWDG_SW_Pos (12U)
#define FLASH_OPTR_IWDG_SW_Msk (0x1UL << FLASH_OPTR_IWDG_SW_Pos) /*!< 0x00010000 */
#define FLASH_OPTR_IWDG_SW FLASH_OPTR_IWDG_SW_Msk
#define FLASH_OPTR_WWDG_SW_Pos (13U)
#define FLASH_OPTR_WWDG_SW_Msk (0x1UL << FLASH_OPTR_WWDG_SW_Pos) /*!< 0x00080000 */
#define FLASH_OPTR_WWDG_SW FLASH_OPTR_WWDG_SW_Msk

u32 RCC_ICSCR_HSI_FS_RESTORE;
void InitRccAndFlashParam(void);
void UnInitRccAndFlashParam(void);
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

  InitRccAndFlashParam();

#ifdef FLASH_OB
  FLASH->OPTKEYR = FLASH_OPTKEY1; /* Unlock Option Bytes */
  FLASH->OPTKEYR = FLASH_OPTKEY2;
#endif

  FLASH->SR |= FLASH_SR_EOP;                      /* Reset FLASH_EOP */
  while (FLASH->SR & FLASH_SR_BSY);                     /* Check FLASH_SR_BSY */

  if ((FLASH->OPTR & FLASH_OPTR_IWDG_SW) == 0x00) /* Test if IWDG is running (IWDG in HW mode) */
  {
    /* Set IWDG time out to ~32.768 second */
    IWDG->KR = 0xAAAA;                                  /* Reload IWDG */
    IWDG->KR = 0x5555; /* Enable write access to IWDG_PR and IWDG_RLR */
    IWDG->PR = 0x06;   /* Set prescaler to 256 */
    IWDG->RLR = 0xFFF; /* Set reload value to 4095 */
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

  UnInitRccAndFlashParam();

  FLASH->CR |= FLASH_CR_LOCK;    /* Lock Flash */

#ifdef FLASH_OB
  FLASH->CR |= FLASH_CR_OPTLOCK; /* Lock Option Bytes */
#endif
  return (0);
}


/*
 *  Erase complete Flash Memory
 *    Return Value:   0 - OK,  1 - Failed
 */

#ifdef FLASH_MEM
int EraseChip(void)
{
#ifdef FLASH_OTP
  ErasePage(OTP_BASE);
#else
  FLASH->SR |= FLASH_SR_EOP; // Reset FLASH_EOP

  FLASH->CR |= FLASH_CR_MER; // Mass Erase Enabled
  FLASH->CR |= FLASH_CR_EOPIE;
  M32(0x08000000) = 0xFF;
  __asm("DSB");

  while (FLASH->SR & FLASH_SR_BSY)
  {
    IWDG->KR = 0xAAAA; // Reload IWDG
  }

  if (FLASH_SR_EOP != (FLASH->SR & FLASH_SR_EOP))       /* Check for FLASH_SR_EOP */
  {
    FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
    return (1);                                       /* Failed */
  }
  FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
  FLASH->CR &= ~FLASH_CR_MER;   // Mass Erase Disabled
  FLASH->CR &= ~FLASH_CR_EOPIE; // Reset FLASH_EOPIE

#endif        // FLASH_OTP
  return (0); // Done
}
#endif /* FLASH_MEM */

#ifdef FLASH_OB
int EraseChip(void)
{
  /* erase chip is not needed for
     - Flash Option bytes
     - Flash One Time Programmable bytes
  */
  return (0); /* Done */
}
#endif

/*
 *  Erase Sector in Flash Memory
 *    Parameter:      adr:  Sector Address
 *    Return Value:   0 - OK,  1 - Failed
 */

#ifdef FLASH_MEM
int EraseSector(unsigned long adr)
{
#ifdef FLASH_OTP
  ErasePage(OTP_BASE);
#else
  FLASH->SR |= FLASH_SR_EOP; // Reset FLASH_EOP

  FLASH->CR |= FLASH_CR_SER; // Sector Erase Enabled
  FLASH->CR |= FLASH_CR_EOPIE;
  M32(adr) = 0xFF; // Sector Address
  __asm("DSB");

  while (FLASH->SR & FLASH_SR_BSY)
  {
    IWDG->KR = 0xAAAA; // Reload IWDG
  }

  if (FLASH_SR_EOP != (FLASH->SR & FLASH_SR_EOP))       /* Check for FLASH_SR_EOP */
  {
    FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
    return (1);                                       /* Failed */
  }
  FLASH->SR |= FLASH_SR_EOP;       /* Reset FLASH_EOP */
  FLASH->CR &= ~FLASH_CR_SER;   // Sector Erase Disabled
  FLASH->CR &= ~FLASH_CR_EOPIE; // Reset FLASH_EOPIE

#endif        /* FLASH_OTP */
  return (0); // Done
}
#endif /* FLASH_MEM */

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
    IWDG->KR = 0xAAAA; /* Reload IWDG */
  }

  if (FLASH_SR_EOP != (FLASH->SR & FLASH_SR_EOP))       /* Check for FLASH_SR_EOP */
  {
    FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
    return (1);                                       /* Failed */
  }
  FLASH->SR |= FLASH_SR_EOP;               /* Reset FLASH_EOP */
  FLASH->CR &= ~FLASH_CR_PER;   /* page Erase Disabled */
  FLASH->CR &= ~FLASH_CR_EOPIE; /* Reset FLASH_EOPIE */


  return (0); /* Done */
}
#endif /* FLASH_MEM */

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

  sz = (sz + FLASH_PAGE_SIZE - 1) & ~(FLASH_PAGE_SIZE - 1); // Adjust size for 32 Words

  FLASH->SR |= FLASH_SR_EOP; // Reset FLASH_EOP

  while (sz)
  {
    FLASH->CR |= FLASH_CR_PG; // Programming Enabled
    FLASH->CR |= FLASH_CR_EOPIE;

    for (u8 i = 0; i < FLASH_PAGE_SIZE / 4; i++)
    {

      M32(adr + i * 4) = *((u32 *)(buf + i * 4)); // Program the first word of the Double Word
      if (i == (FLASH_PAGE_SIZE / 4 - 2))
      {
        FLASH->CR |= FLASH_CR_PGSTRT;
      }
    }
    __asm("DSB");

    while (FLASH->SR & FLASH_SR_BSY)
    {
      IWDG->KR = 0xAAAA; // Reload IWDG
    }

    if (FLASH_SR_EOP != (FLASH->SR & FLASH_SR_EOP))       /* Check for FLASH_SR_EOP */
    {
      FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
      return (1);                                       /* Failed */
    }
    FLASH->SR |= FLASH_SR_EOP;                     /* Reset FLASH_EOP */
    FLASH->CR &= ~FLASH_CR_PG;                /* Programming Disabled */
    FLASH->CR &= ~FLASH_CR_EOPIE;             /* Reset FLASH_EOPIE */


    adr += FLASH_PAGE_SIZE; // Go to next Page
    buf += FLASH_PAGE_SIZE;
    sz -= FLASH_PAGE_SIZE;
  }

  return (0); // Done
}
#endif /* FLASH_MEM */

#ifdef FLASH_OB
int ProgramPage(unsigned long adr, unsigned long sz, unsigned char *buf)
{
  u32 optr;
  u32 sdkr;
  u32 wrpr;

  optr = *((u32 *)(buf + 0x00));
  sdkr = *((u32 *)(buf + 0x04));
  wrpr = *((u32 *)(buf + 0x0C));

  FLASH->SR |= FLASH_SR_EOP; /* Reset FLASH_EOP */

  FLASH->OPTR = (optr & 0x0000FFFF); /* Write OPTR values */
  FLASH->SDKR = (sdkr & 0x0000FFFF); /* Write SDKR values */
  FLASH->WRPR = (wrpr & 0x0000FFFF); /* Write WRPR values */

  FLASH->CR |= FLASH_CR_OPTSTRT;
  FLASH->CR |= FLASH_CR_EOPIE;
  M32(0x40022080) = 0xFF;           /*Address Write Date*/
  __asm("DSB");

  while (FLASH->SR & FLASH_SR_BSY)
  {
    IWDG->KR = 0xAAAA; /* Reload IWDG */
  }

  if (FLASH_SR_EOP != (FLASH->SR & FLASH_SR_EOP))       /* Check for FLASH_SR_EOP */
  {
    FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
    return (1);                                       /* Failed */
  }

  FLASH->SR |= FLASH_SR_EOP;       /* Reset FLASH_EOP */
  FLASH->CR &= ~FLASH_CR_OPTSTRT; /* Programming Disabled */
  FLASH->CR &= ~FLASH_CR_EOPIE;   /* Reset FLASH_EOPIE */

  return (0); /* Done */
}
#endif /* FLASH_OB */

#ifdef FLASH_OB
unsigned long Verify(unsigned long adr, unsigned long sz, unsigned char *buf)
{
  u32 optr;
  u32 sdkr;
  u32 wrpr;

  optr = *((u32 *)(buf + 0x00));
  sdkr = *((u32 *)(buf + 0x04));
  wrpr = *((u32 *)(buf + 0x0C));

  if (M32(adr + 0x00) != optr)
  {
    return (adr + 0x00);
  }
  if (M32(adr + 0x04) != sdkr)
  {
    return (adr + 0x04);
  }
  if (M32(adr + 0x0C) != wrpr)
  {
    return (adr + 0x0C);
  }
  return (adr + sz);
}
#endif /*  FLASH_OB */


#define HSI_HSICALIBRATION_24MHz          ((*(vu32 *)(0x1FFF0088)) & 0xFFFF)  /* 24MHz HSI calibration trimming value */
#define HSI_HSICALIBRATION_8MHz           ((*(vu32 *)(0x1FFF0080)) & 0xFFFF)  /* 8MHz HSI calibration trimming value */

#define RCC_CR_HSIRDY_Pos                     (10U)
#define RCC_CR_HSIRDY_Msk                     (0x1UL << RCC_CR_HSIRDY_Pos) /*!< 0x00000400 */
#define RCC_CR_HSIRDY                         RCC_CR_HSIRDY_Msk                /*!< Internal High Speed clock ready flag */

/********************  Bit definition for RCC_ICSCR register  ***************/
#define RCC_ICSCR_HSI_ABS_TRIMCR_Pos      (0U)
#define RCC_ICSCR_HSI_ABS_TRIMCR_Msk      (0xFFFUL << RCC_ICSCR_HSI_ABS_TRIMCR_Pos)    /*!< 0x00000FFF */
#define RCC_ICSCR_HSI_ABS_TRIMCR          RCC_ICSCR_HSI_ABS_TRIMCR_Msk                 /*!< HSI_ABS_TRIM[11:0] bits */
#define RCC_ICSCR_HSI_ABS_TRIMCR_0        (0x01UL << RCC_ICSCR_HSI_ABS_TRIMCR_Pos)     /*!< 0x00000001 */
#define RCC_ICSCR_HSI_ABS_TRIMCR_1        (0x02UL << RCC_ICSCR_HSI_ABS_TRIMCR_Pos)     /*!< 0x00000002 */
#define RCC_ICSCR_HSI_ABS_TRIMCR_2        (0x04UL << RCC_ICSCR_HSI_ABS_TRIMCR_Pos)     /*!< 0x00000004 */
#define RCC_ICSCR_HSI_ABS_TRIMCR_3        (0x08UL << RCC_ICSCR_HSI_ABS_TRIMCR_Pos)     /*!< 0x00000008 */
#define RCC_ICSCR_HSI_ABS_TRIMCR_4        (0x10UL << RCC_ICSCR_HSI_ABS_TRIMCR_Pos)     /*!< 0x00000010 */
#define RCC_ICSCR_HSI_ABS_TRIMCR_5        (0x20UL << RCC_ICSCR_HSI_ABS_TRIMCR_Pos)     /*!< 0x00000020 */
#define RCC_ICSCR_HSI_ABS_TRIMCR_6        (0x40UL << RCC_ICSCR_HSI_ABS_TRIMCR_Pos)     /*!< 0x00000040 */
#define RCC_ICSCR_HSI_ABS_TRIMCR_7        (0x80UL << RCC_ICSCR_HSI_ABS_TRIMCR_Pos)     /*!< 0x00000080 */
#define RCC_ICSCR_HSI_ABS_TRIMCR_8        (0x100UL << RCC_ICSCR_HSI_ABS_TRIMCR_Pos)    /*!< 0x00000100 */
#define RCC_ICSCR_HSI_ABS_TRIMCR_9        (0x200UL << RCC_ICSCR_HSI_ABS_TRIMCR_Pos)    /*!< 0x00000200 */
#define RCC_ICSCR_HSI_ABS_TRIMCR_10       (0x400UL << RCC_ICSCR_HSI_ABS_TRIMCR_Pos)    /*!< 0x00000400 */
#define RCC_ICSCR_HSI_ABS_TRIMCR_11       (0x800UL << RCC_ICSCR_HSI_ABS_TRIMCR_Pos)    /*!< 0x00000800 */
#define RCC_ICSCR_HSI_TC_TRIMCR_Pos       (12U)
#define RCC_ICSCR_HSI_TC_TRIMCR_Msk       (0xFUL << RCC_ICSCR_HSI_TC_TRIMCR_Pos)     /*!< 0x0000F000 */
#define RCC_ICSCR_HSI_TC_TRIMCR           RCC_ICSCR_HSI_TC_TRIMCR_Msk                 /*!< HSI_TC_TRIM[3:0] bits */
#define RCC_ICSCR_HSI_TC_TRIMCR_0         (0x01UL << RCC_ICSCR_HSI_TC_TRIMCR_Pos)     /*!< 0x00000001 */
#define RCC_ICSCR_HSI_TC_TRIMCR_1         (0x02UL << RCC_ICSCR_HSI_TC_TRIMCR_Pos)     /*!< 0x00000002 */
#define RCC_ICSCR_HSI_TC_TRIMCR_2         (0x04UL << RCC_ICSCR_HSI_TC_TRIMCR_Pos)     /*!< 0x00000004 */
#define RCC_ICSCR_HSI_TC_TRIMCR_3         (0x08UL << RCC_ICSCR_HSI_TC_TRIMCR_Pos)     /*!< 0x00000008 */
#define RCC_ICSCR_HSI_FS_CR_Pos           (16U)
#define RCC_ICSCR_HSI_FS_CR_Msk           (0x1UL << RCC_ICSCR_HSI_FS_CR_Pos)   /*!< 0x00010000 */
#define RCC_ICSCR_HSI_FS_CR               RCC_ICSCR_HSI_FS_CR_Msk              /*!< HSIFS bit  */

void InitRccAndFlashParam(void)
{
  RCC_ICSCR_HSI_FS_RESTORE = READ_BIT(RCC->ICSCR, RCC_ICSCR_HSI_FS_CR);
  RCC->ICSCR = (RCC->ICSCR & 0xFFFE0000) | (0x1 << RCC_ICSCR_HSI_FS_CR_Pos) | \
               HSI_HSICALIBRATION_24MHz;                 /* Set HSI 24M and Triming Data */

  while (RCC_CR_HSIRDY != READ_BIT(RCC->CR, RCC_CR_HSIRDY));

}

void UnInitRccAndFlashParam(void)
{
  MODIFY_REG(RCC->ICSCR, RCC_ICSCR_HSI_FS_CR, RCC_ICSCR_HSI_FS_RESTORE);
  switch (RCC_ICSCR_HSI_FS_RESTORE)
  {
  case RCC_ICSCR_HSI_FS_CR://24MHz
    RCC->ICSCR = (RCC->ICSCR & 0xFFFE0000) | (0x1 << RCC_ICSCR_HSI_FS_CR_Pos) | \
                 HSI_HSICALIBRATION_24MHz;                 /* Set HSI 24M and Triming Data */
    break;
  default://8MHz
    RCC->ICSCR = (RCC->ICSCR & 0xFFFE0000) | (0x0 << RCC_ICSCR_HSI_FS_CR_Pos) | \
                 HSI_HSICALIBRATION_8MHz;                 /* Set HSI 8M and Triming Data */
    break;
  }
  while (RCC_CR_HSIRDY != READ_BIT(RCC->CR, RCC_CR_HSIRDY));
}



