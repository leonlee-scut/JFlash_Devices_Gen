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
 * Project:      Flash Programming Functions for Puya PY32F030xx Flash
 * --------------------------------------------------------------------------- */

/* History:
 *  Version 1.0.0
 *    Initial release
 */

#include "FlashOS.h" /* FlashOS Structures */


typedef volatile unsigned char    vu8;
typedef          unsigned char     u8;
typedef volatile unsigned short   vu16;
typedef          unsigned short    u16;
typedef volatile unsigned long    vu32;
typedef          unsigned long     u32;

#define M8(adr)  (*((vu8  *) (adr)))
#define M16(adr) (*((vu16 *) (adr)))
#define M32(adr) (*((vu32 *) (adr)))


#define SET_BIT(REG, BIT)     ((REG) |= (BIT))

#define CLEAR_BIT(REG, BIT)   ((REG) &= ~(BIT))

#define READ_BIT(REG, BIT)    ((REG) & (BIT))

#define CLEAR_REG(REG)        ((REG) = (0x0))

#define WRITE_REG(REG, VAL)   ((REG) = (VAL))

#define READ_REG(REG)         ((REG))

#define MODIFY_REG(REG, CLEARMASK, SETMASK)  WRITE_REG((REG), (((READ_REG(REG)) & (~(CLEARMASK))) | (SETMASK)))

/* Peripheral Memory Map */
#define FLASH_BASE            (0x08000000UL)  /*!< FLASH base address */
#define FLASH_END             (0x0803FFFFUL)  /*!< FLASH end address */
#define FLASH_SIZE            (FLASH_END - FLASH_BASE + 1)
#define FLASH_PAGE_SIZE       0x00000100U     /*!< FLASH Page Size, 256 Bytes */
#define FLASH_PAGE_NB         (FLASH_SIZE / FLASH_PAGE_SIZE)
#define FLASH_SECTOR_SIZE     0x00002000U     /*!< FLASH Sector Size, 8k Bytes */
#define FLASH_SECTOR_NB       (FLASH_SIZE / FLASH_SECTOR_SIZE)
#define SRAM1_BASE            (0x20000000UL)  /*!< SRAM1(8 KB) base address */
#define SRAM2_BASE            (0x20002000UL)  /*!< SRAM2(24 KB) base address */
#define OB_BASE               (0x1FFF1D00UL)  /*!< Flash Option Bytes base address */
#define UID_BASE              (0x1FFF1C00UL)  /*!< Unique device ID register base address */
#define OTP_BASE              (0x1FFF1A00UL)

#define RCC_BASE               0x40021000
#define IWDG_BASE              0x40003000
#define WWDG_BASE              0x40002C00
#define FLASH_R_BASE           0x40022000

#define FLAHS_SIZE_ADDR                 0x1FFF1FFC
#define FLAHS_BANK_SIZE                (((M32(FLAHS_SIZE_ADDR) & 0x3) + 1) * 32 * 1024)

#define RCC             ((RCC_TypeDef*) RCC_BASE)
#define IWDG            ((IWDG_TypeDef *) IWDG_BASE)
#define WWDG            ((WWDG_TypeDef *) WWDG_BASE)
#define FLASH           ((FLASH_TypeDef*) FLASH_R_BASE)

#define HSI_24M_TRIM_ADDR (0x1FFF1E10UL)
#define FLASH_24M_TRIM_ADDR (0x1FFF1E98UL)


/**
* @brief RCC Registers
*/
typedef struct
{
  vu32 CR;               /*!< RCC CR Register,                    Address offset: 0x00  */
  vu32 ICSCR;            /*!< RCC ICSCR Register,                 Address offset: 0x04  */
} RCC_TypeDef;

/**
* @brief IWDG Registers
*/
typedef struct
{
  vu32 KR;               /*!< IWDG KR Register,                   Address offset: 0x0  */
  vu32 PR;               /*!< IWDG PR Register,                   Address offset: 0x4  */
  vu32 RLR;              /*!< IWDG RLR Register,                  Address offset: 0x8  */
  vu32 SR;               /*!< IWDG SR Register,                   Address offset: 0xC  */
  vu32 ICNTR;            /*!< IWDG ICNTR Register,                Address offset: 0x10  */
  vu32 ICR;              /*!< IWDG ICR Register,                  Address offset: 0x14  */
} IWDG_TypeDef;

/**
* @brief WWDG Registers
*/
typedef struct
{
  vu32 CR;               /*!< WWDR CR Register,                   Address offset: 0x0  */
  vu32 CFR;              /*!< WWDG CFR Register,                  Address offset: 0x4  */
  vu32 SR;               /*!< WWDG SR Register,                   Address offset: 0x8  */
} WWDG_TypeDef;

/**
  * @brief FLASH Registers
  */
