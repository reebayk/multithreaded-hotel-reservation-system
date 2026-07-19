# 🏨 Multithreaded Hotel Reservation System

![C](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white)
![Linux](https://img.shields.io/badge/Linux-FCC624?style=for-the-badge&logo=linux&logoColor=black)
![Git](https://img.shields.io/badge/Git-F05032?style=for-the-badge&logo=git&logoColor=white)
![GitHub](https://img.shields.io/badge/GitHub-181717?style=for-the-badge&logo=github&logoColor=white)
![Markdown](https://img.shields.io/badge/Markdown-000000?style=for-the-badge&logo=markdown&logoColor=white)
![GNU](https://img.shields.io/badge/GNU-A42E2B?style=for-the-badge&logo=gnu&logoColor=white)

Thread-safe hotel reservation system built in **C** using **POSIX Threads**, demonstrating synchronization, race condition detection, overbooking prevention, and concurrency stress testing.

---

## 🎥 Demo

> *(Embed a GIF here after you create one.)*

![Demo](screenshots/demo.gif)

---

## 📸 Screenshots

| Broken Version | Fixed Version |
|---------------|---------------|
| ![](screenshots/demo-bug.png) | ![](screenshots/demo-fix.png) |

| Helgrind | ThreadSanitizer |
|-------------|-----------------|
| ![](screenshots/helgrind.png) | ![](screenshots/tsan.png) |

### 📈 Stress Test Results

<p align="center">
  <img src="screenshots/stress-results.png" alt="Stress Test Results" width="95%">
</p>

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
- Concurrent Programming

---

## 🏗️ System Architecture

```text
                  +------------------+
                  |  Client Threads  |
                  +------------------+
                            |
         +-----------+-----------+-----------+
         |           |           |           |      
      Booking     Cancel      Query      Check-in
         |           |           |           |
         +-----------+-----------+-----------+
                         |
                  +--------------+
                  | db_mtx Mutex |
                  +--------------+
                         |
        +----------------+----------------+
        |                                 |
+--------------------+         +----------------------+
| Booking Database   |         | Shared Statistics    |
| • Rooms            |         | • Bookings           |
| • Categories       |         | • Revenue            |
| • Date Ranges      |         | • Check-ins          |
| • Active Bookings  |         | • Cancellations      |
+--------------------+         +----------------------+
        |                                 |
        +----------------+----------------+
                         |
                  +--------------+
                  | Auditor      |
                  | Thread       |
                  +--------------+
                         |
         +---------------+----------------+
         |                                |
  Overbooking Detection      Revenue Verification
```

---

## 📂 Project Structure

```text
.
├── .git/
├── results/
│   ├── all.csv
│   ├── broken_runs.log
│   ├── helgrind_broken.log
│   ├── helgrind_fixed.log
│   ├── tsan_stress.log
│   ├── SA.csv
│   ├── SB.csv
│   ├── SC.csv
│   ├── SD.csv
│   └── SE.csv
├── screenshots/
│   ├── demo.gif
│   ├── demo-bug.png
│   ├── demo-fix.png
│   ├── stress.png
│   ├── helgrind.png
│   ├── tsan.png
│   └── throughput.png
├── hotel_broken.c
├── hotel_fixed.c
├── hotel_stress.c
├── fixedbackup.c
├── Makefile
├── README.md
└── Report.pdf
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

### 📈 Throughput Scaling

<p align="center">
  <img src="screenshots/throughput.png" alt="Throughput Scaling Graph" width="800">
</p>

*The graph shows that throughput improves with more rooms but decreases as client load increases due to higher contention and linear conflict checking. All configurations completed with **0 audit violations**.*
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

<p align="center">
  <img src="screenshots/maju-logo.png" alt="Mohammad Ali Jinnah University Logo" width="90">
</p>

<p align="center">
  <strong>Mohammad Ali Jinnah University</strong><br>
  Operating Systems Semester Project
</p>
---

## ⭐ Acknowledgements

This project was developed as part of the Operating Systems course to explore multithreading, synchronization, race condition detection, and concurrent systems programming.
