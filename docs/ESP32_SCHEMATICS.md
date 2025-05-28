# ESP32 Sensor Node Schematics
*May 28, 2025*

## Circuit Diagrams

### Power Supply Circuit

```
USB-C Connector (J1)
Pin 1 (GND) ----+
                |
Pin 5 (VBUS) ---+----> TP4056 (U1)
                |      Pin 1 (TEMP)  --> GND via 10kΩ (R1)
                |      Pin 2 (PROG)  --> GND via 2kΩ (R2) [1A charge current]
                |      Pin 3 (GND)   --> GND
                |      Pin 4 (VDD)   --> VBUS
                |      Pin 5 (BAT)   --> Battery+ (BT1)
                |      Pin 6 (CHRG)  --> Charge LED (D1) cathode
                |      Pin 7 (STDBY) --> Standby LED (D2) cathode
                |      Pin 8 (CE)    --> VDD [Enable charging]
                |
                +----> Power Switch (SW1)
                       |
                       +--> AMS1117-3.3 (U2)
                            Pin 1 (GND) --> GND
                            Pin 2 (OUT) --> 3V3 Rail
                            Pin 3 (IN)  --> Battery+
                            
                       C1: 100μF Electrolytic (Input)
                       C2: 10μF Ceramic (Output)

Battery (BT1): 3.7V Li-Po 2000mAh
- Positive --> TP4056 Pin 5, Power Switch
- Negative --> GND
```

### ESP32 Main Circuit

```
ESP32-WROOM-32D (U3)
                    VDD3P3 Pins (2,3) --> 3V3
                    GND Pins (1,15,38,39) --> GND
                    EN (3) --> 3V3 via 10kΩ (R3)
                    
GPIO Assignments:
    Pin 25 (GPIO21) --> I2C SDA
    Pin 26 (GPIO22) --> I2C SCL
    Pin 10 (GPIO25) --> I2S WS (INMP441)
    Pin 11 (GPIO26) --> I2S SCK (INMP441)
    Pin 12 (GPIO27) --> I2S SD (INMP441)
    Pin 24 (GPIO2)  --> Status LED (D3) anode
    Pin 23 (GPIO0)  --> Boot Button (SW2)
    Pin 26 (GPIO4)  --> User Button (SW3)
    Pin 4 (SENSOR_VP) --> Battery Voltage Monitor

Boot Circuit:
    GPIO0 (Pin 23) --> 10kΩ (R4) --> 3V3
    GPIO0 (Pin 23) --> Boot Button (SW2) --> GND
    
Reset Circuit:
    EN (Pin 3) --> 0.1μF (C3) --> GND
    EN (Pin 3) --> Reset Button (SW4) --> GND
```

### I2C Sensor Network

```
I2C Bus:
    SDA (GPIO21) ---+---4.7kΩ (R5) --- 3V3
                    |
    SCL (GPIO22) ---+---4.7kΩ (R6) --- 3V3
                    |
                    +--- All sensor SDA pins
                    |
                    +--- All sensor SCL pins

BME280 Breakout (U4):
    VCC --> 3V3
    GND --> GND
    SDA --> I2C SDA
    SCL --> I2C SCL
    [I2C Address: 0x76]

VEML7700 Breakout (U5):
    VCC --> 3V3
    GND --> GND
    SDA --> I2C SDA
    SCL --> I2C SCL
    [I2C Address: 0x10]

MPU6050 Breakout (U6):
    VCC --> 3V3
    GND --> GND
    SDA --> I2C SDA
    SCL --> I2C SCL
    INT --> GPIO35 (optional)
    [I2C Address: 0x68]
```

### Audio Input Circuit

```
INMP441 Digital Microphone (U7):
    VDD --> 3V3
    GND --> GND
    WS  --> GPIO25 (I2S Word Select)
    SCK --> GPIO26 (I2S Serial Clock)
    SD  --> GPIO27 (I2S Serial Data)
    L/R --> GND [Left channel selection]

Decoupling:
    0.1μF (C4) between VDD and GND
```

### LED and Button Circuit

```
Status LED (D3):
    Anode  --> GPIO2 (ESP32)
    Cathode --> 220Ω (R7) --> GND

Charge LED (D1):
    Anode  --> 3V3 via 1kΩ (R8)
    Cathode --> TP4056 CHRG pin

Standby LED (D2):
    Anode  --> 3V3 via 1kΩ (R9)
    Cathode --> TP4056 STDBY pin

User Button (SW3):
    One side --> GPIO4
    Other side --> GND
    GPIO4 --> 10kΩ (R10) --> 3V3

Power Button (SW1):
    SPST switch in battery positive line
```

## Component List (BOM)

### Active Components
| Reference | Component | Package | Value/Part Number | Quantity |
|-----------|-----------|---------|-------------------|----------|
| U1 | TP4056 | SOP-8 | TP4056 Li-Po Charger | 1 |
| U2 | AMS1117-3.3 | SOT-223 | 3.3V LDO Regulator | 1 |
| U3 | ESP32-WROOM-32D | Module | ESP32 WiFi Module | 1 |
| U4 | BME280 | Breakout | Temp/Humidity/Pressure | 1 |
| U5 | VEML7700 | Breakout | Light Sensor | 1 |
| U6 | MPU6050 | Breakout | Accel/Gyro | 1 |
| U7 | INMP441 | Breakout | I2S Microphone | 1 |

