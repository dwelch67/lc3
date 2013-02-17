
not r0,r1
and r0,r1,#0
add r0,r0,#7
top:
trap 0x67
add r0,r0,#0xFFFF
brnp top
trap 0x25
