# CMPE_246_CNC-Real-Time-Force-Telemetry-and-Compensation
Lowrider V4 CNC Real Time Force Telemetry and Error Avoidance.

Purpose:
This project adds force feedback to the axes of the Lowrider v4 cnc machine. At this point, the system detects the strain on each axis and aborts the operation if a force exceeds a given threshold - indicating the position is likely lost and the machine must be rehomed. 

Potential next steps:
Use experimental deflection data to compensate machine path in real time to improve machining accuracy. Additianally you can adjust the speed of the machine in real time based on force feedback data to keep the machine operating at maximum speed without exceeding the maximum output of the stepper motors.

Setup:
Two low cost strain gauges are affixed to each axis drive belt using epoxy. The gauges are interconnected in a wheatstone half bridge configuration with a poisson gauge. This configuration converts the deformation of the belt as it stretches into a small resistance change. The Wheatstone bridge converts this change in resistance to a voltage change while compensating for the thermal expansion of the material. The wheatstone bridge is connected to an HX711 ADC instrumentation amplifier which amplifies, filters, samples, and quantizes the signal to be interpreted by the microcontroller. The microcontroller converts the voltage value into a force measurement using experimentally determined voltage to force ratios. If the force (either positive or negative) on any axis exceeds the specified threshold, a low signal is sent to the CNC control board which interprets this as hitting a limit switch and aborts the program. 

Files:
Config.yaml: When uploaded to the CNC controller, configures GPIO 39 as input pin for the force limit switch. 

Materials:
2 strain gauges, wheatstone half bridge, poisson gauge, HX711 ADC instrumentation amplifier, microcontroller, CNC machine.

members:
Logan Clancy: 98795826,
Hugo Dejong: 87066320,
Havish Moturi: 81610586,
Adam Parsons: 31464143,
Trinity Bell: 55076178
Due April 9, 2026
