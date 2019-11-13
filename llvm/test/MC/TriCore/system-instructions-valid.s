# RUN: llvm-mc %s -triple=tricore -show-encoding \
# RUN:     | FileCheck -check-prefixes=CHECK,CHECK-INST %s


# CHECK-INST: syscall 0
# CHECK: encoding: [0xad,0x00,0x80,0x00]
.code32
syscall 0

# CHECK-INST: syscall 1
# CHECK: encoding: [0xad,0x10,0x80,0x00]
.code32
syscall 1

# CHECK-INST: syscall 510
# CHECK: encoding: [0xad,0xe0,0x9f,0x00]
.code32
syscall 510

# CHECK-INST: syscall 511
# CHECK: encoding: [0xad,0xf0,0x9f,0x00]
.code32
syscall 511

# CHECK-INST: dsync
# CHECK: encoding: [0x0d,0x00,0x80,0x04]
.code32
dsync

# CHECK-INST: isync
# CHECK: encoding: [0x0d,0x00,0xc0,0x04]
.code32
isync

# CHECK-INST: disable
# CHECK: encoding: [0x0d,0x00,0x40,0x03]
.code32
disable

# CHECK-INST: disable %d0
# CHECK: encoding: [0x0d,0x00,0xc0,0x03]
.code32
disable %d0

# CHECK-INST: disable %d1
# CHECK: encoding: [0x0d,0x01,0xc0,0x03]
.code32
disable %d1

# CHECK-INST: disable %d14
# CHECK: encoding: [0x0d,0x0e,0xc0,0x03]
.code32
disable %d14

# CHECK-INST: disable %d15
# CHECK: encoding: [0x0d,0x0f,0xc0,0x03]
.code32
disable %d15

# CHECK-INST: enable
# CHECK: encoding: [0x0d,0x00,0x00,0x03]
.code32
enable

# CHECK-INST: trapsv
# CHECK: encoding: [0x0d,0x00,0x40,0x05]
.code32
trapsv

# CHECK-INST: trapv
# CHECK: encoding: [0x0d,0x00,0x00,0x05]
.code32
trapv

# CHECK-INST: nop
# CHECK: encoding: [0x00,0x00]
.code16
nop

# CHECK-INST: nop
# CHECK: encoding: [0x0d,0x00,0x00,0x00]
.code32
nop

# CHECK-INST: restore %d0
# CHECK: encoding: [0x0d,0x00,0x80,0x03]
.code32
restore %d0

# CHECK-INST: restore %d1
# CHECK: encoding: [0x0d,0x01,0x80,0x03]
.code32
restore %d1

# CHECK-INST: restore %d14
# CHECK: encoding: [0x0d,0x0e,0x80,0x03]
.code32
restore %d14

# CHECK-INST: restore %d15
# CHECK: encoding: [0x0d,0x0f,0x80,0x03]
.code32
restore %d15

# CHECK-INST: wait
# CHECK: encoding: [0x0d,0x00,0x80,0x05]
.code32
wait

# CHECK-INST: debug
# CHECK: encoding: [0x00,0xa0]
.code16
debug

# CHECK-INST: debug
# CHECK: encoding: [0x0d,0x00,0x00,0x01]
.code32
debug

# CHECK-INST: rfm
# CHECK: encoding: [0x0d,0x00,0x40,0x01]
.code32
rfm

# CHECK-INST: rstv
# CHECK: encoding: [0x2f,0x00,0x00,0x00]
.code32
rstv

# CHECK-INST: updfl %d0
# CHECK: encoding: [0x4b,0x00,0xc1,0x00]
.code32
updfl %d0

# CHECK-INST: updfl %d1
# CHECK: encoding: [0x4b,0x01,0xc1,0x00]
.code32
updfl %d1

# CHECK-INST: updfl %d14
# CHECK: encoding: [0x4b,0x0e,0xc1,0x00]
.code32
updfl %d14

# CHECK-INST: updfl %d15
# CHECK: encoding: [0x4b,0x0f,0xc1,0x00]
.code32
updfl %d15

# CHECK-INST: mfcr %d0, 0
# CHECK: encoding: [0x4d,0x00,0x00,0x00]
.code32
mfcr %d0, 0

# CHECK-INST: mfcr %d0, 1
# CHECK: encoding: [0x4d,0x10,0x00,0x00]
.code32
mfcr %d0, 1

# CHECK-INST: mfcr %d0, 65534
# CHECK: encoding: [0x4d,0xe0,0xff,0x0f]
.code32
mfcr %d0, 65534

# CHECK-INST: mfcr %d0, 65535
# CHECK: encoding: [0x4d,0xf0,0xff,0x0f]
.code32
mfcr %d0, 65535

