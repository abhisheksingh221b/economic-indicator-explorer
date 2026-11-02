# Economic Indicator Explorer

A console program that stores World Bank country data in a **hash table** and **AVL trees**, and answers five kinds of questions: lookup, range filter, top-k ranking, early warning (green / yellow / red) and peer finder.

Data: World Bank World Development Indicators (WDI), year **2023**, 217 countries.

---

## 1. How to run

```bash
make          # compile  (needs g++ 9 or newer)
make run      # start the program
make test     # run the automatic self-test (18 checks)
```

Without `make`:

```bash
g++ -std=c++17 -O2 -o explorer src/main.cpp
./explorer data/countries_2023.csv
```

(On Windows use MinGW or WSL, then `explorer.exe data\countries_2023.csv`.)

Colours: **green** = fine, **yellow** = watch, **red** = serious, **grey** = no data (the 4th colour, see 4.2). Every colour is also printed as a word (`[RED]`), so nothing depends on colour alone.

---

## 2. Files and why each exists

| File | Job | Why it is separate |
|---|---|---|
| `prepare_data.py` | Turns the 200 MB World Bank file into a 217-line file | Cleaning in Python takes 40 lines; in C++ it would take several hundred |
| `data/countries_2023.csv` | The cleaned data (already made for you) | The program starts in under a second |
| `src/Country.h` | `Country` record, indicator names, colour rules | Every other file needs it |
| `src/HashTable.h` | Find a country by name | One data structure per file |
| `src/AVLTree.h` | Sorted, self-balancing tree | One data structure per file |
| `src/QueryEngine.h` | Owns the hash table and the 5 trees; answers questions | Logic without any printing, so it can be tested |
| `src/Menu.h` | Everything typed and printed | Screen code kept apart from logic |
| `src/Loader.h` | Reads the data file | Shared by the program and the test |
| `src/main.cpp` | Starts everything | About 40 lines |
| `tests/test_all.cpp` | Self-test | Proves the answers are right |

This matches the class diagram: `Menu`, `QueryEngine`, `HashTable`, `AVLTree`, `Node`, `Country`.

---

## 3. Step by step: what happens when you start it

1. **Read the file** (`Loader.h`) into a `std::vector<Country>`. Every country is created **once**.
   *Why:* one copy of the data means a correction is made in one place, and memory use stays tiny.
2. **Build the indexes** (`QueryEngine` constructor). For every country:
   - it goes into the hash table;
   - for each of the five indicators that has a value, it goes into that indicator's AVL tree.
   *Why two kinds of structure:* a hash table is the fastest way to find **one** name, but it has no order, so it cannot answer "between 3 and 6". A sorted tree can. Each is used for what it is best at.
3. **Show the menu** and repeat until you choose 0.

The vector is completely filled **before** the indexes are built, because the indexes store pointers into it. A vector that is still growing can move its contents and make those pointers invalid.

---

## 4. Decisions and the reason for each

### 4.1 Data preparation (`prepare_data.py`)
- **Removing aggregates.** The World Bank file contains rows such as "World" and "High income". The country table gives a *Region* only to real countries, so a blank Region means "not a country". That is why 217 rows remain.
- **Reading line by line.** The file is about 200 MB. Reading one line at a time keeps memory use tiny.
- **Year 2023.** It is the latest year with good coverage: GDP 204 countries, inflation 177, unemployment 184. 2024 has fewer countries, and 2025 has no debt data. Change the year with `python3 prepare_data.py raw/WDICSV.csv raw/WDICountry.csv 2022 data/countries_2022.csv`.
- **`;` as separator.** World Bank names such as "Korea, Rep." contain commas, which would break a comma-separated file.

### 4.2 Missing values and the 4th colour
- Empty cells are stored as the number `-9999` (`MISSING`).
- **Not `-1`**, because -1 is a real value: inflation of -1 % or growth of -1 % happens. `-9999` cannot be real.
- Missing values are **never put in the trees**, so they can never appear in a range, ranking or warning.
- Grey is the 4th colour because "no data" is not good and not bad; showing it as green would be misleading.

### 4.3 Hash table (`HashTable.h`)
- **Hash function:** polynomial, `h = h*31 + letter`, on the lower-case name. Order of letters matters, so "Niger" and "Nigeria" land far apart.
- **Separate chaining:** each bucket is a list; two names in the same bucket just share the list. It is the simplest collision method and never fills up.
- **Starts at 101 buckets and grows by itself:** when it is more than 75 % full, it builds a table about twice as large (next prime number) and re-inserts everything. Loading 217 countries makes it grow twice. You never have to guess a size.
- **Case-insensitive:** `india` finds `India`.

