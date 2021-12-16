EESchema Schematic File Version 2  date Mon 19 Dec 2011 10:01:30 AM MST
LIBS:ubertooth-symbols
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:special
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:pogoprog-cache
EELAYER 25  0
EELAYER END
$Descr A4 11700 8267
encoding utf-8
Sheet 1 1
Title "Ubertooth Pogoprog"
Date "19 dec 2011"
Rev "$Rev$"
Comp "Copyright 2010 Michael Ossmann"
Comment1 "License: GPL v2"
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
Text Label 5000 3500 0    40   ~ 0
VCCIO
Text Notes 3800 4800 0    60   ~ 0
R5 is bridged on the PCB, providing\nRTS output on pin 2.  If CTS input is\ndesired, cut the R5 bridge trace and\ninstall a 0 ohm resistor at R6.  Do\nnot install R6 unless R5 is cut.
Text Label 5050 4200 0    40   ~ 0
CTS
Text Label 5050 3400 0    40   ~ 0
RTS
Connection ~ 4200 3400
Wire Wire Line
	4200 3400 4200 4200
Wire Wire Line
	4200 4200 4400 4200
Wire Wire Line
	4900 3400 5100 3400
Connection ~ 3300 3500
Wire Wire Line
	3300 2950 3300 4000
Wire Wire Line
	3100 3050 3100 2950
Connection ~ 4200 3200
Wire Wire Line
	3400 2950 3400 3200
Wire Wire Line
	3400 3200 5100 3200
Connection ~ 4400 3300
Wire Wire Line
	3600 2950 3600 3300
Wire Wire Line
	3600 3300 5100 3300
Wire Wire Line
	4000 2950 4000 3400
Connection ~ 7550 4000
Wire Wire Line
	7550 4100 7550 4000
Wire Wire Line
	7550 4500 7550 4600
Connection ~ 7100 4000
Wire Wire Line
	7100 4000 7100 5050
Wire Wire Line
	7100 5050 6700 5050
Wire Wire Line
	6700 5050 6700 5150
Connection ~ 6800 4300
Wire Wire Line
	6800 4250 6800 4400
Wire Wire Line
	6800 4900 6800 4800
Wire Wire Line
	6400 4500 6500 4500
Wire Wire Line
	6500 4500 6500 5150
Wire Wire Line
	2700 3950 2700 4000
Wire Wire Line
	3300 4000 3200 4000
Wire Wire Line
	3900 2950 3900 3050
Wire Wire Line
	4300 2950 4300 3600
Wire Wire Line
	5000 3500 5100 3500
Wire Wire Line
	6400 4200 6500 4200
Wire Wire Line
	6400 3500 6500 3500
Wire Wire Line
	6800 2700 6800 2650
Wire Wire Line
	6800 3300 6800 3200
Wire Wire Line
	7100 3700 7100 3800
Wire Wire Line
	7100 3800 6400 3800
Wire Wire Line
	6400 3700 6800 3700
Wire Wire Line
	7100 3300 7100 3200
Wire Wire Line
	7100 2650 7100 2700
Wire Wire Line
	6400 3400 6500 3400
Wire Wire Line
	6400 3900 6500 3900
Wire Wire Line
	5000 3800 5100 3800
Wire Wire Line
	4200 2950 4200 3200
Wire Wire Line
	7750 4000 6400 4000
Connection ~ 3300 4000
Wire Wire Line
	3300 4500 3500 4500
Wire Wire Line
	6600 5150 6600 4400
Wire Wire Line
	6600 4400 6400 4400
Wire Wire Line
	6400 4300 6800 4300
Wire Wire Line
	6300 5050 6300 5150
Wire Wire Line
	7250 4500 7250 4600
Wire Wire Line
	7250 4000 7250 4100
Connection ~ 7250 4000
Wire Wire Line
	4400 2950 4400 3300
Wire Wire Line
	5100 3600 3500 3600
Wire Wire Line
	3500 3600 3500 2950
Connection ~ 4300 3600
Wire Wire Line
	3200 2950 3200 3400
Connection ~ 4000 3400
Wire Wire Line
	3300 3500 4100 3500
Wire Wire Line
	4100 3500 4100 2950
Wire Wire Line
	3200 3400 4400 3400
Wire Wire Line
	5100 4200 4900 4200
$Comp
L R R6
U 1 1 4EA4EC65
P 4650 4200
F 0 "R6" V 4730 4200 50  0000 C CNN
F 1 "DNP" V 4650 4200 50  0000 C CNN
	1    4650 4200
	0    1    1    0   