typedef struct
{
  vu32 ACR;              /*!< FLASH ACR Register,                 Address offset: 0x00  */
  vu32 RESERVED1;
  vu32 KEYR;             /*!< FLASH KEYR Register,                Address offset: 0x08  */
  vu32 OPTKEYR;          /*!< FLASH OPTKEYR Register,             Address offset: 0x0C  */
  vu32 SR;               /*!< FLASH SR Register,                  Address offset: 0x10  */
  vu32 CR;               /*!< FLASH CR Register,                  Address offset: 0x14  */
  vu32 ECCR;             /*!< FLASH ECCR Register,                Address offset: 0x18  */
  vu32 RESERVED2;
  vu32 OPTR;             /*!< FLASH OPTR Register,                Address offset: 0x20  */
  vu32 RESERVED3;
  vu32 WRPR;             /*!< FLASH WRPR Register,                Address offset: 0x28  */
  vu32 RESERVED4;
  vu32 PCROPR0;          /*!< FLASH PCROPR0 Register,             Address offset: 0x30  */
  vu32 PCROPR1;          /*!< FLASH PCROPR1 Register,             Address offset: 0x34  */
  vu32 RESERVED5[22];
  vu32 LPCR;             /*!< FLASH LPCR Register,                Address offset: 0x90  */
  vu32 RESERVED6[27];
  vu32 TS0;              /*!< FLASH TS0 Register,                 Address offset: 0x100  */
  vu32 TS1;              /*!< FLASH TS1 Register,                 Address offset: 0x104  */
  vu32 TS2P;             /*!< FLASH TS2P Register,                Address offset: 0x108  */
  vu32 TPS3;             /*!< FLASH TPS3 Register,                Address offset: 0x10C  */
  vu32 TS3;              /*!< FLASH TS3 Register,                 Address offset: 0x110  */
  vu32 PERTPE;           /*!< FLASH PERTPE Register,              Address offset: 0x114  */
  vu32 SMERTPE;          /*!< FLASH SMERTPE Register,             Address offset: 0x118  */
  vu32 PRGTPE;           /*!< FLASH PRGTPE Register,              Address offset: 0x11C  */
  vu32 PRETPE;           /*!< FLASH PRETPE Register,              Address offset: 0x120  */
  vu32 TACLK2PW;         /*!< FLASH TACLK2PW Register,            Address offset: 0x124  */
} FLASH_TypeDef;



/* Flash Keys */
#define FLASH_KEY1              ((unsigned int)0x45670123)
#define FLASH_KEY2              ((unsigned int)0xCDEF89AB)
#define FLASH_OPTKEY1           ((unsigned int)0x08192A3B)
#define FLASH_OPTKEY2           ((unsigned int)0x4C5D6E7F)


#define RCC_CR_MSIDIV_Pos                         (0U)
#define RCC_CR_MSIDIV_Msk                         (0x7UL<<RCC_CR_MSIDIV_Pos)                        /*!< 0x00000007 */
#define RCC_CR_MSIDIV                             RCC_CR_MSIDIV_Msk
#define RCC_CR_MSIDIV_0                           (0x1UL<<RCC_CR_MSIDIV_Pos)                        /*!< 0x00000001 */
#define RCC_CR_MSIDIV_1                           (0x2UL<<RCC_CR_MSIDIV_Pos)                        /*!< 0x00000002 */
#define RCC_CR_MSIDIV_2                           (0x4UL<<RCC_CR_MSIDIV_Pos)                        /*!< 0x00000004 */
#define RCC_CR_HSIKERON_Pos                       (6U)
#define RCC_CR_HSIKERON_Msk                       (0x1UL<<RCC_CR_HSIKERON_Pos)                      /*!< 0x00000040 */
#define RCC_CR_HSIKERON                           RCC_CR_HSIKERON_Msk
#define RCC_CR_MSION_Pos                          (7U)
#define RCC_CR_MSION_Msk                          (0x1UL<<RCC_CR_MSION_Pos)                         /*!< 0x00000080 */
#define RCC_CR_MSION                              RCC_CR_MSION_Msk
#define RCC_CR_HSION_Pos                          (8U)
#define RCC_CR_HSION_Msk                          (0x1UL<<RCC_CR_HSION_Pos)                         /*!< 0x00000100 */
#define RCC_CR_HSION                              RCC_CR_HSION_Msk
#define RCC_CR_MSIRDY_Pos                         (9U)
#define RCC_CR_MSIRDY_Msk                         (0x1UL<<RCC_CR_MSIRDY_Pos)                        /*!< 0x00000200 */
#define RCC_CR_MSIRDY                             RCC_CR_MSIRDY_Msk
#define RCC_CR_HSIRDY_Pos                         (10U)
#define RCC_CR_HSIRDY_Msk                         (0x1UL<<RCC_CR_HSIRDY_Pos)                        /*!< 0x00000400 */
#define RCC_CR_HSIRDY                             RCC_CR_HSIRDY_Msk
#define RCC_CR_HSIDIV_Pos                         (11U)
#define RCC_CR_HSIDIV_Msk                         (0x7UL<<RCC_CR_HSIDIV_Pos)                        /*!< 0x00003800 */
#define RCC_CR_HSIDIV                             RCC_CR_HSIDIV_Msk
#define RCC_CR_HSIDIV_0                           (0x1UL<<RCC_CR_HSIDIV_Pos)                        /*!< 0x00000800 */
#define RCC_CR_HSIDIV_1                           (0x2UL<<RCC_CR_HSIDIV_Pos)                        /*!< 0x00001000 */
#define RCC_CR_HSIDIV_2                           (0x4UL<<RCC_CR_HSIDIV_Pos)                        /*!< 0x00002000 */

