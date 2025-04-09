Process Flow (Transmit on Left → Receive on Right):
[ arq_client ]          ⇄  [ arq_server ]
       ↓                         ↑
[ transmit_arq_frame ]  ⇄  [ receive_arq_frame ]
       ↓                         ↑
[ phy_send_bit ]        ⇄  [ phy_recv_bit ]
       ↓                         ↑
     [ Prime+Probe via pp_common.c ]


File Overview:
Layer        | Purpose                          | Files
--------------------------------------------------------------------------
Application  | User logic (send/recv msgs)      | arq_client.c, arq_server.c 
ARQ Protocol | Reliability, sequencing, ACKS    | arq_channel.c/.h
Physical     | Cache timing, prime+probe        | phy.c/.h, pp_common.c/.h
Utils        | Frame format, thresholds, parity | covert_utils.h

File Details:
- arq_client.c 
    - Sents string (e.g. "HELLO WORLD") using ARQ Protocol
    - calls `transmit_arq_frame(...)`
- arq_server.c
    - Listens for frames via `receive_arq_frame(...)`
    - Extracts the message & writes to `server_recv.txt`

- arq_channel.c 
    - Implements reliable transmission by:
        - Adding sequence numbers
        - Detecting stale frames
        - Retransmitting on timeout
- arq_channel.h 
    - Header for the above
    - Defines arq_frame_t (containing data, parity, seq nums)

- phy.c 
    - Implements:
        - phy_send_bit(): prime (and optionally evict)
        - phy_receive_bit(): probe access time, compare to threshold()
        - phy_get_time(), phy_init(), phy_compute_threshold()
    - Uses underlying chache sets & shared memory

- pp_common.c
    - Handles shared memory mapping
    - Finds eviction sets from physical address info
    - Sets up the Prime+Probe "cache set"

- covert_utils.h
    - Defines frame_t structure
    - Frame helpers:
        - byte_to_bools(), valid_frame(), construct_frame(), calculate_parity(), print_frame()
    - Defines rdtscp64()
    - Includes compute_dynamic_threshold() logic

System Requirements:
- Requires `/dev/hugepages` to be mounted (for shared memory)
- Tested on modern Intel CPUs with inclusive LLC
- Run client and server on isolated cores (`taskset -c`)
- May require `sudo` to access huge pages or perform low-level ops

Running the channel:
# Terminal 1 (Server)
sudo taskset -c 0 ./arq_server

# Terminal 2 (Client)
sudo taskset -c 1 ./arq_client "HELLO WORLD"