$EndComp
$Comp
L R R5
U 1 1 4EA4EC49
P 4650 3400
F 0 "R5" V 4730 3400 50  0000 C CNN
F 1 "DNP" V 4650 3400 50  0000 C CNN
	1    4650 3400
	0    1    1    0   
$EndComp
Text Notes 1800 4800 0    60   ~ 0
R1 is bridged on the PCB, providing\n3.3V output on pin 3.  If 5V output is\ndesired, cut the R1 bridge trace and\ninstall a 0 ohm resistor at R2.  Do\nnot install R2 unless R1 is cut.\n\nThis affects both the supply output on\npin 3 and the logic levels on all pins.\n\n3.3V output should be limited to 50mA.\n5V output is direct from USB.
Text Notes 3100 2350 0    60   ~ 0
alternative\npin header
Text Notes 3950 2400 0    60   ~ 0
pogo pins
$Comp
L GND #PWR01
U 1 1 4EA4E16B
P 3100 3050
F 0 "#PWR01" H 3100 3050 30  0001 C CNN
F 1 "GND" H 3100 2980 30  0001 C CNN
	1    3100 3050
	1    0    0    -1  
$EndComp
Text Label 3300 3950 1    40   ~ 0
VCCIO
$Comp
L CONN_6 P2
U 1 1 4EA4E0D6
P 3350 2600
F 0 "P2" V 3300 2600 60  0000 C CNN
F 1 "DNP" V 3400 2600 60  0000 C CNN
	1    3350 2600
	0    -1   -1   0   
$EndComp
Text Label 6450 4500 0    40   ~ 0
D+
Text Label 6450 4400 0    40   ~ 0
D-
Text Label 6450 3800 0    40   ~ 0
RXLED
Text Label 6450 3700 0    40   ~ 0
TXLED
Text Label 5050 3600 0    40   ~ 0
RXD
Text Label 5050 3300 0    40   ~ 0
DTR
Text Label 5050 3200 0    40   ~ 0
TXD
NoConn ~ 6400 5150
$Comp
L USB-MINI-B J1
U 1 1 4CDDCB6A
P 6500 5400
F 0 "J1" H 6500 5700 60  0000 C CNN
F 1 "USB-MICRO-B" H 6500 5050 60  0000 C CNN
F 4 "FCI" H 6500 5400 60  0001 C CNN "Field1"
F 5 "10103594-0001LF" H 6500 5400 60  0001 C CNN "Field2"
F 6 "USB Connectors 5P Quick Connect Micro USB TypeB Rcpt" H 6500 5400 60  0001 C CNN "Field6"
	1    6500 5400
	0    1    1    0   
$EndComp
$Comp
L CP1 C2
U 1 1 4CDDB3EB
P 7250 4300
F 0 "C2" H 7300 4400 50  0000 L CNN
F 1 "10μF" H 7300 4200 50  0000 L CNN
F 4 "Kemet" H 7250 4300 60  0001 C CNN "Field1"
F 5 "B45196H2106K109" H 7250 4300 60  0001 C CNN "Field2"
F 6 "Tantalum Capacitors - Solid SMD 10volts 10uF 10%" H 7250 4300 60  0001 C CNN "Field3"
	1    7250 4300
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR02
U 1 1 4CDDB287
P 7550 4600
F 0 "#PWR02" H 7550 4600 30  0001 C CNN
F 1 "GND" H 7550 4530 30  0001 C CNN
	1    7550 4600
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR03
U 1 1 4CDDB285
P 7250 4600
F 0 "#PWR03" H 7250 4600 30  0001 C CNN
F 1 "GND" H 7250 4530 30  0001 C CNN
	1    7250 4600
	1    0    0    -1  
$EndComp
$Comp
L C C3
U 1 1 4CDDB263
P 7550 4300
F 0 "C3" H 7600 4400 50  0000 L CNN
F 1 "100nF" H 7600 4200 50  0000 L CNN
F 4 "TDK" H 7550 4300 60  0001 C CNN "Field1"
F 5 "C1608X7R1H104K" H 7550 4300 60  0001 C CNN "Field2"
F 6 "Multilayer Ceramic Capacitors (MLCC) - SMD/SMT 0603 0.1uF 50volts X7R 10%" H 7550 4300 60  0001 C CNN "Field3"
	1    7550 4300
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR04
U 1 1 4CDDB238
P 6300 5050
F 0 "#PWR04" H 6300 5050 30  0001 C CNN
F 1 "GND" H 6300 4980 30  0001 C CNN
	1    6300 5050
	-1   0    0    1   