### 4.4 AVL tree (`AVLTree.h`)
- **Balance:** after every insert, each node on the way back up is checked. If its two sides differ in height by more than 1, a rotation fixes it. Height stays about log2(n), so searches stay fast even if the data arrives sorted. The test proves this with 5,000 sorted inserts.
- **Height convention:** a leaf has height **0** and "no node" has -1 (height counts edges).
- **Ties:** the order is *(value, then country name)*. Two countries with equal inflation are both kept; without this the second would be lost.
- **Range query:** goes left only if the node's value >= low, right only if <= high. It skips every part of the tree that cannot contain answers: O(log n + k).
- **Top-k:** walks the tree from the biggest side (or smallest side) and stops after k results.

### 4.5 Early warning
- Uses **range queries on the tree**: red = one range query, yellow = another, green is the rest counted by subtraction. It does not scan all countries.
- Default lines (rules of thumb, **not official World Bank limits**; you can change them while running):

| Indicator | Yellow | Red | Why |
|---|---|---|---|
| Inflation | 6 % or more | 10 % or more | Above ~2-3 % is unusual; 10 % hurts savings |
| Unemployment | 6 % or more | 8 % or more | Normal range is about 4-6 % |
| Govt debt | 60 % of GDP | 90 % of GDP | 60 % is the euro-area rule; 90 % is a common high-debt marker |
| GDP growth | below 2 % | below 0 % | Negative growth is a contraction |

- GDP per capita has **no colour**: being rich or poor is not "good" or "bad".
- **Combined check (option 6):** lists countries that are red on two or more indicators. This is the stronger warning.
- Green countries are **counted, not listed**, because the list would be 100+ lines of "fine".

### 4.5b Peer finder
1. Find the country in the hash table.
2. Range query on the **GDP tree**: GDP per capita within +-25 %.
3. Keep those with inflation within +-2 points.
4. Rank by `distance = |GDP gap| / target GDP + |inflation gap| / 2`. Inside the filter both parts are between 0 and 1, so neither dominates. Smaller = more similar.

### 4.6 What was left out, as you asked
- No benchmark and no structure statistics (tree height, bucket counts and so on).
- The only structure-related code is `isBalanced()` in `AVLTree.h`. It is used **only** by `make test` and is never called from the menu.

### 4.7 Making the code easy
- Every file starts with a "WHAT / WHY" comment.
- No templates, no inheritance, no smart pointers; plain `vector`, `list` and one `struct` per idea.
- Input is checked (`readInt`, `readDouble`): wrong input asks again and never crashes.

---

## 5. How it was checked

`make test` compares every feature with a slow but obviously correct version (plain loops over all countries):

- hash table finds all 217 countries in exact, UPPER and lower case; unknown names are rejected;
- 1,500 random range queries equal brute force, including ties and edges;
- top-k equals a sorted list for all indicators, several k and both directions;
- early warning counts and order equal brute force; so do the combined check and the peer finder (for every country);
- AVL tree stays balanced for 5,000 ascending, 5,000 random and 5,000 equal keys, and no country is lost when all keys are equal.

Also done: the code was run under AddressSanitizer and UndefinedBehaviorSanitizer (no errors), and all 1,085 values in `countries_2023.csv` were compared with the original World Bank file (0 differences).

---

## 6. Limits to state honestly

- It is a **screening tool**: a red flag means "look closer", not "a crisis will happen".
- One year only (2023). No live data.
- **Debt data exists for only 40 countries**, because many governments do not report it to the World Bank; the combined check can therefore miss countries.
- The cut-off lines are conventions, not official limits.
- There are no insert or delete options in the menu; the data is loaded once.

---

## 7. Differences from the report (update these so the report matches)

1. **`remove()` is not implemented** in `HashTable` or `AVLTree`, because the data never changes. Delete it from Fig 2.4, or tell me and I will add it.
2. **`Node` has `rec: Country*`**, which the class diagram currently shows only as an arrow. Add it as an attribute.
3. **Height:** leaf = 0.
4. **Missing values** are stored as -9999 (the report says -1).
5. **Peer finder distance** is defined in 4.5b; Equation (2.1) was removed from the report.
6. **Hash table starts at 101 buckets**, not 211, and grows by itself.
7. **Data year is 2023**, and the data file uses `;` as separator.
8. **Missing debt:** 40 countries only.
