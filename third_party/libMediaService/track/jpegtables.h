#ifndef AVCODEC_JPEGTABLES_H
#define AVCODEC_JPEGTABLES_H

#include <stdint.h>

extern const uint8_t avpriv_mjpeg_bits_dc_luminance[];
extern const uint8_t avpriv_mjpeg_val_dc[];

extern const uint8_t avpriv_mjpeg_bits_dc_chrominance[];

extern const uint8_t avpriv_mjpeg_bits_ac_luminance[];
extern const uint8_t avpriv_mjpeg_val_ac_luminance[];

extern const uint8_t avpriv_mjpeg_bits_ac_chrominance[];
extern const uint8_t avpriv_mjpeg_val_ac_chrominance[];

void ff_mjpeg_build_huffman_codes(uint8_t *huff_size, uint16_t *huff_code,
                                  const uint8_t *bits_table,
                                  const uint8_t *val_table);

#endif /* AVCODEC_JPEGTABLES_H */
