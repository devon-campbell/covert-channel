#include "receive.h"
#include "phy.h"         
#include "covert_utils.h" // for frame_t helpers
#include <stdio.h>
#include <string.h>

bool receive_bit(void) {
    return phy_recv_bit();
}

/**
 * receive_byte_frame():
 *   - Detects the 8-bit start delimiter (0x9A) using a shift register
 *   - Then reads 8 data bits and 1 parity bit
 *   - Validates the full frame
 *   - Returns the frame_buf pointer on success, or NULL on timeout/invalid
 */
frame_t *receive_byte_frame(frame_t *frame_buf, uint64_t timeout)
{
    memset(frame_buf, 0, sizeof(frame_t));

    uint8_t shift_reg = 0;
    uint64_t start_tsc = rdtscp64();

    printf("[receive_byte_frame] Waiting up to %lu cycles for start delimiter...\n", timeout);

    while (1) {
        bool bit = phy_recv_bit(); 

        shift_reg = (shift_reg << 1) | (bit ? 1 : 0);

        if (shift_reg == START_DELIMITER) break;

        if (timeout > 0 && (rdtscp64() - start_tsc) > timeout) {
            printf("Timeout waiting for start delimiter\n");
            return NULL;
        }
    }

    byte_to_bools(shift_reg, frame_buf->start_delimiter);

    for (int i = 0; i < 8; i++)
        frame_buf->data[i] = phy_recv_bit();

    frame_buf->parity = phy_recv_bit();

    if (!valid_frame(*frame_buf)) {
        printf("Invalid frame received\n");
        print_frame(*frame_buf);
        return NULL;
    }

    printf("[receive_byte_frame] Valid frame received\n");
    print_frame(*frame_buf);
    return frame_buf;
}
