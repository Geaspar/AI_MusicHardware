# ESP32 Sensor Node Enclosure Design
*May 28, 2025*

## Design Overview

The ESP32 sensor node enclosure is designed for both indoor and outdoor environmental monitoring applications. It provides protection while allowing proper sensor operation and easy access for maintenance.

## Specifications

### Physical Dimensions
- **Outer Dimensions**: 70mm (L) x 50mm (W) x 25mm (H)
- **Inner Dimensions**: 65mm (L) x 45mm (W) x 20mm (H)
- **Wall Thickness**: 2.5mm
- **Material**: ABS plastic or PETG (3D printing)
- **Color**: Light gray or white (UV resistance)

### Environmental Protection
- **IP Rating**: IP54 (Dust protected, splash resistant)
- **Operating Temperature**: -20°C to +60°C
- **Storage Temperature**: -40°C to +80°C
- **Humidity**: 0-95% RH (non-condensing)
- **UV Resistance**: UV stabilized material

## Enclosure Features

### Top Cover Design

```
Top Cover (70mm x 50mm x 12mm)
+---------------------------------------+
|  ○ ○ ○     [Clear Window]      ○ ○ ○  |  ← Ventilation holes
|                                       |    for environmental
|  ○ ○ ○       10x8mm          ○ ○ ○   |    sensors (BME280)
|                                       |
|    [Light Sensor Window]              |  ← Clear acrylic window
|         5x5mm                         |    for VEML7700
|                                       |
|              [Status LED]             |  ← Light pipe for
|                ●                      |    status indication
|                                       |
+---------------------------------------+

Features:
- 12 x Ø2mm ventilation holes (Ø1.5mm internal)
- 1 x 10mm x 8mm clear acrylic window for light sensor
- 1 x Ø3mm light pipe for status LED
- Raised lips around ventilation for water deflection
```

### Bottom Case Design

```
Bottom Case (70mm x 50mm x 13mm)
+---------------------------------------+
|                                       |
|  [PCB Mounting Posts]                 |
|    ●               ●                  |  ← M2.5 threaded inserts
|                                       |    or molded bosses
|                                       |
|  ●    [PCB Area]           ●          |
|      65mm x 40mm                      |
|                                       |
|       [Battery Compartment]           |
|        [USB-C Access Port]            |  ← Recessed opening
|             ▬▬▬▬▬                     |    9mm x 3.5mm
|                                       |
+---------------------------------------+

Features:
- 4 x PCB mounting posts (M2.5 x 6mm)
- Recessed USB-C port access
- Battery cable routing channel
- Integrated mounting tabs
```

### Side Features

```
Left Side View:
+----------------+
|                |  ← Top cover
|                |
|████████████████|  ← Gasket groove
|                |
|   [Microphone  |  ← 3mm hole with
|    Acoustic    |    acoustic mesh
|    Port] ●     |    for INMP441
|                |
|                |  ← Bottom case
+----------------+

Right Side View:
+----------------+
|                |
|                |
|████████████████|  ← Gasket groove
|                |
|  [Power Button]|  ← Flush button
|       ●        |    access
|                |
|                |
+----------------+
```

## Component Access

### USB-C Port Access
```
USB-C Port Recess:
- Opening: 12mm x 5mm
- Depth: 3mm recess
- Chamfer: 0.5mm x 45°
- Cable strain relief groove
- Water drainage channel

Cross-section view:
     ┌─────┐
  ┌──┤ USB ├──┐
──┤  └─────┘  ├─  ← PCB level
  └───┬───┬───┘
      │ ▼ │      ← Drainage holes (2 x Ø1mm)
      └───┘
```

### Button Access
```
Power Button:
- Actuator: Ø6mm rubber cap
- Travel: 1.5mm
- Force: 2.5N activation
- Sealing: O-ring around shaft

User Button:
- Access through removable plug
- Magnetic attachment for plug
- Emergency access capability
```

## Mounting Solutions

