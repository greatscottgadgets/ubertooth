EESchema Schematic File Version 2  date Sat 10 Sep 2011 08:44:03 PM MDT
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
LIBS:ubertooth-symbols
LIBS:endive-cache
EELAYER 25  0
EELAYER END
$Descr A4 11700 8267
encoding utf-8
Sheet 1 1
Title ""
Date "10 sep 2011"
Rev "$Rev$"
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
Text Label 7700 3000 2    40   ~ 0
VCC
Text Label 6500 3000 0    40   ~ 0
VIN
Wire Wire Line
	7600 3000 7700 3000
Wire Wire Line
	5950 2550 5950 2650
Wire Wire Line
	6900 5050 6900 5250
Wire Wire Line
	7200 4450 7200 4550
Wire Wire Line
	6600 4450 6600 4550
Connection ~ 6600 3750
Wire Wire Line
	6900 3750 6900 4050
Connection ~ 6250 3850
Wire Wire Line
	6250 2550 6250 4500
Wire Wire Line
	6250 4500 6050 4500
Connection ~ 5050 4700
Wire Wire Line
	5050 4700 5250 4700
Connection ~ 5050 5000
Wire Wire Line
	5050 5000 5250 5000
Wire Wire Line
	5050 5100 5250 5100
Wire Wire Line
	5250 4500 5050 4500
Connection ~ 6150 4050
Wire Wire Line
	6150 2550 6150 4800
Wire Wire Line
	6150 4800 6050 4800
Connection ~ 6000 3500
Wire Wire Line
	6000 3200 6000 3500
Wire Wire Line
	5850 3950 5850 3400
Wire Wire Line
	5850 3950 5050 3950
Connection ~ 6150 3500
Wire Wire Line
	5050 4050 6150 4050
Connection ~ 6250 3300
Wire Wire Line
	5050 3850 6250 3850
Connection ~ 5750 3600
Wire Wire Line
	5050 4150 5750 4150
Wire Wire Line
	5750 3600 5050 3600
Wire Wire Line
	6250 3300 5050 3300
Wire Wire Line
	5050 3500 6150 3500
Wire Wire Line
	5850 3400 5050 3400
Wire Wire Line
	6050 2550 6050 2750
Wire Wire Line
	6050 2750 6000 2750
Wire Wire Line
	6000 2750 6000 2800
Wire Wire Line
	5750 4250 5750 2550
Connection ~ 5750 4150
Wire Wire Line
	5150 4800 5250 4800
Wire Wire Line
	5050 4500 5050 5200
Connection ~ 5050 5100
Wire Wire Line
	5050 4900 5250 4900
Connection ~ 5050 4900
Wire Wire Line
	5050 4600 5250 4600
Connection ~ 5050 4600
Wire Wire Line
	6600 4050 6600 3750
Connection ~ 5850 3750
Wire Wire Line
	5850 3750 7200 3750
Wire Wire Line
	7200 3750 7200 4050
Connection ~ 6900 3750
Wire Wire Line
	6900 4450 6900 4550
Wire Wire Line
	6600 5050 6600 5150
Wire Wire Line
	7200 5050 7200 5250
Wire Wire Line
	6600 3000 6500 3000
$Comp
L SPST SW1
U 1 1 4E6BCC00
P 7100 3000
F 0 "SW1" H 7100 3100 70  0000 C CNN
F 1 "SPST" H 7100 2900 70  0000 C CNN
	1    7100 3000
	1    0    0    -1  
$EndComp
Text Label 5950 2650 1    40   ~ 0
VIN
Text Label 5150 4800 0    40   ~ 0
VIN
Text Label 7200 5250 1    40   ~ 0
MODE
Text Label 6900 5250 1    40   ~ 0
RESET
$Comp
L GND #PWR01
U 1 1 4E6BCA6D
P 6600 5150
F 0 "#PWR01" H 6600 5150 30  0001 C CNN
F 1 "GND" H 6600 5080 30  0001 C CNN
	1    6600 5150
	1    0    0    -1  
$EndComp
$Comp
L R R3
U 1 1 4E6BCA69
P 7200 4800
F 0 "R3" V 7280 4800 50  0000 C CNN
F 1 "330" V 7200 4800 50  0000 C CNN
	1    7200 4800
	1    0    0    -1  
