# Bill of materials

Prototype as built (July 2016 photos in `hardware/photos/`) plus the parts named in firmware and datasheets.

| Qty | Item | Notes |
|-----|------|--------|
| 1 | Waterproof junction box | Through-wall male pipe threads on two short sides; lid with gasket. Photos show a black ABS-style enclosure. |
| 1 | RPE R-series valve body | Plastic meter/valve block, male threads both ends. |
| 1 | RPE BVS_RF-3-30L flow meter | 3–30 L/min, 245 pulses/L Hall or 490 Reed. Datasheet: `hardware/datasheets/BVS_RF-3-30L.pdf`. |
| 1 | RPE R-series solenoid | **Confirm SKU.** Firmware pulse is 25 ms, which matches the latching L6V coil, not a 100% ED holding coil. Datasheet: `hardware/datasheets/RPE Coils Data Sheet.pdf`. |
| 2 | Stainless pipe nipples / adapters | Visible in prototype photos. |
| 1 | Microduino Core+ (ATmega644PA) | 4 KB SRAM, Serial1 on D2/D3. Schematic: `hardware/schematics/Microduino-Core+/`. |
| 1 | Microduino GSM SIM800 shield | UART to Core+ Serial1. Schematic: `hardware/schematics/Microduino-GSMShield/`. |
| 1 | GSM antenna + SMA pigtail | Routed out of the box in photos. Seal the hole. |
| 1 | L9110 dual H-bridge | Drives the latching coil. 2.5–12 V, ~800 mA continuous/channel. `hardware/datasheets/Datasheet-l9110.pdf`. Not populated on the proto island in `20160727_200548.jpg`. |
| 1 | Perfboard / proto island | Right-hand 3D-printed cup in the prototype. |
| 2 | 3D-printed module cups | Hold the Microduino stack and protoboard off the box floor. Prefer lid-mount as in `hardware/renders/`. |
| 1 | 2G SIM (Tigo M2M in the original sketch) | See `docs/2G-EOL.md` before buying more. |
| 1 | Power source | Lab supply in the prototype. A field unit needs a sized battery/solar pack; the radio is no longer held up continuously, but 2G attach is still expensive. |

Hall pickup wiring (black JST B3P): pin 1 output, pin 2 negative, pin 3 positive (5–28 V). Firmware enables the AVR pull-up on `PIN_FLOW`.
