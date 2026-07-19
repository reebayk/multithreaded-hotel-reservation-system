We are Group 11 Project 25 members: 
Ammar Aamir (SP24-BSCS-0081)
Sakina Murtaza (SP24-BSCS-0098)
Areeba Kalwar (SP24-BSCS-0099). 

Our project is a Multithreaded Hotel Reservation System with overbooking detection developed in C. The system handles multiple client requests like booking, cancellation, availability checking, and check-in while using mutexes and semaphores to avoid race conditions and incorrect updates, it also includes a broken version as well along with fixed version and stress testing. To build and run the project, use `make all` followed by the required commands like `make demo-bug`, `make demo-fix`, `make stress`, and `make verify`.


| File | Description |
|------|-------------|
| `hotel_broken.c` | Intentionally broken — 3 race condition bugs |
| `hotel_fixed.c` | Fixed using mutex + semaphore |
| `hotel_stress.c` | Stress test with invariant checker |
| `Makefile` | Build and run all targets |
| `results/all.csv` | Stress test results (configs A–D) |
| `results/helgrind_broken.log` | Helgrind output on broken version |
| `results/helgrind_fixed.log` | Helgrind output on fixed version |
| `results/tsan_stress.log` | ThreadSanitizer output |
| `report.pdf` | Full seven-section report |
