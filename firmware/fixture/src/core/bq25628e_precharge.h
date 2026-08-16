// Pure helpers for BQ25628E REG0x10 IPRECHG encoding.
//
// TI BQ25628E Rev. C: IPRECHG is REG0x10[7:3], code 1h..1Fh,
// 10 mA per step. Bits 2:0 are reserved and must be preserved.
#pragma once

#include <stdint.h>

#define BQ25628E_PRECHARGE_MIN_MA 10
#define BQ25628E_PRECHARGE_MAX_MA 310
#define BQ25628E_PRECHARGE_STEP_MA 10
#define BQ25628E_PRECHARGE_MASK 0xF8u

static inline bool bq25628ePrechargeMaValid(uint16_t ma) {
  return ma >= BQ25628E_PRECHARGE_MIN_MA &&
         ma <= BQ25628E_PRECHARGE_MAX_MA &&
         (ma % BQ25628E_PRECHARGE_STEP_MA) == 0;
}

static inline uint8_t bq25628ePrechargeReg10(uint8_t current, uint16_t ma) {
  uint8_t code = (uint8_t)(ma / BQ25628E_PRECHARGE_STEP_MA);
  return (uint8_t)((current & ~BQ25628E_PRECHARGE_MASK) |
                   ((code << 3) & BQ25628E_PRECHARGE_MASK));
}

static inline uint16_t bq25628ePrechargeMa(uint8_t reg10) {
  uint8_t code = (uint8_t)((reg10 & BQ25628E_PRECHARGE_MASK) >> 3);
  return (uint16_t)code * BQ25628E_PRECHARGE_STEP_MA;
}
