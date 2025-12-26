# RA3 1.76 Decompilation

This repository contains a decompilation of the qagamei386.so binary from RA3 1.76.

## Verifying

1. Setup a VM running Red Hat 9 "Shrike".
2. Fumble mounting CDs and install gcc.
3. Copy code from tag `v176` onto the machine (ssh, wget, ...).
4. `cd ra3-sdk`
5. `make`
6. `sha1 build/qagamei386.so`
7. Should output `eaa27d2f02df58ab52a795a859bbeb8bddb1afb6  build/qagamei386.so`

## Building QVM

1. `git clone https://github.com/inolen/ra3_176_decomp.git`
2. `cd ra3_176_decomp/ra3-sdk`
3. `ENABLE_QVM=1 make`
