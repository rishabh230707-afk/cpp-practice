# ERL-HUB — submission guide

**Developed by Rishabh Bansal**  
B.Tech CSE, Second Year · MAIT, Delhi  
CIC-211: Object-Oriented Programming · Option 2

## Run the project

Open a terminal in this folder:

```sh
make
./build/erl-hub
```

For the sample scenario, use a new filename:

```sh
./build/erl-hub --demo --data presentation.bin
```

The demo queues two missions. The priority-5 medical mission leaves first. Its road is blocked, so the amphibious medical unit takes the water route. The ration mission follows. The total dispatched cargo is 0.11 tonnes.

Restart with the same data file to see the saved records:

```sh
./build/erl-hub --data presentation.bin
```

Choose **6** for the audit log, **2** for vehicle fuel and **8** for statistics.

## Files to read

| File | Contents |
|---|---|
| docs/PROJECT_REPORT.md | Problem, design, algorithms, results and limitations |
| docs/REQUIREMENTS.md | Every required syllabus feature and its code location |
| docs/TEST_REPORT.md | Test results and verification limits |
| docs/VIVA.md | Short explanations and a demonstration sequence |
| README.md | Full build instructions and menu guide |

The `include/erl/` and `src/` folders contain the source code. The `tests/` folder contains the regression suite, and `examples/` contains captured output.

## Before uploading

- Run `make test`; the current suite contains 45 checks.
- Confirm the form's required student details and upload format.
- Upload `ERL-HUB_Submission.zip` and include the project link below.

Project: https://github.com/rishabh230707-afk/cpp-practice/tree/main/ERL-HUB

Submission deadline: **27 November 2026, 11:59 PM**.

The ZIP contains source code and documentation. It excludes compiled executables and live data files. Reports are in Markdown and can be viewed directly on GitHub.
