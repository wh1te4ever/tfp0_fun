#!/bin/zsh
# (?:stp x[^;]*; ){2}add[^;]*; mov[^;]*; ldr[^;]*; ldr[^;]*; ldr[^;]*; blr[^;]*; str[^;]*; mov[^;]*; ldp[^;]*; ldp[^;]*; ret
grep 'stp x[^;]*; stp x[^;]*; add[^;]*; mov[^;]*; ldr[^;]*; ldr[^;]*; ldr[^;]*; blr[^;]*; str[^;]*; mov[^;]*; ldp[^;]*; ldp[^;]*; ret' kernelcache-gadgets.txt > prologue.txt

# ldp[^;]*; ldp[^;]*; 
# grep 'ldp[^;]*; ldp[^;]*;' kernelcache-gadgets.txt > populate.txt

# ldp[^;]*; ldp[^;]*; ldr[^;]*; mov[^;]*; br
grep 'ldp[^;]*; ldp[^;]*; ldr[^;]*; mov[^;]*; br' kernelcache-gadgets.txt > populate.txt

# ldp x1, x2, [^;]*; ldp x3, x4, [^;]*; [^;]*; [^;]*; [^;]*; [^;]*; [^;]*;[^;]*;
# ^(?!.*\bsp\b).*ldp x1, x2, [^;]*; ldp x3, x4, [^;]*;