#define RCC_ICSCR_HSI_TRIMCR_Pos                  (0U)
#define RCC_ICSCR_HSI_TRIMCR_Msk                  (0x1FFFUL<<RCC_ICSCR_HSI_TRIMCR_Pos)              /*!< 0x00001FFF */
#define RCC_ICSCR_HSI_TRIMCR                      RCC_ICSCR_HSI_TRIMCR_Msk
#define RCC_ICSCR_HSI_FS_OPCR_Pos                 (13U)
#define RCC_ICSCR_HSI_FS_OPCR_Msk                 (0x7UL<<RCC_ICSCR_HSI_FS_OPCR_Pos)                  /*!< 0x0000E000 */
#define RCC_ICSCR_HSI_FS_OPCR                     RCC_ICSCR_HSI_FS_OPCR_Msk
#define RCC_ICSCR_HSI_FS_OPCR_0                   (0x1UL<<RCC_ICSCR_HSI_FS_OPCR_Pos)                  /*!< 0x00002000 */
#define RCC_ICSCR_HSI_FS_OPCR_1                   (0x2UL<<RCC_ICSCR_HSI_FS_OPCR_Pos)                  /*!< 0x00004000 */
#define RCC_ICSCR_HSI_FS_OPCR_2                   (0x4UL<<RCC_ICSCR_HSI_FS_OPCR_Pos)                  /*!< 0x00008000 */

/********************************* Bit definition for FLASH_ACR register ********************************************/
#define FLASH_ACR_LATENCY_Pos                     (0U)
#define FLASH_ACR_LATENCY_Msk                     (0x3UL<<FLASH_ACR_LATENCY_Pos)                    /*!< 0x00000003 */
#define FLASH_ACR_LATENCY                         FLASH_ACR_LATENCY_Msk                             /*!< Latency */
#define FLASH_ACR_LATENCY_0                       (0x1UL<<FLASH_ACR_LATENCY_Pos)                    /*!< 0x00000001 */
#define FLASH_ACR_LATENCY_1                       (0x2UL<<FLASH_ACR_LATENCY_Pos)                    /*!< 0x00000002 */

/********************************* Bit definition for FLASH_SR register *********************************************/
#define FLASH_SR_EOP_Pos                          (0U)
#define FLASH_SR_EOP_Msk                          (0x1UL<<FLASH_SR_EOP_Pos)                         /*!< 0x00000001 */
#define FLASH_SR_EOP                              FLASH_SR_EOP_Msk                                  /*!< End of operation */
#define FLASH_SR_WRPERR_Pos                       (4U)
#define FLASH_SR_WRPERR_Msk                       (0x1UL<<FLASH_SR_WRPERR_Pos)                      /*!< 0x00000010 */
#define FLASH_SR_WRPERR                           FLASH_SR_WRPERR_Msk                               /*!< Write protection error */

#define FLASH_SR_OPTVERR_Pos                      (15U)
#define FLASH_SR_OPTVERR_Msk                      (0x1UL<<FLASH_SR_OPTVERR_Pos)                     /*!< 0x00008000 */
#define FLASH_SR_OPTVERR                          FLASH_SR_OPTVERR_Msk                              /*!< Option and trimming bits loading validity error */
#define FLASH_SR_BSY0_Pos                         (16U)
#define FLASH_SR_BSY0_Msk                         (0x1UL<<FLASH_SR_BSY0_Pos)                        /*!< 0x00010000 */
#define FLASH_SR_BSY0                             FLASH_SR_BSY0_Msk                                 /*!< Bank0 Busy */
#define FLASH_SR_BSY1_Pos                         (17U)
#define FLASH_SR_BSY1_Msk                         (0x1UL<<FLASH_SR_BSY1_Pos)                        /*!< 0x00020000 */
#define FLASH_SR_BSY1                             FLASH_SR_BSY1_Msk                                 /*!< Bank1 Busy */

