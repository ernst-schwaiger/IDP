## Define the differences between safety, reliability, and robustness

### Safety

“The condition of being protected from or unlikely to cause danger, risk, or injury.”

“Freedom from unacceptable risk.” (e.g. IEC 61508)

Exanple Applications
- Occupational Safety: Preventing workplace injuries and illnesses
- Functional Safety: Ensuring systems operate correctly under failure
- Process Safety: Managing hazards in industrial processes
- Cyber Safety: Protecting systems from digital threats

### Reliability

“The probability that a product, system, or service will perform its intended function adequately for a specified period of time, under defined conditions, without failure.”

The most important components of this definition must be clearly understood to fully know how reliability in a product or service is established:

- Probability: the likelihood of mission success
- Intended function: for example, to light, cut, rotate, or heat
- Satisfactory: perform according to a specification, with an acceptable degree of compliance
- Specific period of time: minutes, days, months, or number of cycles
- Specified conditions: for example, temperature, speed, or pressure

Source: https://asq.org/quality-resources/reliability

### Robustness

“The degree to which a system or component can function correctly in the presence of invalid inputs or stressful environmental conditions.”
(IEEE Standard Glossary of Software Engineering Terminology, IEEE Std 610.12-1990 )
https://en.wikipedia.org/wiki/Robustness_%28computer_science%29

Core aspects of robustness are:

- Error tolerance: The system continues to operate even when faced with unexpected or incorrect inputs.
- Environmental resilience: It performs reliably under stress, such as temperature extremes, network failures, or hardware faults.
- Graceful degradation: Instead of crashing, a robust system may reduce functionality while maintaining core operations.

Robustness is often tested through techniques like fault injection, stress testing, or fuzz testing, which simulate adverse conditions to evaluate system behavior.
