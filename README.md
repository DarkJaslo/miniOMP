# miniOMP
Minimal implementation of OpenMP for the Parallel Architectures and Programming (PAP) subject.

This library implements the gcc GOMP functions specified in the code. In short:

- No nested parallel regions
- Support for barrier, critical, critical with name
- Support for single and single with nowait
- Nested tasks and taskgroups, taskwait
- No taskloop or task reductions of any kind

It is of course not intended to be used, but it still is a very useful learning project.