# This was hastily assembled from one of DJ's examples plus R8C/2L/2K reference
# information.  There are likely errors.  Check the data sheet to verify.

pm0  04  : : : : pm03 : : :
pm1  05  : : : : : pm12 : :
cm0  06  .bits
cm1  07  .bits
prcr 0a  : : : : prc3 prc2 prc1 prc0
ocd  0c  : : : : ocd3 ocd2 ocd1 ocd0

wdtr 0d
wdts 0e
wdc 0f

rmad0 10
rmad0l 10
rmad0m 11
rmad0h 12
aier 13
rmad1 14
rmad1l 14
rmad1m 15
rmad1h 16

cspr 1c

fra0 23  .bits
fra1 24
fra2 25
fra6 2b

vca1 31
vca2 32 .bits
vw1c 36
vw2c 37
vw0c 38

trcic 47
trd0ic 48
trd1ic 49
s2tic 4b
s2ric 4c
kupic 4d
adic 4e
s0tic 51
s0ric 52
traic 56
trbic 58
int1ic 59
int3ic 5a
int0ic 5d

u0mr  a0  : prye pry stps ckdir smd:3
u0brg a1
u0tb  a2
u0tbl a2
u0tbh a3
u0c0  a4  uform ckpol nch : txept : clk:2
u0c1  a5  : : : : ri re ti te
u0rb  a6  .HI
u0rbl a6
u0rbh a7

ad     c0  .HI
adl c0
adh c1
adcon2 d4  : : : : : : : smp
adcon0 d6  cks0 adst adcap adgsel0 md ch:3
adcon1 d7  : : vcut cks1 bits : : :

p0    e0  .bits
p1    e1  .bits
pd0   e2  .bits
pd1   e3  .bits
p2    e4  .bits
p3    e5  .bits
pd2   e6  .bits
pd3   e7  .bits
p4    e8  .bits
pd4   ea  .bits
p2drr f4  .bits

inten f9  int3pl int3en : : int1pl int1en int0pl int0en
intf  fa  int3f:2 : : int1f:2 int0f:2
kien  fb
pur0  fc  .bits
pur1  fd  .bits

tracr  100 : : tundf tedgf : tstop tcstf tstart
traioc 101 : : tipf:2 tiosel toena topcr tedgsel
tramr  102 tckcut tck:3 : tmod:3
trapre 103
tra    104

lincr2 105
lincr 106
linst 107

trbcr  108 : : : : : tstop tcstf tstart
trbocr 109 : : : : : tosstf tossp tosst
trbioc 10a : : : : inoseg inostg tocnt topl
trbmr  10b tckcut : tck:2 twrc : tmod:2
trbpre 10c
trbsc  10d
trbpr  10e

trcmr 120
trccr1 121
trcier 122
trcsr 123
trcior0 124
trcior1 125
trc 126
trcgra 128
trcgrb 12a
trcgrc 12c
trcgrd 12e
trccr2 130
trcdf 131
trcoer 132

trdstr 137
trdmr 138
trdpmr 139
trdfcr 13a
trdoer1 13b
trdoer2 13c
trdocr 13d
trddf0 13e
trddf1 13f
trdcr0 140
trdiora0 141
trdiorc0 142
trdsr0 143
trdier0 144
trdpocr0 145
trd0 146
trdgra0 148
trdgrb0 14a
trdgrc0 14c
trdgrd0 14e
trdcr1 150
trdiora1 151
trdiorc1 152
trdsr1 153
trdier1 154
trdpocr1 155
trd1 156
trdgra1 158
trdgrb1 15a
trdgrc1 15c
trdgrd1 15e

u2mr  160  : prye pry stps ckdir smd:3
u2brg 161
u2tb  162
u2tbl 162
u2tbh 163
u2c0  164  uform ckpol nch : txept : clk:2
u2c1  165  : : : : ri re ti te
u2rb  166  .HI
u2rbl 166
u2rbh 167

fmr4 1b3
fmr1 1b5
fmr0 1b7
