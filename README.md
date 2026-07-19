# 🏨 Multithreaded Hotel Reservation System

![Language](https://img.shields.io/badge/C-Programming-blue)
![POSIX Threads](https://img.shields.io/badge/POSIX-Pthreads-success)
![Platform](https://img.shields.io/badge/Linux-Kali-informational)
![Build](https://img.shields.io/badge/Build-Makefile-orange)
![Thread Safety](https://img.shields.io/badge/Thread--Safe-Yes-brightgreen)
![License](https://img.shields.io/badge/Academic-Project-lightgrey)

Thread-safe hotel reservation system built in **C** using **POSIX Threads**, demonstrating synchronization, race condition detection, overbooking prevention, and concurrency stress testing.

---

## 🎥 Demo

> *(Embed a GIF here after you create one.)*

![Demo](screenshots/demo.gif)

Or include a short screen recording:

https://github.com/yourusername/multithreaded-hotel-reservation-system/assets/...

---

## 📸 Screenshots

| Broken Version | Fixed Version |
|---------------|---------------|
| ![](screenshots/demo-bug.png) | ![](screenshots/demo-fix.png) |

| Stress Test | ThreadSanitizer |
|-------------|-----------------|
| ![](screenshots/stress.png) | ![](screenshots/tsan.png) |

---

## ✨ Features

- Multithreaded client simulation
- POSIX Threads (pthreads)
- Mutex-protected shared database
- Thread-safe statistics
- Room booking by category
- Date-overlap checking
- Revenue consistency verification
- Auditor thread
- Race condition demonstration
- Stress testing
- Helgrind verification
- ThreadSanitizer verification

---

## 🧠 Concepts Demonstrated

- Multithreading
- Mutual Exclusion
- Race Conditions
- Thread Safety
- Critical Sections
- Synchronization
- POSIX Threads
- Mutexes
- Semaphores
- Concurrent Programming

---

## 🏗️ System Architecture

```
        Client Threads
              │
      ┌───────┼────────┐
      │       │        │
  Booking  Cancel  Check-in
      │       │        │
      └───────┼────────┘
              │
         db_mtx (Mutex)
              │
      Booking Database
              │
      Auditor Thread
              │
   Detect Overbooking &
 Verify Revenue Consistency
```

---

## 📂 Project Structure

```
.
├── Makefile
├── hotel_broken.c
├── hotel_fixed.c
├── stress.c
├── auditor.c
├── results/
├── screenshots/
├── README.md
└── report.pdf
```

---

## 🚀 Getting Started

### Clone

```bash
git clone https://github.com/yourusername/multithreaded-hotel-reservation-system.git
cd multithreaded-hotel-reservation-system
```

### Build

```bash
make all
```

---

## ▶️ Run

### Demonstrate Race Conditions

```bash
make demo-bug
```

### Run Fixed Version

```bash
make demo-fix
```

### Stress Test

```bash
make stress
```

### Verify with Helgrind

```bash
make verify
```

### Verify with ThreadSanitizer

```bash
make tsan
```

---

## 📊 Results

| Configuration | Clients | Rooms | Audit Violations |
|--------------|---------|-------|------------------|
| A | 1000 | 10 | 0 |
| B | 1000 | 100 | 0 |
| C | 10000 | 100 | 0 |
| D | 10000 | 100 | 0 |
| E | 100000 | 100 | 0 |

---

## 🔍 Race Conditions Demonstrated

### Broken Version

- Double booking
- Lost updates
- Busy waiting
- Data races

### Fixed Version

- Mutex-protected booking database
- Atomic booking operations
- Thread-safe revenue updates
- Zero audit violations

---

## 🛠️ Technologies

- C
- POSIX Threads
- GCC
- Make
- Valgrind
- Helgrind
- ThreadSanitizer
- Linux (Kali)

---

## 👥 Team

| Roll No. | Name |
|----------|------|
| SP24-BSCS-0081 | Ammar Aamir |
| SP24-BSCS-0098 | Sakina Murtaza |
| SP24-BSCS-0099 | Areeba Kalwar |

---

## 📄 Report

The complete project report is available in:

```
report.pdf
```

---

## 🎓 Course

**Operating Systems**

Semester Project

Mohammad Ali Jinnah University

---

## ⭐ Acknowledgements

This project was developed as part of the Operating Systems course to explore multithreading, synchronization, race condition detection, and concurrent systems programming.
