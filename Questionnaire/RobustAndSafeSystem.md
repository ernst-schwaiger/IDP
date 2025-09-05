## How can a system be developed more robust and safe?

### Redundancy and Fault Tolerance
- Use backup components and failover mechanisms to ensure continuity during failures.
- Example: RAID storage, load balancers, and redundant power supplies.
### Modular and Scalable Architecture
- Design systems using microservices or layered architectures to isolate faults and scale efficiently.
- This helps prevent cascading failures and simplifies maintenance.
### Rigorous Testing and Validation
- Employ unit tests, integration tests, stress tests, and fault injection to simulate real-world conditions.
- Safety-critical systems often require formal verification and certification.
### Security by Design
- Implement authentication, authorization, encryption, and secure coding practices from the ground up.
- A secure system is inherently more robust against malicious disruptions.
### Monitoring and Observability
- Use logging, metrics, and alerting to detect anomalies early and respond quickly.
- Tools like Prometheus, Grafana, and ELK stack are popular in this space.
### Graceful Degradation
- Ensure the system can continue to operate in a reduced capacity when parts fail.
- This avoids total shutdown and maintains essential functionality.
### Human Factors and Usability
- Design interfaces and workflows that reduce user error and support safe operation.

Source: https://fastercapital.com/content/Reliable--Building-a-Reliable-and-Robust-System--Key-Considerations.html
