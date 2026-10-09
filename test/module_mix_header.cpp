// Header-mode half of zr_module_smoke: this TU includes zr textually, the other
// imports it. Linking the two proves both modes name the same zr entities.
#include "zr/ranges.h"

int zr_mix_sum(zr::ranges::iota_view<int, int> v) {
    int sum = 0;
    for (int x : v) sum += x;
    return sum;
}
