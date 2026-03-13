# Lab1_hw: STM32 RTOS LED Control

This project is an STM32 practice repository focused on Real-Time Operating System (RTOS) concepts. It demonstrates multi-tasking, interrupt handling, and inter-task communication using Semaphores, Mutexes, and Message Queues to manage and protect shared hardware resources.

## System Architecture & Resource Protection

The system consists of two primary tasks (`Task_1` and `Task_2`), both attempting to control the same hardware resource (`LED2`). 
To prevent interference and ensure smooth blinking procedures, a **Mutex / Semaphore** is implemented. This guarantees that only one task can acquire control of the LED at any given time, protecting the blinking procedure from interruption.

---

## 🟢 Basic Problem

### Task_1: Button-Triggered Control
* **Trigger Mechanism:** External Interrupt (EXTI).
* **Communication:** The EXTI interrupt handler detects a button press and releases a **binary semaphore** to wake up Task_1.
* **Execution:** Upon acquiring the semaphore, Task_1 blinks LED2 continuously for **5 seconds at a 1Hz frequency**.

### Task_2: Timer-Triggered Control
* **Trigger Mechanism:** Hardware Timer.
* **Communication:** The timer is configured to trigger periodically every 10 seconds. The Timer ISR detects the time-up event and releases another **binary semaphore** to inform Task_2.
* **Execution:** Upon acquiring the semaphore, Task_2 blinks LED2 continuously for **2 seconds at a 10Hz frequency**.

---

## 🔴 Option Problem_A (Advanced Feature)

In this advanced implementation, the button-triggered mechanism for `Task_1` is upgraded to distinguish between "Normal Press" and "Long Press" events.

### Duration Detection & Message Queue
* **Long Press Condition:** The button is pressed and held for **over 1 second**.
* **Communication Change:** Since Task_1 now needs to handle distinct types of events, the binary semaphore is replaced with a **Message Queue**. The ISR identifies the press duration and sends distinguishable messages to the queue.
* **Task_1 Execution (Based on the received message):**
  * **Normal Press:** LED2 blinks continuously for **5 seconds at a 1Hz frequency**.
  * **Long Press:** LED2 blinks continuously for **5 seconds at a 10Hz frequency**.