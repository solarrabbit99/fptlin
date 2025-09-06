# Fptlin

Fixed parameter tractable algorithms for monitoring linearizability for concurrent histories.

## Histories

Histories are text files that provide a **data type** as header and **operations** on a single object of the stated data type in the following rows. Yes, we assume all operations to be completed by the end of the history.

**Data types** are prefixed with `#` followed by any of the supported tags:

- `stack`
- `queue`
- `priorityqueue`
- `rmw`

**Operations** are denoted by process id, method, value (one or more), start time, and end time in that order. Refer to examples in `testcases` directory for supported methods and values for a given data type.

### Example

```
# stack
0 push 1 1 2
0 peek 2 3 4
1 pop 2 5 6
1 pop 1 7 8
```

## Usage

```bash
-bash-4.2$ ./fptlin [-tvh] <history_file>
```

### Options

- `-t`: report time taken in seconds
- `-v`: print verbose information
- `-h`: include header
- `--help`: show help message

### Output

The standard output shall be in the form:

```
"%d %f\n", <linearizability>, <time taken>
```

_linearizability_ prints `1` when input history is linearizable, `0` otherwise.

```bash
-bash-4.2$ ./build/fptlin -t testcases/priorityqueue/lin_simple_0.log
1 1.8e-05
```

## Time Complexity

`n` is the size of the given history and `k` is the number of processes

| Data Type                  | Time Complexity          |
| -------------------------- | ------------------------ |
| Stack                      | $O(k2^{3k} \cdot n^3)$   |
| Queue                      | $O(k2^{2k} \cdot n^2)$   |
| Priority Queue             | $O(k2^k \cdot n\log{n})$ |
| Read-Modify-Write Register | $O(k2^k \cdot n)$        |