/********************************* Bit definition for FLASH_CR register *********************************************/
#define FLASH_CR_PG_Pos                           (0U)
#define FLASH_CR_PG_Msk                           (0x1UL<<FLASH_CR_PG_Pos)                          /*!< 0x00000001 */
#define FLASH_CR_PG                               FLASH_CR_PG_Msk                                   /*!< Page Program */
#define FLASH_CR_PER_Pos                          (1U)
#define FLASH_CR_PER_Msk                          (0x1UL<<FLASH_CR_PER_Pos)                         /*!< 0x00000002 */
#define FLASH_CR_PER                              FLASH_CR_PER_Msk                                  /*!< Page Erase */
#define FLASH_CR_MER0_Pos                         (2U)
#define FLASH_CR_MER0_Msk                         (0x1UL<<FLASH_CR_MER0_Pos)                        /*!< 0x00000004 */
#define FLASH_CR_MER0                             FLASH_CR_MER0_Msk                                 /*!< Bank 0 Mass Erase */
#define FLASH_CR_MER1_Pos                         (3U)
#define FLASH_CR_MER1_Msk                         (0x1UL<<FLASH_CR_MER1_Pos)                        /*!< 0x00000008 */
#define FLASH_CR_MER1                             FLASH_CR_MER1_Msk                                 /*!< Bank 1 Mass Erase */
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
#define FLASH_CR_RDERR0IE_Pos                     (22U)
#define FLASH_CR_RDERR0IE_Msk                     (0x1UL<<FLASH_CR_RDERR0IE_Pos)                    /*!< 0x00400000 */
#define FLASH_CR_RDERR0IE                         FLASH_CR_RDERR0IE_Msk                             /*!< Bank0 pcrop read error interrupt enable */
#define FLASH_CR_RDERR1IE_Pos                     (23U)
#define FLASH_CR_RDERR1IE_Msk                     (0x1UL<<FLASH_CR_RDERR1IE_Pos)                    /*!< 0x00800000 */
#define FLASH_CR_RDERR1IE                         FLASH_CR_RDERR1IE_Msk                             /*!< Bank1 pcrop read error interrupt enable */
#define FLASH_CR_EOPIE_Pos                        (24U)
#define FLASH_CR_EOPIE_Msk                        (0x1UL<<FLASH_CR_EOPIE_Pos)                       /*!< 0x01000000 */
#define FLASH_CR_EOPIE                            FLASH_CR_EOPIE_Msk                                /*!< End of operation interrupt enable */
#define FLASH_CR_ERRIE_Pos                        (25U)
#define FLASH_CR_ERRIE_Msk                        (0x1UL<<FLASH_CR_ERRIE_Pos)                       /*!< 0x02000000 */
#define FLASH_CR_ERRIE                            FLASH_CR_ERRIE_Msk                                /*!< Error interrupt enable */
#define FLASH_CR_OBL_LAUNCH_Pos                   (27U)
#define FLASH_CR_OBL_LAUNCH_Msk                   (0x1UL<<FLASH_CR_OBL_LAUNCH_Pos)                  /*!< 0x08000000 */
#define FLASH_CR_OBL_LAUNCH                       FLASH_CR_OBL_LAUNCH_Msk                           /*!< Force the option bytes loading */
#define FLASH_CR_OPTLOCK_Pos                      (30U)
#define FLASH_CR_OPTLOCK_Msk                      (0x1UL<<FLASH_CR_OPTLOCK_Pos)                     /*!< 0x40000000 */
#define FLASH_CR_OPTLOCK                          FLASH_CR_OPTLOCK_Msk                              /*!< Option Lock */
#define FLASH_CR_LOCK_Pos                         (31U)
#define FLASH_CR_LOCK_Msk                         (0x1UL<<FLASH_CR_LOCK_Pos)                        /*!< 0x80000000 */
#define FLASH_CR_LOCK                             FLASH_CR_LOCK_Msk                                 /*!< Lock */

#define FLASH_OPTR_IWDG_STDBY_Pos                 (10U)
#define FLASH_OPTR_IWDG_STDBY_Msk                 (0x1UL<<FLASH_OPTR_IWDG_STDBY_Pos)                /*!< 0x00000400 */
#define FLASH_OPTR_IWDG_STDBY                     FLASH_OPTR_IWDG_STDBY_Msk                         /*!< IWDG Standy Count Control */
#define FLASH_OPTR_IWDG_STOP_Pos                  (11U)
#define FLASH_OPTR_IWDG_STOP_Msk                  (0x1UL<<FLASH_OPTR_IWDG_STOP_Pos)                 /*!< 0x00000800 */
#define FLASH_OPTR_IWDG_STOP                      FLASH_OPTR_IWDG_STOP_Msk                          /*!< IWDG Stop Count Control */
#define FLASH_OPTR_IWDG_SW_Pos                    (12U)
#define FLASH_OPTR_IWDG_SW_Msk                    (0x1UL<<FLASH_OPTR_IWDG_SW_Pos)                   /*!< 0x00001000 */
#define FLASH_OPTR_IWDG_SW                        FLASH_OPTR_IWDG_SW_Msk                            /*!< IWDG Software Enable */
#define FLASH_OPTR_WWDG_SW_Pos                    (13U)
#define FLASH_OPTR_WWDG_SW_Msk                    (0x1UL<<FLASH_OPTR_WWDG_SW_Pos)                   /*!< 0x00002000 */
#define FLASH_OPTR_WWDG_SW                        FLASH_OPTR_WWDG_SW_Msk                            /*!< WWDG Software Enable */
#define FLASH_OPTR_BFB_Pos                        (23U)
#define FLASH_OPTR_BFB_Msk                        (0x1UL<<FLASH_OPTR_BFB_Pos)                       /*!< 0x00800000 */
#define FLASH_OPTR_BFB                            FLASH_OPTR_BFB_Msk                                /*!< Boot Flash Bank */


#define WWDG_CR_T_Pos                             (0U)
#define WWDG_CR_T_Msk                             (0x7FUL<<WWDG_CR_T_Pos)                           /*!< 0x0000007F */
#define WWDG_CR_T                                 WWDG_CR_T_Msk                                     /*!< wwdg counter bit6 */
#define WWDG_CR_WDGA_Pos                          (7U)
#define WWDG_CR_WDGA_Msk                          (0x1UL<<WWDG_CR_WDGA_Pos)                         /*!< 0x00000080 */
#define WWDG_CR_WDGA                              WWDG_CR_WDGA_Msk                                  /*!< WWDG Activation */

