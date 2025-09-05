## Define the following terms: fault, error, failure, hazard


### Fault "Fehlerursache"
- Is the cause for an Error
- Active Faults are causing Errors in Systems
- Dormant Faults did not yet cause Errors (they have not yet been activated)
- External Faults: Faults introduced to the system from the outside (incorrect usage of a system, cosmic radiation)
- Internal Faults: Faults that exist within the system (e.g. cold solder joint on a pcb)
- Systematic Faults: Programming Bug
- Non Systematic Faults: Stochastic Faults, wear & tear, ...

### Error "Fehlerzustand"

- Internal state of a system that deviates from the intended state
- An error occurs only when the system is active
- An error may cause subsequent errors "error propagation"
- If an error reaches the boundary of a system, it causes a failure

### Failure

- Deviation of a systems behavior from its intended (specified) behavior
- Manifests itself at the boundary  of a system
- Failures are caused by errors
- Failure can manifest themselves in the value or timing range


### Hazard

“A process, phenomenon or human activity that may cause loss of life, injury or other health impacts, property damage, social and economic disruption or environmental degradation.”
source: https://www.undrr.org/publication/documents-and-publications/hazard-definition-and-classification-review-technical-report

Hazards can be classified according to

- its potential (likelyhood of occurrence, severity of the consequences, exposure of the environment to the hazard, vulnerability of the affected environment)
- its trigger, e.g. human, natural
- its damage: loss of human life, loss of money, ...