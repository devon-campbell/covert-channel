# Covert Channels
Contributors: Matheu Campbell (mgc2171), Isaac Trost (wit2102), Denzel Farmer (df2817), Devon Campbell (dec2180)

## Work Log
We met twice over the course of the assignment and communicated progress asynchronously throughout. Each member contributed equally to this work.  

## Reflection report
### Technique 1: Flush + Reload
(Explanation)
(Why believed technique would work)
(Expected bandwidth vs actual bandwidth achieved)
(Hardware specs & any information required for reproducing your results)

### Technique 2: Prime + Probe
This project implements a microarchitectural covert channel using the Prime+Probe cache side-channel technique to transmit data between isolated processes. The sender and receiver coordinate by targeting a shared last-level cache (LLC) set. To transmit a bit, the sender either evicts the cache set (encoding a `1`) or remains idle (encoding a `0`). The receiver probes the same set and infers the transmitted bit by measuring access latency: high latency implies eviction (`1`), low latency implies no eviction (`0`).

Synchronization is achieved using the processor’s timestamp counter (`rdtscp`), with both parties aligning to defined time slots via a `SLOT_MASK`. Shared cache set contention is ensured by selecting memory addresses that map to a specific LLC set (`TARGET_SET`) using large page mappings (`/dev/hugepages`), which also reduce TLB misses and improve physical address predictability.

To add reliability, the channel uses an Automatic Repeat reQuest (ARQ) protocol. Each frame includes start delimiters, parity bits, and sequence numbers, enabling the receiver to detect errors, discard stale frames, and request retransmissions. Together, these design choices allow for robust, stealthy communication via cache timing variations, without requiring shared memory or direct interprocess communication.

#### Bandwidth
##### Expected
At a high level, **bandwidth (in bits/sec)** can be estimated using:

```text
bandwidth = bits_per_frame / time_per_frame
```

Or, over the full transmission:

```text
bandwidth = total_bits_sent / total_time_taken
```

In this implementation, each ARQ frame consists of:

- 6 bits → initial sequence number  
- 8 bits → start delimiter  
- 8 bits → data  
- 1 bit  → parity  
- 5 bits → final sequence number  

**Total: 28 bits per frame**

Each bit occupies a fixed time slot, defined by `SLOT_MASK` in `phy.c`. For example:

```c
#define SLOT_MASK 0x3FFFF
```

This means the slot counter repeats every `2^18 = 262,144` cycles. On **GCP AMD EPYC "Rome"** CPUs (2.25 GHz), this translates to:

```
262,144 cycles ÷ 2.25 cycles/ns ≈ 116,508 ns ≈ 116.5 μs per slot
```

Thus, a single frame takes:

```
28 bits × 116.5 μs ≈ 3.26 milliseconds
```

And the theoretical bandwidth is:

```
Bits per second = 1 / 116.5e-6 × 28 ≈ 240,000 bits/sec ≈ 30,000 bytes/sec
```

**Note:** This is a best-case estimate. Real-world performance is lower due to:
- Retransmissions on timeout when a frame is lost or corrupted
- ACK delay (Each frame sent requires a correctly received ACK before the next can be transmitted)
- Cache contention or interference (other processes, cores, or interrupts may use the same cache set)
- Logging/printing delays

##### Actual 

#### Hardware specs & information to run
This experiment was conducted on a `e2-standard-4` GCP instance (4 vCPUs, 16 GB RAM) using the AMD EPYC Rome platform, which runs at a base frequency of 2.25 GHz. The machine uses 1 vCPU per physical core, which helps eliminate interference from sibling threads on the same core. 2 vCPUs were used, allowing the sender and receiver to be isolated on separate physical cores via `taskset -c 0` (server) and `taskset -c 1` (client), ensuring clearer timing behavior and reducing scheduling noise.

System Requirements:
- Requires `/dev/hugepages` to be mounted (for shared memory)
- Tested on modern Intel CPUs with inclusive LLC
- Run client and server on isolated cores (`taskset -c`)
- May require `sudo` to access huge pages or perform low-level ops

Running the channel:
```bash
# Enable huge pages
sudo sysctl -w vm.nr_hugepages=8
sudo mkdir -p /dev/hugepages
sudo mount -t hugetlbfs none /dev/hugepages

# Terminal 1 (Server)
sudo taskset -c 0 ./arq_server

# Terminal 2 (Client)
sudo taskset -c 1 ./arq_client "HELLO WORLD"
```

### Technique 3: Return Address Stack (RAS)
(Explanation)
(Why believed technique would work)
(Expected bandwidth vs actual bandwidth achieved)
(Hardware specs & any information required for reproducing your results)

### Technique 4: Port
(Explanation)
(Why believed technique would work)
(Expected bandwidth vs actual bandwidth achieved)
(Hardware specs & any information required for reproducing your results)


