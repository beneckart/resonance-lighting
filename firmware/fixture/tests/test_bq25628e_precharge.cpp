#include "test_util.h"

#include "../src/core/bq25628e_precharge.h"

int main() {
  CHECK(!bq25628ePrechargeMaValid(0));
  CHECK(!bq25628ePrechargeMaValid(9));
  CHECK(bq25628ePrechargeMaValid(10));
  CHECK(bq25628ePrechargeMaValid(300));
  CHECK(bq25628ePrechargeMaValid(310));
  CHECK(!bq25628ePrechargeMaValid(311));
  CHECK(!bq25628ePrechargeMaValid(305));

  // POR 30 mA is code 3 / 0x18. Production 300 mA is code 0x1E / 0xF0.
  CHECK_EQ(bq25628ePrechargeReg10(0x00, 30), 0x18);
  CHECK_EQ(bq25628ePrechargeReg10(0x00, 300), 0xF0);
  CHECK_EQ(bq25628ePrechargeMa(0x18), 30);
  CHECK_EQ(bq25628ePrechargeMa(0xF0), 300);

  // Preserve the reserved low bits during the read-modify-write.
  CHECK_EQ(bq25628ePrechargeReg10(0x05, 300), 0xF5);
  CHECK_EQ(bq25628ePrechargeMa(0xF5), 300);

  return testReport("test_bq25628e_precharge");
}
