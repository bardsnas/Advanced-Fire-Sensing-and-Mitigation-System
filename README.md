# Advanced-Fire-Sensing-and-Mitigation-System
The Advanced Fire Sensing and Mitigation System (AFSMS) predicts fire risk and spread in dense urban areas using rain, wind, temperature, and humidity sensors. Inspired by forest fire monitoring systems like the Fire Weather Index (FWI), AFSMS enhances early detection and mitigation efforts to improve urban fire safety.

# Reasons for implementation:
## FreeRTOS:
### Task Notifications:
Tradition approach to inter-task communication cons:<br\>
*Using a binary semaphore to signal a task: Uses more memory (stored in kernal data structures).

### Software Timer:
*Memory: No task stack required<br\>
*CPU: Runs only when needed<br\>
*Precision: More accurate timing<br\>
*Best use: Simple periodic actions<br\>
Used to blink LEDs indicating critical level<br\>

### Task(vTaskDelay())
*Memory: Comsumes stack space<br\>
*CPU: Blocks task execution<br\>
*Precision: Less precise than Software Timer<br\>
*Best use: Complex tasks<br\>
Sensor readings display and FWI calculations<br\>



