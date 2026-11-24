#include <stdint.h>
#include <string.h>

#ifndef Symbol_H
#define Symbol_H

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

  uint8_t* get_data() { return pixel_data_; }

 private:
  uint8_t pixel_data_[8] = {0};
};

// Every time a write is done, an internal counter is incremented such that each
// pixel is only turned on for the opacity% of the time.
struct OpacitySymbol {
  OpacitySymbol(const uint8_t data[5][8]) {
    for (uint8_t x = 0; x < 5; x++) {
      for (uint8_t y = 0; y < 8; y++) {
        pixel_data_[x][y] = data[x][y] > 100 ? 100 : data[x][y];
      }
    }
  }

  OpacitySymbol() = default;

  void set_pixel(uint8_t x, uint8_t y, uint8_t opacity) {
    if (x > 4 || y > 7 || opacity > 100) return;

    pixel_data_[x][y] = opacity;
  }

  void clear() { memset(pixel_data_, 0, sizeof(pixel_data_)); }

  void write_data(uint8_t* dest) {
    for (uint8_t x = 0; x < 5; x++) {
      for (uint8_t y = 0; y < 8; y++) {
        if (pixel_data_[x][y] > time_) {
          dest[y] |= (1 << (4 - x)); 
        } else {
          dest[y] &= ~(1 << (4 - x));
        }
      }
    }

    // Large step size coprime with 39 so all on/off values don't appear
    // contiguous.
    time_ = (time_ + 29) % 100;
  }

 private:
  uint8_t pixel_data_[5][8] = {0};
  uint8_t time_ = 0;
};

#endif