$EndComp
$Comp
L C C1
U 1 1 4CDDA7E7
P 6800 4600
F 0 "C1" H 6850 4700 50  0000 L CNN
F 1 "100nF" H 6850 4500 50  0000 L CNN
F 4 "TDK" H 6800 4600 60  0001 C CNN "Field1"
F 5 "C1608X7R1H104K" H 6800 4600 60  0001 C CNN "Field2"
F 6 "Multilayer Ceramic Capacitors (MLCC) - SMD/SMT 0603 0.1uF 50volts X7R 10%" H 6800 4600 60  0001 C CNN "Field3"
	1    6800 4600
	1    0    0    -1  
$EndComp
Text Label 3500 4500 0    60   ~ 0
VBUS
$Comp
L 3V3 #PWR05
U 1 1 4CDDB137
P 2700 3950
F 0 "#PWR05" H 2700 4050 40  0001 C CNN
F 1 "3V3" H 2700 4075 40  0000 C CNN
	1    2700 3950
	1    0    0    -1  
$EndComp
$Comp
L R R2
U 1 1 4CDDB10B
P 3300 4250
F 0 "R2" V 3380 4250 50  0000 C CNN
F 1 "DNP" V 3300 4250 50  0000 C CNN
	1    3300 4250
	-1   0    0    1   
$EndComp
$Comp
L R R1
U 1 1 4CDDB0FC
P 2950 4000
F 0 "R1" V 3030 4000 50  0000 C CNN
F 1 "DNP" V 2950 4000 50  0000 C CNN
	1    2950 4000
	0    1    1    0   
$EndComp
Text Label 7750 4000 0    60   ~ 0
VBUS
$Comp
L GND #PWR06
U 1 1 4CDDB090
P 3900 3050
F 0 "#PWR06" H 3900 3050 30  0001 C CNN
F 1 "GND" H 3900 2980 30  0001 C CNN
	1    3900 3050
	1    0    0    -1  
$EndComp
$Comp
L CONN_6 P1
U 1 1 4CDDAFDF
P 4150 2600
F 0 "P1" V 4100 2600 60  0000 C CNN
F 1 "pogopins" V 4200 2600 60  0000 C CNN
F 4 "SparkFun" V 4150 2600 60  0001 C CNN "Field1"
F 5 "PRT-09174" V 4150 2600 60  0001 C CNN "Field2"
F 6 "QTY 6 Pogo Pins w/ Pointed Tip" V 4150 2600 60  0001 C CNN "Field3"
	1    4150 2600
	0    -1   -1   0   
$EndComp
$Comp
L GND #PWR07
U 1 1 4CDDA9D5
P 5000 3800
F 0 "#PWR07" H 5000 3800 30  0001 C CNN
F 1 "GND" H 5000 3730 30  0001 C CNN
	1    5000 3800
	0    1    1    0   
$EndComp
NoConn ~ 5100 4500
NoConn ~ 5100 4400
NoConn ~ 5100 4300
NoConn ~ 5100 4100
NoConn ~ 5100 4000
NoConn ~ 5100 3700
NoConn ~ 6400 3300
NoConn ~ 6400 3200
NoConn ~ 6400 4100
NoConn ~ 6400 3600
NoConn ~ 5100 3900
$Comp
L GND #PWR08
U 1 1 4CDDA830
P 6800 4900
F 0 "#PWR08" H 6800 4900 30  0001 C CNN
F 1 "GND" H 6800 4830 30  0001 C CNN
	1    6800 4900
	1    0    0    -1  
$EndComp
$Comp
L 3V3 #PWR09
U 1 1 4CDDA736
P 6800 4250
F 0 "#PWR09" H 6800 4350 40  0001 C CNN
F 1 "3V3" H 6800 4375 40  0000 C CNN
	1    6800 4250
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR010
U 1 1 4CDDA68D
P 6500 3400
F 0 "#PWR010" H 6500 3400 30  0001 C CNN
F 1 "GND" H 6500 3330 30  0001 C CNN
	1    6500 3400
	0    -1   -1   0   
