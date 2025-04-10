# The RAS Channel
## Theory
### Microarchitectural Basis
The return address stack (RAS) is a microarchitectural structure that optimizes function 
calls. Every time a function is called, its return address is added to the stack. When the 
function completes execution, its return branch may be executed speculatively simply by popping the top of the stack. Because it's a microarchitectural structure, the RAS size is limited, so if the call stack is too deeply nested, earlier return branches are ejected, and their return addresses must be computed directly, costing execution time.

### RAS Timing Side-Channel
The limited capacity of the RAS, combined with the fact that it is shared among all 
processes on a core, exposes a potential side-channel that can be used to build a covert 
channel. A sending process is able to influence the time taken for a receiving process to 
return from a heavily nested process by flushing the RAS with its own heavily nested 
process.

### Implementation
To flesh out the covert channel implementation, we first need to know the size of the 
return address stack. To maximize the covert channel's accuracy, we will want to configure 
the receiving process to be nested exactly as deeply as the RAS is. RAS size can be 
benchmarked by timing execution time for increasingly nested functions and identifying the inflection point where the additional execution time per level of nesting increases. A graph is shown below. 

[INSERT GRAPH]

On the Google Cloud Instance, the RAS capacity is 16.

Once the RAS capacity is known, the implementation of the send and receive functions are as follows:

```
// SENDING
static inline void flush_ras(int count, int threshold){
    if (count == threshold) {
        sched_yield();
        return;
    }
    else flush_ras(++count, threshold);
}

// RECEIVING
static inline uint64_t recurse_and_yield(int depth, int count){
    if (count == depth){
        // Yield to transmitter process
        sched_yield();
        return rdtscp64();
    }else{
        if (count == 0){  // Last to return (assuming depth > 0)
            uint64_t start = recurse_and_yield(depth, count+1);
            return rdtscp64() - start;
        }
        else return recurse_and_yield(depth, count+1);
    }
}
```

To exchange information, the receiver first descends to its full depth of recursion. At 
this point, the RAS is now full with its return addresses. Then, it yields control to other 
processes on the core. At this point, the sender may flush the RAS by calling its own 
nested functions, or it may not. By measuring the time to return from its nested calls, the 
receiver can determine the activity of the sending method.

## Results


## Codebase
There are two relevant subdirectories in this directory.

`./basic` - Contains code for a basic implementation of the covert-channel. Running `make` will generate four executables: `benchmark`, `threshold`, `sender`, and `receiver`.
- `benchmark`: Generates data that can be plotted to determine RAS size.
- `threshold`: Continuously averages return time for a flushed and non-flushed RAS.
- `sender`: Repeatedly flushes the RAS to send ones (note: must use taskset to pin to the same CPU as receiver)
- `receiver`: Repeatedly checks nested return time to read incoming data from sender (note: must use taskset to pin to the same CPU as sender)

`./arq_ras` - Contains a client-server ARQ implementation with the RAS channel as its backend. Run `make all` to generate tests and targets, and `make test` for tests only.
- `arq_client` - 
- `arq_server` - 
- `send_bitstream` - 
- `receive_bistream` - 
