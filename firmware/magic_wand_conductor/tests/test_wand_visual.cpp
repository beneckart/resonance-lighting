#include <stdio.h>

#include "core/wand_visual.h"

#define CHECK(x)                                                               \
  do {                                                                         \
    if (!(x)) {                                                                \
      fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x);                    \
      return 1;                                                                \
    }                                                                          \
  } while (0)

int main() {
  CHECK(wandGatherLoadedPixels(0, 360000) == 0);
  CHECK(wandGatherLoadedPixels(180000, 360000) == 18);
  CHECK(wandGatherLoadedPixels(360000, 360000) == 37);
  CHECK(wandGatherLoadedPixels(999999, 360000) == 37);

  CHECK(wandNextColor(WAND_COLOR_RED) == WAND_COLOR_GREEN);
  CHECK(wandNextColor(WAND_COLOR_GREEN) == WAND_COLOR_BLUE);
  CHECK(wandNextColor(WAND_COLOR_BLUE) == WAND_COLOR_RGB);
  CHECK(wandNextColor(WAND_COLOR_RGB) == WAND_COLOR_RED);

  WandRgb local = wandLocalColor(WAND_COLOR_RGB);
  CHECK(local.red == 2 && local.green == 2 && local.blue == 2);
  WandRgb tree = wandTreeColor(WAND_COLOR_RGB, 64, 21);
  CHECK(tree.red == 21 && tree.green == 21 && tree.blue == 21);
  tree = wandTreeColor(WAND_COLOR_BLUE, 64, 21);
  CHECK(tree.red == 0 && tree.green == 0 && tree.blue == 64);

  printf("wand_visual ok\n");
  return 0;
}