#define WWDG_CFR_W_Pos                            (0U)
#define WWDG_CFR_W_Msk                            (0x7FUL<<WWDG_CFR_W_Pos)                          /*!< 0x0000007F */
#define WWDG_CFR_W                                WWDG_CFR_W_Msk                                    /*!< window value bit 6 */

#define WWDG_CFR_WDGTB_Pos                        (7U)
#define WWDG_CFR_WDGTB_Msk                        (0x3UL<<WWDG_CFR_WDGTB_Pos)                       /*!< 0x00000180 */
#define WWDG_CFR_WDGTB                            WWDG_CFR_WDGTB_Msk                                /*!< time base bit 1 */
#define WWDG_CFR_WDGTB_0                          (0x1UL<<WWDG_CFR_WDGTB_Pos)                       /*!< 0x00000080 */
#define WWDG_CFR_WDGTB_1                          (0x2UL<<WWDG_CFR_WDGTB_Pos)                       /*!< 0x00000100 */
#define WWDG_CFR_EWI_Pos                          (9U)
#define WWDG_CFR_EWI_Msk                          (0x1UL<<WWDG_CFR_EWI_Pos)                         /*!< 0x00000200 */
#define WWDG_CFR_EWI                              WWDG_CFR_EWI_Msk                                  /*!< Early wakeup interrupt */

