## How can you make code more robust and safe?

### Design

- For critical parts of the software: Do a Failure Mode and Effects Analysis (FMEA) upfront https://de.wikipedia.org/wiki/FMEA
- Plan and Design countermeasures for intolerable risks
- Separate critical parts from non-critical parts, ensure "freedom from interference"
- Leave a margin for errors (e.g. timing, precision of input values)
- Introduce mechanisms for runtime error detection, e.g. watchdogs, CRC checks
- Introduce redundancy for systems that have to deal with non-systematic errors

### Software Development

- Choose the proper tools (e.g. programming language) for any given task
- Set the compiler warnings to a reasonable level and dont ignore warnings (ideally have no warnings at all)
- Consider re-use of software that is already proven in use

### QA

- Set up a Software Test plan upfront, for critical parts of the software
- Ensure that the Requirement Specification is complete and broken down from System Requirements into Module Requirements
- Ensure that Requirements Tracing is set up. For every requirement, there is a combination of detailing requirements, design items, or tests
- For critical parts of the software, use code inspections (formal code reviews)
- Extract metrics out of the automated tests, e.g. test coverage, cyclomatic complexity of functions
- Ensure all code that enters production branches in the version control system was reviewed
- Establish coding guidelines, use checker tools to ensure they are followed
- Use static and dynamic code checkers. Static code checkers may generate lots of false-positive warnigns. Find an automated way to filter out as much false-positives as possible, so that only a few warnings are left to check manually

### HR
- Ensure the involved personnel has the proper skills