### Wall Mount Option
```
Wall Mount Bracket:
+-------------------+
|  ○             ○  |  ← Mounting holes for
|                   |    #8 screws (4mm)
|   ┌───────────┐   |
|   │           │   |  ← Enclosure clips
|   │  Enclosure │   |    (bayonet mount)
|   │    Area    │   |
|   │           │   |
|   └───────────┘   |
|                   |
|  ○             ○  |
+-------------------+

Material: ABS or Aluminum
Finish: Powder coated white
Dimensions: 90mm x 70mm x 15mm
```

### Magnetic Mount Option
```
Magnetic Base:
- 4 x Neodymium magnets (Ø10mm x 3mm)
- Holding force: 15N per magnet
- Steel backing plate for distribution
- Foam padding to prevent scratching
- Quick-release lever mechanism

Installation:
┌─────────────┐
│   Enclosure │
└──────┬──────┘
       │
  ┌────▼────┐
  │ Magnetic│  ← Rotates to lock/unlock
  │  Base   │
  └─────────┘
     │││││     ← Magnetic field
  ═══════════  ← Metal surface
```

### Tripod Mount Option
```
Tripod Adapter:
- Standard 1/4"-20 threaded insert
- Integrated into bottom case
- Reinforced mounting boss
- Compatible with camera tripods
- 360° rotation capability

Thread Specification:
- Thread: 1/4"-20 UNC
- Depth: 8mm minimum engagement
- Material: Brass threaded insert
- Torque: 5 Nm maximum
```

## Gasket and Sealing

### Gasket Design
```
Gasket Groove Specification:
- Width: 3mm
- Depth: 1.5mm
- Material: Silicone rubber (Shore A 50)
- Color: Black or clear
- Continuous seal around perimeter

Cross-section:
 Top Cover
┌─────────────┐
│      ┌──────┤  ← Gasket groove
│      │ ▓▓▓▓ │  ← Silicone gasket
│      └──────┤
└─────────────┘
  Bottom Case

Compression: 25% (0.375mm compressed height)
```

### Critical Sealing Points
1. **USB-C Port**: Double seal with gasket and plug
2. **Button Shaft**: O-ring seal
3. **Cover Junction**: Continuous perimeter gasket
4. **Ventilation Holes**: Hydrophobic membrane backing
5. **Light Window**: Silicone adhesive seal

## Ventilation System

### Environmental Sensor Ventilation
```
Ventilation Hole Array:
○ ○ ○     ○ ○ ○
○ ○ ○     ○ ○ ○

Specifications:
- 12 holes total
- Ø2mm external, Ø1.5mm internal
- 3mm center-to-center spacing
- Chamfered edges (0.2mm x 45°)
- Hydrophobic PTFE membrane backing

Membrane Specification:
- Material: ePTFE (expanded PTFE)
- Thickness: 0.2mm
- Pore size: 0.2μm
- Water resistance: >1000mm H2O
- Air permeability: >50 L/min/dm²
```

### Pressure Equalization
```
Pressure Relief Valve:
- Location: Bottom case corner
- Type: Umbrella valve
- Cracking pressure: 50 Pa
- Flow rate: 0.1 L/min at 100 Pa
- Temperature range: -40°C to +85°C

Function:
- Prevents internal pressure buildup
- Allows temperature-induced volume changes
- Maintains sensor accuracy
- Prevents moisture condensation
```

## Manufacturing Specifications

### 3D Printing Parameters
```
Material Options:
1. ABS (Recommended for outdoor use)
   - Layer height: 0.2mm
   - Infill: 40%
   - Print speed: 50mm/s
   - Nozzle temp: 250°C
   - Bed temp: 100°C

2. PETG (Good chemical resistance)
   - Layer height: 0.25mm
   - Infill: 35%
   - Print speed: 40mm/s
   - Nozzle temp: 240°C
   - Bed temp: 80°C

3. ASA (UV stable outdoor use)
   - Layer height: 0.2mm
   - Infill: 45%
   - Print speed: 45mm/s
   - Nozzle temp: 260°C
   - Bed temp: 110°C
```