$EndComp
$Comp
L GND #PWR011
U 1 1 4CDDA689
P 6500 3500
F 0 "#PWR011" H 6500 3500 30  0001 C CNN
F 1 "GND" H 6500 3430 30  0001 C CNN
	1    6500 3500
	0    -1   -1   0   
$EndComp
$Comp
L GND #PWR012
U 1 1 4CDDA688
P 6500 3900
F 0 "#PWR012" H 6500 3900 30  0001 C CNN
F 1 "GND" H 6500 3830 30  0001 C CNN
	1    6500 3900
	0    -1   -1   0   
$EndComp
$Comp
L GND #PWR013
U 1 1 4CDDA67E
P 6500 4200
F 0 "#PWR013" H 6500 4200 30  0001 C CNN
F 1 "GND" H 6500 4130 30  0001 C CNN
	1    6500 4200
	0    -1   -1   0   
$EndComp
$Comp
L 3V3 #PWR014
U 1 1 4CDDA64A
P 7100 2650
F 0 "#PWR014" H 7100 2750 40  0001 C CNN
F 1 "3V3" H 7100 2775 40  0000 C CNN
	1    7100 2650
	1    0    0    -1  
$EndComp
$Comp
L 3V3 #PWR015
U 1 1 4CDDA646
P 6800 2650
F 0 "#PWR015" H 6800 2750 40  0001 C CNN
F 1 "3V3" H 6800 2775 40  0000 C CNN
	1    6800 2650
	1    0    0    -1  
$EndComp
$Comp
L R R4
U 1 1 4CDDA631
P 7100 2950
F 0 "R4" V 7180 2950 50  0000 C CNN
F 1 "330" V 7100 2950 50  0000 C CNN
F 4 "Bourns" V 7100 2950 60  0001 C CNN "Field1"
F 5 "CR0603-JW-331ELF" V 7100 2950 60  0001 C CNN "Field2"
F 6 "Thick Film Resistors - SMD 0603 330OHMS 5% 1/10WATT" V 7100 2950 60  0001 C CNN "Field3"
	1    7100 2950
	1    0    0    -1  
$EndComp
$Comp
L R R3
U 1 1 4CDDA629
P 6800 2950
F 0 "R3" V 6880 2950 50  0000 C CNN
F 1 "330" V 6800 2950 50  0000 C CNN
F 4 "Bourns" V 6800 2950 60  0001 C CNN "Field1"
F 5 "CR0603-JW-331ELF" V 6800 2950 60  0001 C CNN "Field2"
F 6 "Thick Film Resistors - SMD 0603 330OHMS 5% 1/10WATT" V 6800 2950 60  0001 C CNN "Field3"
	1    6800 2950
	1    0    0    -1  
$EndComp
$Comp
L LED D2
U 1 1 4CDDA621
P 7100 3500
F 0 "D2" H 7100 3600 50  0000 C CNN
F 1 "RXLED" H 7100 3400 50  0000 C CNN
F 4 "Kingbright" H 7100 3500 60  0001 C CNN "Field1"
F 5 "APT1608EC" H 7100 3500 60  0001 C CNN "Field2"
F 6 "Standard LED - SMD HI EFF RED WTR CLR" H 7100 3500 60  0001 C CNN "Field3"
	1    7100 3500
	0    1    1    0   
$EndComp
$Comp
L LED D1
U 1 1 4CDDA608
P 6800 3500
F 0 "D1" H 6800 3600 50  0000 C CNN
F 1 "TXLED" H 6800 3400 50  0000 C CNN
F 4 "Kingbright" H 6800 3500 60  0001 C CNN "Field1"
F 5 "APT1608SGC" H 6800 3500 60  0001 C CNN "Field2"
F 6 "Standard LED - SMD GREEN WATER CLEAR" H 6800 3500 60  0001 C CNN "Field3"
	1    6800 3500
	0    1    1    0   
$EndComp
$Comp
L FT232RL U1
U 1 1 4CDDA5D6
P 5750 3850
F 0 "U1" H 5750 4600 60  0000 C CNN
F 1 "FT232RL" H 5750 3100 60  0000 C CNN
F 4 "FTDI" H 5750 3850 60  0001 C CNN "Field1"
F 5 "FT232RL" H 5750 3850 60  0001 C CNN "Field2"
F 6 "USB Interface IC USB to Serial UART Enhanced IC SSOP-28" H 5750 3850 60  0001 C CNN "Field3"
	1    5750 3850
	1    0    0    -1  
$EndComp
$EndSCHEMATC
