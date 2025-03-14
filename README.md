# Advanced-Fire-Sensing-and-Mitigation-System
The Advanced Fire Sensing and Mitigation System (AFSMS) predicts fire risk and spread in dense urban areas using rain, wind, temperature, and humidity sensors. Inspired by forest fire monitoring systems like the Fire Weather Index (FWI), AFSMS enhances early detection and mitigation efforts to improve urban fire safety.

# Reasons for implementation:  

# FreeRTOS:
## Task Notifications:
## Tradition approach to inter-task communication cons
* Using a binary semaphore to signal a task: Uses more memory (stored in kernal data structures).
* Using a queue to sned a small piece of data: Adds processing overhead, even for simple signals.
* Using polling(chekcing conditions repeatedly): Wastes CPU time checking for events.

## Software Timer:
* Memory: No task stack required
* CPU: Runs only when needed  
* Precision: More accurate timing  
* Best use: Simple periodic actions like blinking LEDs to indicate critical level  

## Task(vTaskDelay())
* Memory: Comsumes stack space  
* CPU: Blocks task execution  
* Precision: Less precise than Software Timer  
* Best use: Complex tasks like sensor readings display and FWI calculations  

# Fire-Weather Index (FWI)
## Fine Fuel Moisture Code (FFMC)
$FFMC_{t} = \frac{59.5 * (250 - M)}{147.2 + M}$

where:

-   Moisture content: $M = \frac{147.2 * (101 - RH)}{59.5 + T}$

-   $RH$ is relative humidity (%)

-   $T$ is temperature (Celsius)

-   If rain occurs, $FFMC$ is adjusted using an empirical formula.