# CHECK-INST: mfcr %d1, 0
# CHECK: encoding: [0x4d,0x00,0x00,0x10]
.code32
mfcr %d1, 0

# CHECK-INST: mfcr %d1, 1
# CHECK: encoding: [0x4d,0x10,0x00,0x10]
.code32
mfcr %d1, 1

# CHECK-INST: mfcr %d1, 65534
# CHECK: encoding: [0x4d,0xe0,0xff,0x1f]
.code32
mfcr %d1, 65534

# CHECK-INST: mfcr %d1, 65535
# CHECK: encoding: [0x4d,0xf0,0xff,0x1f]
.code32
mfcr %d1, 65535

# CHECK-INST: mfcr %d14, 0
# CHECK: encoding: [0x4d,0x00,0x00,0xe0]
.code32
mfcr %d14, 0

# CHECK-INST: mfcr %d14, 1
# CHECK: encoding: [0x4d,0x10,0x00,0xe0]
.code32
mfcr %d14, 1

# CHECK-INST: mfcr %d14, 65534
# CHECK: encoding: [0x4d,0xe0,0xff,0xef]
.code32
mfcr %d14, 65534

# CHECK-INST: mfcr %d14, 65535
# CHECK: encoding: [0x4d,0xf0,0xff,0xef]
.code32
mfcr %d14, 65535

# CHECK-INST: mfcr %d15, 0
# CHECK: encoding: [0x4d,0x00,0x00,0xf0]
.code32
mfcr %d15, 0

# CHECK-INST: mfcr %d15, 1
# CHECK: encoding: [0x4d,0x10,0x00,0xf0]
.code32
mfcr %d15, 1

# CHECK-INST: mfcr %d15, 65534
# CHECK: encoding: [0x4d,0xe0,0xff,0xff]
.code32
mfcr %d15, 65534

# CHECK-INST: mfcr %d15, 65535
# CHECK: encoding: [0x4d,0xf0,0xff,0xff]
.code32
mfcr %d15, 65535

# CHECK-INST: mtcr 0, %d0
# CHECK: encoding: [0xcd,0x00,0x00,0x00]
.code32
mtcr 0, %d0

# CHECK-INST: mtcr 0, %d1
# CHECK: encoding: [0xcd,0x01,0x00,0x00]
.code32
mtcr 0, %d1

# CHECK-INST: mtcr 0, %d14
# CHECK: encoding: [0xcd,0x0e,0x00,0x00]
.code32
mtcr 0, %d14

# CHECK-INST: mtcr 0, %d15
# CHECK: encoding: [0xcd,0x0f,0x00,0x00]
.code32
mtcr 0, %d15

# CHECK-INST: mtcr 1, %d0
# CHECK: encoding: [0xcd,0x10,0x00,0x00]
.code32
mtcr 1, %d0

# CHECK-INST: mtcr 1, %d1
# CHECK: encoding: [0xcd,0x11,0x00,0x00]
.code32
mtcr 1, %d1

# CHECK-INST: mtcr 1, %d14
# CHECK: encoding: [0xcd,0x1e,0x00,0x00]
.code32
mtcr 1, %d14

# CHECK-INST: mtcr 1, %d15
# CHECK: encoding: [0xcd,0x1f,0x00,0x00]
.code32
mtcr 1, %d15

# CHECK-INST: mtcr 65534, %d0
# CHECK: encoding: [0xcd,0xe0,0xff,0x0f]
.code32
mtcr 65534, %d0

# CHECK-INST: mtcr 65534, %d1
# CHECK: encoding: [0xcd,0xe1,0xff,0x0f]
.code32
mtcr 65534, %d1

# CHECK-INST: mtcr 65534, %d14
# CHECK: encoding: [0xcd,0xee,0xff,0x0f]
.code32
mtcr 65534, %d14

# CHECK-INST: mtcr 65534, %d15
# CHECK: encoding: [0xcd,0xef,0xff,0x0f]
.code32
mtcr 65534, %d15

# CHECK-INST: mtcr 65535, %d0
# CHECK: encoding: [0xcd,0xf0,0xff,0x0f]
.code32
mtcr 65535, %d0

# CHECK-INST: mtcr 65535, %d1
# CHECK: encoding: [0xcd,0xf1,0xff,0x0f]
.code32
mtcr 65535, %d1

# CHECK-INST: mtcr 65535, %d14
# CHECK: encoding: [0xcd,0xfe,0xff,0x0f]
.code32
mtcr 65535, %d14

# CHECK-INST: mtcr 65535, %d15
# CHECK: encoding: [0xcd,0xff,0xff,0x0f]
.code32
mtcr 65535, %d15