### Passive Components
| Reference | Component | Package | Value | Quantity |
|-----------|-----------|---------|-------|----------|
| R1 | Resistor | 0805 | 10kΩ | 1 |
| R2 | Resistor | 0805 | 2kΩ | 1 |
| R3-R6, R10 | Resistor | 0805 | 10kΩ | 5 |
| R7 | Resistor | 0805 | 220Ω | 1 |
| R8-R9 | Resistor | 0805 | 1kΩ | 2 |
| C1 | Electrolytic Cap | Radial | 100μF 16V | 1 |
| C2 | Ceramic Cap | 0805 | 10μF 16V | 1 |
| C3-C4 | Ceramic Cap | 0805 | 0.1μF 50V | 2 |

### Mechanical Components
| Reference | Component | Description | Quantity |
|-----------|-----------|-------------|----------|
| J1 | USB-C Connector | Through-hole or SMD | 1 |
| SW1 | Power Switch | SPST Slide Switch | 1 |
| SW2-SW4 | Tactile Button | 6x6mm Through-hole | 3 |
| D1-D3 | LED | 3mm Through-hole | 3 |
| BT1 | Battery Connector | JST-PH 2-pin | 1 |

## PCB Layout Guidelines

### Layer Stack-up (4-layer)
1. **Top Layer (Signal)**: Component placement and high-speed signals
2. **Layer 2 (Ground)**: Solid ground plane
3. **Layer 3 (Power)**: 3.3V power plane with cutouts
4. **Bottom Layer (Signal)**: Secondary routing and crystal circuits

### Design Rules
- **Trace Width**: 
  - Power: 0.5mm minimum (1A current)
  - Signal: 0.2mm minimum
  - High-speed: 0.15mm with controlled impedance
- **Via Size**: 0.2mm drill, 0.4mm pad
- **Clearance**: 0.15mm minimum
- **Drill Size**: 0.1mm minimum

### Critical Layout Considerations

#### Power Supply Layout
```
TP4056 → AMS1117 → ESP32
  |         |        |
  |         |        +-- Local 0.1μF decoupling
  |         +-- 10μF output capacitor
  +-- 100μF input capacitor

- Wide power traces (≥1mm)
- Short connections between regulator and capacitors
- Star ground connection at battery negative
```

#### High-Speed Digital Layout
```
ESP32 Crystal Circuit:
- Keep crystal close to ESP32 (≤5mm)
- Minimize trace lengths
- Guard with ground traces
- Place on same layer as ESP32

I2S Audio Traces:
- Match trace lengths (±0.5mm)
- Route on single layer
- 50Ω characteristic impedance
- Minimize crosstalk with 3x trace width spacing
```

#### Antenna Layout
```
ESP32 Antenna Area:
- 15mm x 5mm keepout area around antenna
- No copper pour under antenna
- No traces crossing antenna area
- Ground plane cutout extending 2mm beyond antenna
```

### Component Placement Strategy

#### Top Layer Placement
```
+------------------------------------------+
|  [USB-C]  [PWR_LED]  [CHG_LED]  [SW1]   |
|                                          |
|  [TP4056] [AMS1117]     [ESP32-WROOM]   |
|     |         |              |          |
|   [C1]      [C2]          [Antenna]     |
|                            Area         |
|  [Battery   [BME280]                     |
|   Conn]     [VEML7700]                   |
|             [SGP30]        [STATUS_LED]  |
|                                          |
|  [MPU6050]  [LIS3MDL]     [INMP441]     |
|                                          |
|  [SW2]      [SW3]         [SW4]          |
|  Boot       User          Reset          |
+------------------------------------------+
```

## Assembly Instructions

### SMD Assembly Order
1. **Smallest components first**: Resistors, capacitors
2. **ICs**: TP4056, AMS1117 
3. **ESP32 module**: Use hot air or reflow oven
4. **Breakout modules**: Hand solder or use sockets

### Through-hole Assembly
1. **Connectors**: USB-C, Battery connector
2. **Switches and buttons**: Power switch, tactile buttons
3. **LEDs**: Status, charge, standby indicators
4. **Headers**: Programming and expansion headers

### Testing Procedure
1. **Visual inspection**: Check for solder bridges, component orientation
2. **Power test**: Verify 3.3V output with multimeter
3. **Programming test**: Upload test firmware via USB
4. **Sensor test**: Verify I2C communication with all sensors
5. **Audio test**: Check I2S microphone data stream
6. **WiFi test**: Verify wireless connectivity
7. **Battery test**: Check charging circuit and battery monitoring

## Manufacturing Files

### Gerber Files Required
- Top Copper (GTL)
- Bottom Copper (GBL)
- Top Solder Mask (GTS)
- Bottom Solder Mask (GBS)
- Top Silk Screen (GTO)
- Bottom Silk Screen (GBO)
- Drill File (TXT)
- Pick and Place (CSV)

### Documentation Files
- Assembly Drawing (PDF)
- Fabrication Drawing (PDF)
- Bill of Materials (CSV/Excel)
- Component Placement Report (CSV)

## Design for Manufacturing (DFM)

### PCB Specifications
- **Board Size**: 60mm x 40mm
- **Board Thickness**: 1.6mm
- **Copper Weight**: 1oz (35μm)
- **Solder Mask**: Green LPI
- **Silkscreen**: White LPI
- **Surface Finish**: HASL or ENIG
- **Min Hole Size**: 0.2mm
- **Min Track Width**: 0.15mm
- **Min Via Size**: 0.4mm

### Assembly Considerations
- **Component Orientation**: All polarized components clearly marked
- **Fiducial Markers**: At least 3 fiducials for pick-and-place
- **Test Points**: Accessible for in-circuit testing
- **Panel Size**: Optimize for PCB fabrication panel sizes
- **Tooling Holes**: 3mm diameter for assembly fixtures