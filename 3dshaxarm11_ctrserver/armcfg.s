.text
.cpu mpcore
.arm

.global get_armcfgregs
.type get_armcfgregs, %function
get_armcfgregs:
mrc p15, 0, r1, c0, c0, 0 @ Main ID Register
str r1, [r0, #0x0]
mrc p15, 0, r1, c0, c0, 1 @ Cache Type Register
str r1, [r0, #0x4]
mrc p15, 0, r1, c0, c0, 3 @ TLB Type Register
str r1, [r0, #0x8]
mrc p15, 0, r1, c0, c0, 3 @ CPUID Register
str r1, [r0, #0xc]

mrc p15, 0, r1, c0, c1, 0 @ ID_PFR0, Processor Feature Register 0
str r1, [r0, #0x10]
mrc p15, 0, r1, c0, c1, 1 @ ID_PFR1, Processor Feature Register 1
str r1, [r0, #0x14]
mrc p15, 0, r1, c0, c1, 2 @ ID_DFR0, Debug Feature Register 0
str r1, [r0, #0x18]
mrc p15, 0, r1, c0, c1, 4 @ IDMMFR0, Memory Model Feature Register 0
str r1, [r0, #0x1c]
mrc p15, 0, r1, c0, c1, 5 @ IDMMFR0, Memory Model Feature Register 1
str r1, [r0, #0x20]
mrc p15, 0, r1, c0, c1, 6 @ IDMMFR0, Memory Model Feature Register 2
str r1, [r0, #0x24]
mrc p15, 0, r1, c0, c1, 7 @ IDMMFR0, Memory Model Feature Register 3.
str r1, [r0, #0x28]

mrc p15, 0, r1, c0, c2, 0 @ ID_ISAR0, ISA Feature Register 0
str r1, [r0, #0x2c]
mrc p15, 0, r1, c0, c2, 1 @ ID_ISAR1, ISA Feature Register 1
str r1, [r0, #0x30]
mrc p15, 0, r1, c0, c2, 2 @ ID_ISAR2, ISA Feature Register 2
str r1, [r0, #0x34]
mrc p15, 0, r1, c0, c2, 3 @ ID_ISAR3, ISA Feature Register 3
str r1, [r0, #0x38]
mrc p15, 0, r1, c0, c2, 4 @ ID_ISAR4, ISA Feature Register 4
str r1, [r0, #0x3c]

bx lr

