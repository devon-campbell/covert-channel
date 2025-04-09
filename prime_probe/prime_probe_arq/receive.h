#ifndef RECEIVE_H
#define RECEIVE_H

#include <stdbool.h>
#include <stdint.h>
#include "covert_utils.h"

// Prime+Probe receive-side logic
bool receive_bit(void);
frame_t *receive_byte_frame(frame_t *frame_buf, uint64_t timeout);

#endif // RECEIVE_H
