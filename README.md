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
This implementation demonstrates a microarchitectural covert channel leveraging the Prime+Probe cache side-channel technique to transmit information stealthily between a sender and receiver. At a high level, the sender and receiver coordinate by repeatedly targeting a specific set in the CPU's last-level cache (LLC). The sender encodes bits by selectively evicting cache lines: a logical `1` bit is represented by aggressively evicting cache lines (thus causing cache misses), while a `0` bit is indicated by idling (allowing cache hits). 

Concurrently, the receiver probes the same cache set by measuring access latencies to a carefully chosen eviction set. Elevated latency indicates sender-induced cache misses (interpreted as bit `1`), whereas low latency indicates the absence of eviction activity (interpreted as bit `0`). Synchronization between sender and receiver is achieved using the processor's timestamp counter (rdtscp instruction) to align their time slots precisely. An Automatic Repeat reQuest (ARQ) protocol overlays this channel to ensure reliability, detect errors, and manage retransmissions. The result is a covert communication mechanism exploiting microarchitectural timing variations invisible to conventional monitoring tools.

(Why believed technique would work)

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
(Hardware specs & any information required for reproducing your results)

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