int ErasePage(unsigned long adr);
u32 RCC_ICSCR_HSI_FS_RESTORE;
void InitRccAndFlashParam(void);
void UnInitRccAndFlashParam(void);

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
  FLASH->ACR &= ~FLASH_ACR_LATENCY;   /* Zero Wait State, no Prefetch */
  FLASH->SR |= FLASH_SR_EOP; /* Clear EOP flag */

  while (FLASH->SR & (FLASH_SR_BSY0 | FLASH_SR_BSY1));  /* Check FLASH_SR_BSY */

  if ((FLASH->OPTR & FLASH_OPTR_WWDG_SW) == 0)         /* Test if WWDG is running (IWDG in HW mode) */
  {
    /* Set WWDG time out to maximum */
    WWDG->CFR |= WWDG_CFR_WDGTB;                      /* Set prescaler to maximum */
    SET_BIT(WWDG->CR, WWDG_CR_T);                     /* Reload WWDG */
  }

  if ((FLASH->OPTR & FLASH_OPTR_IWDG_SW) == 0)       /* Test if IWDG is running (IWDG in HW mode) */
  {
    /* Set IWDG time out to maximum */
    IWDG->KR = 0xAAAA;                              /* Reload IWDG */
    IWDG->KR = 0x5555;                              /* Enable write access to IWDG_PR and IWDG_RLR */
    IWDG->PR = 0x06;                                /* Set prescaler to maximum */
    IWDG->RLR = 0xFFF;                              /* Set reload value to maximum */
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
int EraseChip(void)
{
#ifdef FLASH_OTP
  EraseSector(OTP_BASE);
#endif        /* FLASH_OTP */

#ifdef FLASH_MEM
  FLASH->SR |= FLASH_SR_EOP; /* Reset FLASH_EOP */
  if (READ_BIT(FLASH->OPTR, FLASH_OPTR_BFB) != FLASH_OPTR_BFB)
  {
    FLASH->CR |= FLASH_CR_MER0; /* Mass BANK0 Erase Enabled */
    FLASH->CR |= FLASH_CR_EOPIE;
    M32(FLASH_BASE) = 0xFF;     /*BANK0*/
    __asm("DSB");
    while (FLASH->SR & (FLASH_SR_BSY0 | FLASH_SR_BSY1))
    {
      WWDG->CR |= WWDG_CR_T;       /* Reload WWDG */
      IWDG->KR = 0xAAAAU;         /* Reload IWDG */
    }
    if (FLASH_SR_EOP != (FLASH->SR & FLASH_SR_EOP))       /* Check for FLASH_SR_EOP */
    {
      FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
      return (1);                                       /* Failed */
    }
    FLASH->CR &= ~(FLASH_CR_MER0); /* Mass Erase Disabled */
    FLASH->CR &= ~FLASH_CR_EOPIE; /* Reset FLASH_EOPIE */

    FLASH->SR |= FLASH_SR_EOP; /* Reset FLASH_EOP */
    FLASH->CR |= FLASH_CR_MER1; /* Mass BANK1 Erase Enabled */
    FLASH->CR |= FLASH_CR_EOPIE;
    M32(FLASH_BASE + FLAHS_BANK_SIZE) = 0xFF; /*BANK1*/

  }
  else
  {
    FLASH->CR |= FLASH_CR_MER1; /* Mass BANK1 Erase Enabled */
    FLASH->CR |= FLASH_CR_EOPIE;
    M32(FLASH_BASE + FLAHS_BANK_SIZE) = 0xFF; /*BANK1*/

    __asm("DSB");

    while (FLASH->SR & (FLASH_SR_BSY0 | FLASH_SR_BSY1))
    {
      WWDG->CR |= WWDG_CR_T;       /* Reload WWDG */
      IWDG->KR = 0xAAAAU;         /* Reload IWDG */
    }

    if (FLASH_SR_EOP != (FLASH->SR & FLASH_SR_EOP))       /* Check for FLASH_SR_EOP */
    {
      FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
      return (1);                                       /* Failed */
    }
    FLASH->CR &= ~(FLASH_CR_MER1); /* Mass Erase Disabled */
    FLASH->CR &= ~FLASH_CR_EOPIE; /* Reset FLASH_EOPIE */

    FLASH->SR |= FLASH_SR_EOP; /* Reset FLASH_EOP */
    FLASH->CR |= FLASH_CR_MER0; /* Mass BANK0 Erase Enabled */
    FLASH->CR |= FLASH_CR_EOPIE;
    M32(FLASH_BASE) = 0xFF;     /*BANK0*/

  }

  __asm("DSB");

  while (FLASH->SR & (FLASH_SR_BSY0 | FLASH_SR_BSY1))
  {
    WWDG->CR |= WWDG_CR_T;       /* Reload WWDG */
    IWDG->KR = 0xAAAAU;         /* Reload IWDG */
  }

  if (FLASH_SR_EOP != (FLASH->SR & FLASH_SR_EOP))       /* Check for FLASH_SR_EOP */
  {
    FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
    return (1);                                       /* Failed */
  }

  FLASH->CR &= ~(FLASH_CR_MER0 | FLASH_CR_MER1); /* Mass Erase Disabled */
  FLASH->CR &= ~FLASH_CR_EOPIE; /* Reset FLASH_EOPIE */

#endif        /* FLASH_MEM */
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
  FLASH->SR |= FLASH_SR_EOP; /* Clear EOP flag */

  FLASH->CR |= FLASH_CR_SER; /* Sector Erase Enabled */

  FLASH->CR |= FLASH_CR_EOPIE;

  M32(adr) = 0xFFFFFFFF; /* Sector Address */

  while (FLASH->SR & (FLASH_SR_BSY0 | FLASH_SR_BSY1))
  {
    WWDG->CR |= WWDG_CR_T;       /* Reload WWDG */
    IWDG->KR = 0xAAAAU;         /* Reload IWDG */
  }

  if (FLASH_SR_EOP != (FLASH->SR & FLASH_SR_EOP))       /* Check for FLASH_SR_EOP */
  {
    FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
    return (1);                                       /* Failed */
  }

  FLASH->CR &= ~FLASH_CR_SER;   /* Sector Erase Disabled */

  FLASH->CR &= ~FLASH_CR_EOPIE;

  return (0); /* Done */
}
#endif

#ifdef FLASH_OTP
int EraseSector(unsigned long adr)
{
  FLASH->SR |= FLASH_SR_EOP;
  FLASH->CR |= FLASH_CR_UPER;
  FLASH->CR |= FLASH_CR_EOPIE;
  M32(adr) = 0xFF;
  __asm("DSB");

  while (FLASH->SR & (FLASH_SR_BSY0 | FLASH_SR_BSY1))
  {
    WWDG->CR |= WWDG_CR_T;
    IWDG->KR = 0xAAAAU;
  }

  if (FLASH_SR_EOP != (FLASH->SR & FLASH_SR_EOP))       /* Check for FLASH_SR_EOP */
  {
    FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
    return (1);                                       /* Failed */
  }

  FLASH->CR &= ~FLASH_CR_UPER;
  FLASH->CR &= ~FLASH_CR_EOPIE;

  return (0); /* Done */
}
#endif /* FLASH_OTP */

#ifdef FLASH_OB
int EraseSector(unsigned long adr)
{
  return (0);
}
#endif /* FLASH_OB */

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

  SET_BIT(FLASH->SR, FLASH_SR_EOP); /* Clear EOP flag */

  while (sz)
  {
    FLASH->CR |= FLASH_CR_PG; /* Programming Enabled */

    for (u8 i = 0; i < FLASH_PAGE_SIZE / 4; i++)
    {

      M32(adr + i * 4) = *((u32 *)(buf + i * 4)); /* Program the first word of the Double Word */

      if (i == (FLASH_PAGE_SIZE / 4 - 2))
      {
        FLASH->CR |= FLASH_CR_PGSTRT;
      }
    }
    __asm("DSB");


    while (FLASH->SR & (FLASH_SR_BSY0 | FLASH_SR_BSY1))
    {
      WWDG->CR |= WWDG_CR_T;
      IWDG->KR = 0x0000AAAAU;
    }

    if (FLASH_SR_EOP != (FLASH->SR & FLASH_SR_EOP))       /* Check for FLASH_SR_EOP */
    {
      FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
      return (1);                                       /* Failed */
    }
    FLASH->CR &= ~FLASH_CR_PG; /* Programming Disabled */
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

    while (FLASH->SR & (FLASH_SR_BSY0 | FLASH_SR_BSY1))
    {
      WWDG->CR |= WWDG_CR_T;
      IWDG->KR = 0x0000AAAAU;
    }

    if (FLASH_SR_EOP != (FLASH->SR & FLASH_SR_EOP))       /* Check for FLASH_SR_EOP */
    {
      FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
      return (1);                                       /* Failed */
    }

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
  u32 optr;
  u32 wrpr;
  u32 pcropr0;
  u32 pcropr1;

  optr = M16(buf + 0x00) | (M16(buf + 0x04) << 16);
  wrpr = M16(buf + 0x10) | (M16(buf + 0x14) << 16);
  pcropr0 = M16(buf + 0x18) | (M16(buf + 0x1C) << 16);
  pcropr1 = M32(buf + 0x20) | (M16(buf + 0x24) << 16);

  FLASH->SR |= FLASH_SR_EOP; /* Reset FLASH_EOP */

  FLASH->OPTR = optr & 0x03FFFFFF;
  FLASH->WRPR = wrpr & 0xFFFFFFFF;
  FLASH->PCROPR0 = pcropr0 & 0x01FF01FF;
  FLASH->PCROPR1 = pcropr1 & 0x01FF01FF;

  FLASH->CR |= FLASH_CR_OPTSTRT;
  FLASH->CR |= FLASH_CR_EOPIE;

  M32(0x1FFF1D00) = 0xFFFFFFFF;

  while (FLASH->SR & (FLASH_SR_BSY0 | FLASH_SR_BSY1))
  {
    WWDG->CR |= WWDG_CR_T;
    IWDG->KR = 0xAAAAU;
  }

  if (FLASH_SR_EOP != (FLASH->SR & FLASH_SR_EOP))       /* Check for FLASH_SR_EOP */
  {
    FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
    return (1);                                       /* Failed */
  }

  FLASH->CR &= ~FLASH_CR_OPTSTRT; /* Programming Disabled */
  FLASH->CR &= ~FLASH_CR_EOPIE;   /* Reset FLASH_EOPIE */

  return (0); /* Done */
}
#endif /*  FLASH_OB */


#ifdef FLASH_OB
unsigned long Verify(unsigned long adr, unsigned long sz, unsigned char *buf)
{
  unsigned long baseaddr = 0x1FFF1D00;
  unsigned long offset = 0;

  if (adr != baseaddr || sz < 0x24)
  {
    return (adr);
  }

  if (M16(buf + offset) != M16(baseaddr + offset))
  {
    return (baseaddr + offset);
  }

  offset = 0x4;
  if (M16(buf + offset) != M16(baseaddr + offset))
  {
    return (baseaddr + offset);
  }

  offset = 0x10;
  if (M16(buf + offset) != M16(baseaddr + offset))
  {
    return (baseaddr + offset);
  }

  offset = 0x14;
  if (M16(buf + offset) != M16(baseaddr + offset))
  {
    return (baseaddr + offset);
  }

  offset = 0x18;
  if (M16(buf + offset) != M16(baseaddr + offset))
  {
    return (baseaddr + offset);
  }

  offset = 0x1C;
  if (M16(buf + offset) != M16(baseaddr + offset))
  {
    return (baseaddr + offset);
  }

  offset = 0x20;
  if (M16(buf + offset) != M16(baseaddr + offset))
  {
    return (baseaddr + offset);
  }

  offset = 0x24;
  if (M16(buf + offset) != M16(baseaddr + offset))
  {
    return (baseaddr + offset);
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

  while (FLASH->SR & (FLASH_SR_BSY0 | FLASH_SR_BSY1))
  {
    WWDG->CR |= WWDG_CR_T;
    IWDG->KR = 0xAAAAU;
  }

  if (FLASH_SR_EOP != (FLASH->SR & FLASH_SR_EOP))       /* Check for FLASH_SR_EOP */
  {
    FLASH->SR |= FLASH_SR_EOP;                        /* Reset FLASH_EOP */
    return (1);                                       /* Failed */
  }

  FLASH->CR &= ~FLASH_CR_PER;   /* page Erase Disabled */
  FLASH->CR &= ~FLASH_CR_EOPIE; /* Reset FLASH_EOPIE */

  return (0); /* Done */
}
#endif /* FLASH_MEM */

#define RCC_HSICALIBRATION_8MHz                  ((0x1<<13) | ((*(vu32 *)(0x1FFF1E04)) & 0x1FFF))  /*!< 8MHz HSI calibration trimming value */
#define RCC_HSICALIBRATION_16MHz                 ((0x2<<13) | ((*(vu32 *)(0x1FFF1E08)) & 0x1FFF))  /*!< 16MHz HSI calibration trimming value */
#define RCC_HSICALIBRATION_22p12MHz              ((0x3<<13) | ((*(vu32 *)(0x1FFF1E0C)) & 0x1FFF))  /*!< 22.12MHz HSI calibration trimming value */
#define RCC_HSICALIBRATION_24MHz                 ((0x4<<13) | ((*(vu32 *)(0x1FFF1E10)) & 0x1FFF))  /*!< 24MHz HSI calibration trimming value */
#define RCC_HSICALIBRATION_48MHz                 ((0x5<<13) | ((*(vu32 *)(0x1FFF1E14)) & 0x1FFF))  /*!< 48MHz HSI calibration trimming value */
#define RCC_HSICALIBRATION_64MHz                 ((0x6<<13) | ((*(vu32 *)(0x1FFF1E18)) & 0x1FFF))  /*!< 64MHz HSI calibration trimming value */

/********************************* Bit definition for RCC_CR register ***********************************************/
#define RCC_CR_HSIRDY_Pos                         (10U)
#define RCC_CR_HSIRDY_Msk                         (0x1UL<<RCC_CR_HSIRDY_Pos)                        /*!< 0x00000400 */
#define RCC_CR_HSIRDY                             RCC_CR_HSIRDY_Msk

/********************************* Bit definition for RCC_ICSCR register ********************************************/
#define RCC_ICSCR_HSI_TRIMCR_Pos                  (0U)
#define RCC_ICSCR_HSI_TRIMCR_Msk                  (0x1FFFUL<<RCC_ICSCR_HSI_TRIMCR_Pos)              /*!< 0x00001FFF */
#define RCC_ICSCR_HSI_TRIMCR                      RCC_ICSCR_HSI_TRIMCR_Msk
#define RCC_ICSCR_HSI_FS_OPCR_Pos                 (13U)
#define RCC_ICSCR_HSI_FS_OPCR_Msk                 (0x7UL<<RCC_ICSCR_HSI_FS_OPCR_Pos)                  /*!< 0x0000E000 */
#define RCC_ICSCR_HSI_FS_OPCR                     RCC_ICSCR_HSI_FS_OPCR_Msk
#define RCC_ICSCR_HSI_FS_OPCR_0                   (0x1UL<<RCC_ICSCR_HSI_FS_OPCR_Pos)                  /*!< 0x00002000 */
#define RCC_ICSCR_HSI_FS_OPCR_1                   (0x2UL<<RCC_ICSCR_HSI_FS_OPCR_Pos)                  /*!< 0x00004000 */
#define RCC_ICSCR_HSI_FS_OPCR_2                   (0x4UL<<RCC_ICSCR_HSI_FS_OPCR_Pos)                  /*!< 0x00008000 */

/* Set HSI 24M and Triming Data */
void InitRccAndFlashParam(void)
{
  RCC_ICSCR_HSI_FS_RESTORE = READ_BIT(RCC->ICSCR, RCC_ICSCR_HSI_FS_OPCR);
  MODIFY_REG(RCC->ICSCR, (RCC_ICSCR_HSI_FS_OPCR | RCC_ICSCR_HSI_TRIMCR), RCC_HSICALIBRATION_24MHz);    /* HSI_24MHz */
  while (RCC_CR_HSIRDY != READ_BIT(RCC->CR, RCC_CR_HSIRDY));
}

/* Set HSI and Triming Data defult*/
void UnInitRccAndFlashParam(void)
{
  MODIFY_REG(RCC->ICSCR, RCC_ICSCR_HSI_FS_OPCR, RCC_ICSCR_HSI_FS_RESTORE);
  switch (RCC_ICSCR_HSI_FS_RESTORE)
  {
  case (RCC_ICSCR_HSI_FS_OPCR_1|RCC_ICSCR_HSI_FS_OPCR_2):         /* 64MHz */
    FLASH->ACR |= FLASH_ACR_LATENCY_1;   /* 2 Wait State */
    MODIFY_REG(RCC->ICSCR, (RCC_ICSCR_HSI_FS_OPCR | RCC_ICSCR_HSI_TRIMCR), RCC_HSICALIBRATION_64MHz);    /* HSI_64MHz */
    break;
  case (RCC_ICSCR_HSI_FS_OPCR_0|RCC_ICSCR_HSI_FS_OPCR_2):         /* 48MHz */
    FLASH->ACR |= FLASH_ACR_LATENCY_0;   /* 1 Wait State */
    MODIFY_REG(RCC->ICSCR, (RCC_ICSCR_HSI_FS_OPCR | RCC_ICSCR_HSI_TRIMCR), RCC_HSICALIBRATION_48MHz);    /* HSI_48MHz */
    break;
  case RCC_ICSCR_HSI_FS_OPCR_2:                  /* 24MHz */
    MODIFY_REG(RCC->ICSCR, (RCC_ICSCR_HSI_FS_OPCR | RCC_ICSCR_HSI_TRIMCR), RCC_HSICALIBRATION_24MHz);    /* HSI_24MHz */
    break;
  case (RCC_ICSCR_HSI_FS_OPCR_0|RCC_ICSCR_HSI_FS_OPCR_1):                  /* 22.12MHz */
    MODIFY_REG(RCC->ICSCR, (RCC_ICSCR_HSI_FS_OPCR | RCC_ICSCR_HSI_TRIMCR), RCC_HSICALIBRATION_22p12MHz);    /* HSI_22.12MHz */
    break;
  case RCC_ICSCR_HSI_FS_OPCR_1:                  /* 16MHz */
    MODIFY_REG(RCC->ICSCR, (RCC_ICSCR_HSI_FS_OPCR | RCC_ICSCR_HSI_TRIMCR), RCC_HSICALIBRATION_16MHz);    /* HSI_16MHz */
    break;
  default: /* 8MHz */
    MODIFY_REG(RCC->ICSCR, (RCC_ICSCR_HSI_FS_OPCR | RCC_ICSCR_HSI_TRIMCR), RCC_HSICALIBRATION_8MHz);    /* HSI_16MHz */
    break;
  }
  while (RCC_CR_HSIRDY != READ_BIT(RCC->CR, RCC_CR_HSIRDY));
}



