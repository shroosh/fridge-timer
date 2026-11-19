#include <stdint.h>
#include <string.h>

struct Symbol {
  Symbol(const uint8_t* data)
      : pixel_data_{data[0], data[1], data[2], data[3],
                    data[4], data[5], data[6], data[7]} {}

  Symbol() = default;

  void set_pixel(uint8_t x, uint8_t y, bool on) {
    if (x > 4 || y > 7) return;

    if (on) {
      pixel_data_[y] |= 1 << (4 - x);
    } else {
      pixel_data_[y] &= ~(1 << (4 - x));
    }
  }

  void clear() { memset(pixel_data_, 0, sizeof(pixel_data_)); }

  const uint8_t* get_data() const { return pixel_data_; }

 private:
  uint8_t pixel_data_[8] = {0};
};