## What is fault tolerance and how can it be achieved?

### Definition

"Fault tolerance is the ability of a system to maintain proper operation despite failures or faults in one or more of its components. This capability is essential for high-availability, mission-critical, or even life-critical systems."

https://en.wikipedia.org/wiki/Fault_tolerance

### How To Achieve

- Introduce Hardware Redundancy: Duplicate components (hardware or software) are built into the system so that if one fails, another can take over instantly.
- Add Standby Systems: Automatic switching to backup systems or components when a fault is detected.
- hot standby systems can be switched to immediately
- cold standby systems must startup before replacing the failed system
- Allow a system to degrade gracefully: The system may reduce functionality but still operate, rather than crashing entirely.
- In software, or hardware, introduce Error Detection and Correction:
-- Use software redundancy, e.g. two algorithms to calculate the value a specific parameter and compare the calculated values
-- Employ (self-correcting) CRCs
- Employ recovery mechanisms
-- Backward recovery: Reset the system into a consistent state recorded in the past
-- Forward recovery: Reset the system into a consistent state, e.g. by re-applying transaction logs in database systems