### Injection Molding (Production)
```
Material: ABS + UV Stabilizer
Shrinkage: 0.5-0.7%
Draft Angle: 1° minimum
Wall Thickness: 2.5mm ± 0.2mm
Tolerance: ± 0.1mm for critical dimensions

Mold Features:
- Core/cavity construction
- Side actions for undercuts
- Ejector pins in non-critical areas
- Texture: VDI 24 (light texture)
- Gate location: Hidden on bottom
```

### Post-Processing
```
Required Operations:
1. Support removal and surface finishing
2. Drilling accurate holes for hardware
3. Threaded insert installation (heat/ultrasonic)
4. Gasket groove cleaning and inspection
5. Surface treatment (UV coating for outdoor use)
6. Quality inspection and dimensional verification

Hardware Installation:
- M2.5 threaded inserts: Heat installation at 200°C
- Light pipe: Press fit with adhesive backup
- Gasket: Stretch installation into groove
- Membrane: Adhesive lamination over vent holes
```

## Assembly Instructions

### Step-by-Step Assembly
```
1. PCB Installation:
   - Install threaded inserts in bottom case
   - Place PCB with components facing up
   - Secure with M2.5 x 6mm screws
   - Route battery cable through channel

2. Component Integration:
   - Install status LED light pipe
   - Position acoustic mesh over microphone port
   - Apply hydrophobic membrane over vent holes
   - Install clear window with silicone sealant

3. Gasket Installation:
   - Clean gasket groove thoroughly
   - Install continuous silicone gasket
   - Ensure proper seating in groove
   - Check for cuts or gaps

4. Final Assembly:
   - Apply thin layer of silicone grease to gasket
   - Align top and bottom case halves
   - Press together evenly around perimeter
   - Secure with 6 x M3 x 12mm screws

5. Testing:
   - Power-on test for functionality
   - LED operation verification
   - Button function check
   - Seal integrity test (pressure decay)
```

## Quality Control

### Inspection Points
```
Dimensional Checks:
□ Overall dimensions ± 0.2mm
□ USB-C port opening: 12mm x 5mm ± 0.1mm
□ Mounting hole positions ± 0.1mm
□ Gasket groove width: 3mm ± 0.1mm
□ Ventilation hole diameter: 2mm ± 0.1mm

Functional Tests:
□ Gasket compression test
□ Button actuation force: 2.5N ± 0.5N
□ LED light transmission
□ Microphone acoustic path
□ USB cable insertion/retention

Environmental Tests:
□ Water spray test (IP54 verification)
□ Temperature cycling: -20°C to +60°C
□ UV exposure: 1000 hours accelerated
□ Vibration test: 10G, 10-2000Hz
□ Drop test: 1.5m onto concrete
```

### Acceptance Criteria
- **Sealing**: No water ingress during IP54 spray test
- **Mechanical**: All buttons function through full life cycle
- **Optical**: >90% light transmission through window
- **Acoustic**: <3dB attenuation for microphone path
- **Durability**: No cracks or damage after environmental testing

## Cost Analysis

### Prototype Costs (10 units)
- 3D Printing: $15/unit
- Hardware (screws, inserts): $3/unit
- Gasket and seals: $2/unit
- Light pipe and window: $2/unit
- **Total per unit**: $22

### Low Volume Production (100 units)
- Injection molded parts: $8/unit
- Hardware and seals: $2/unit
- Assembly labor: $3/unit
- **Total per unit**: $13

### High Volume Production (1000+ units)
- Injection molded parts: $4/unit
- Hardware and seals: $1.50/unit
- Assembly labor: $1.50/unit
- **Total per unit**: $7

## Design Files

### CAD Files Required
- SolidWorks/Fusion 360 native files
- STEP files for manufacturing
- STL files for 3D printing
- 2D drawings with GD&T
- Assembly drawings with BOM
- Exploded view for documentation

### Manufacturing Drawings
- Part drawings with tolerances
- Assembly sequence diagrams
- Hardware specification sheets
- Material and finish specifications
- Packaging and shipping requirements