$EndComp
$Comp
L R R2
U 1 1 4E6BCA67
P 6900 4800
F 0 "R2" V 6980 4800 50  0000 C CNN
F 1 "330" V 6900 4800 50  0000 C CNN
	1    6900 4800
	1    0    0    -1  
$EndComp
$Comp
L R R1
U 1 1 4E6BCA63
P 6600 4800
F 0 "R1" V 6680 4800 50  0000 C CNN
F 1 "330" V 6600 4800 50  0000 C CNN
	1    6600 4800
	1    0    0    -1  
$EndComp
$Comp
L LED D4
U 1 1 4E6BCA5F
P 7200 4250
F 0 "D4" H 7200 4350 50  0000 C CNN
F 1 "MODELED" H 7200 4150 50  0000 C CNN
	1    7200 4250
	0    1    1    0   
$EndComp
$Comp
L LED D3
U 1 1 4E6BCA5A
P 6900 4250
F 0 "D3" H 6900 4350 50  0000 C CNN
F 1 "RESETLED" H 6900 4150 50  0000 C CNN
	1    6900 4250
	0    1    1    0   
$EndComp
$Comp
L LED D2
U 1 1 4E6BCA4A
P 6600 4250
F 0 "D2" H 6600 4350 50  0000 C CNN
F 1 "VCCLED" H 6600 4150 50  0000 C CNN
	1    6600 4250
	0    1    1    0   
$EndComp
Text Notes 3750 4850 0    60   ~ 0
connect to Renesas E8a
Text Notes 4350 2200 0    60   ~ 0
connect to Pogoprog or\nsimilar FTDI serial board
Text Notes 3250 3400 0    60   ~ 0
two ways to connect to\nDandelion's E8a connector:\npin header or pogo pins
Text Label 5050 3600 0    40   ~ 0
GND
Text Label 5050 3500 0    40   ~ 0
MODE
Text Label 5050 3400 0    40   ~ 0
VCC
Text Label 5050 3300 0    40   ~ 0
RESET
NoConn ~ 6050 5100
NoConn ~ 6050 5000
NoConn ~ 6050 4900
NoConn ~ 6050 4700
NoConn ~ 6050 4600
$Comp
L GND #PWR02
U 1 1 4E6BC672
P 5050 5200
F 0 "#PWR02" H 5050 5200 30  0001 C CNN
F 1 "GND" H 5050 5130 30  0001 C CNN
	1    5050 5200
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR03
U 1 1 4E6BC611
P 5750 4250
F 0 "#PWR03" H 5750 4250 30  0001 C CNN
F 1 "GND" H 5750 4180 30  0001 C CNN
	1    5750 4250
	1    0    0    -1  
$EndComp
NoConn ~ 5850 2550
$Comp
L CONN_4 P2
U 1 1 4E6BC46E
P 4700 4000
F 0 "P2" V 4650 4000 50  0000 C CNN
F 1 "CONN_4" V 4750 4000 50  0000 C CNN
	1    4700 4000
	-1   0    0    -1  
$EndComp
$Comp
L CONN_7X2 P3
U 1 1 4E6BC429
P 5650 4800
F 0 "P3" H 5650 5200 60  0000 C CNN
F 1 "CONN_7X2" V 5650 4800 60  0000 C CNN
	1    5650 4800
	-1   0    0    1   
$EndComp
$Comp
L CONN_4 P1
U 1 1 4E6BC41A
P 4700 3450
F 0 "P1" V 4650 3450 50  0000 C CNN
F 1 "CONN_4" V 4750 3450 50  0000 C CNN
	1    4700 3450
	-1   0    0    -1  
$EndComp
$Comp
L CONN_6 P4
U 1 1 4E6BC40B
P 6000 2200
F 0 "P4" V 5950 2200 60  0000 C CNN
F 1 "CONN_6" V 6050 2200 60  0000 C CNN
	1    6000 2200
	0    -1   -1   0   
$EndComp
$Comp
L DIODESCH D1
U 1 1 4E6BC3BB
P 6000 3000
F 0 "D1" H 6000 3100 40  0000 C CNN
F 1 "DIODESCH" H 6000 2900 40  0000 C CNN
	1    6000 3000
	0    -1   -1   0   
$EndComp
$EndSCHEMATC
