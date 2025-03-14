# Advanced-Fire-Sensing-and-Mitigation-System
The Advanced Fire Sensing and Mitigation System (AFSMS) predicts fire risk and spread in dense urban areas using rain, wind, temperature, and humidity sensors. Inspired by forest fire monitoring systems like the Fire Weather Index (FWI), AFSMS enhances early detection and mitigation efforts to improve urban fire safety.

# Reasons for implementation:
## FreeRTOS:
### Task Notifications:
Tradition approach to inter-task communication cons:
*Using a binary semaphore to signal a task: Uses more memory (stored in kernal data structures).

### Software Timer:
*Memory: No task stack required
*CPU: Runs only when needed
*Precision: More accurate timing
*Best use: Simple periodic actions
Used to blink LEDs indicating critical level

### Task(vTaskDelay())
*Memory: Comsumes stack space
*CPU: Blocks task execution
*Precision: Less precise than Software Timer
*Best use: Complex tasks
Sensor readings display and FWI calculations



