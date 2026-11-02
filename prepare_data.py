"""
prepare_data.py  -  turns the huge World Bank file into a tiny file the C++ program can read.

Why a separate script?  The World Bank file (WDICSV.csv) is ~200 MB, has 1,300+ rows per
indicator group, and one column for every year.  Cleaning that in C++ would need a lot of
code.  Python's built-in csv module does it in about 40 lines, so the C++ part stays simple.

Usage:   python3 prepare_data.py  raw/WDICSV.csv  raw/WDICountry.csv  2023  data/countries.csv
"""
import csv, sys

raw_file, country_file, year, out_file = sys.argv[1:5]

# The five indicators we need, in the order they will be written to the output file.
INDICATORS = ["NY.GDP.PCAP.CD",      # gdp per capita (US$)
              "NY.GDP.MKTP.KD.ZG",   # gdp growth (%)
              "FP.CPI.TOTL.ZG",      # inflation (%)
              "SL.UEM.TOTL.ZS",      # unemployment (%)
              "GC.DOD.TOTL.GD.ZS"]   # central government debt (% of gdp)

# STEP 1 - find the REAL countries.
# The World Bank file also contains rows like "World", "High income", "Euro area".
# WDICountry.csv gives a 'Region' only to real countries, so a blank Region = aggregate.
real = set()
with open(country_file, encoding="utf-8-sig", newline="") as f:
    for row in csv.DictReader(f):
        if row["Region"].strip() != "":
            real.add(row["Country Code"])

# STEP 2 - read the big file ONE LINE AT A TIME (so memory use stays tiny)
# and keep only: real countries + our 5 indicators + the chosen year.
table = {}          # iso code -> {"name": ..., indicator code: value text}
with open(raw_file, encoding="utf-8-sig", newline="") as f:
    for row in csv.DictReader(f):
        code = row["Indicator Code"]
        iso = row["Country Code"]
        if code in INDICATORS and iso in real:
            entry = table.setdefault(iso, {"name": row["Country Name"]})
            entry[code] = row[year].strip()      # "" means the World Bank has no value

# STEP 3 - write the clean file.  Separator is ';' because names such as
# "Korea, Rep." contain commas.  A missing value stays as an empty cell.
with open(out_file, "w", encoding="utf-8", newline="") as f:
    f.write("name;iso;gdp;growth;inflation;unemployment;debt\n")
    for iso in sorted(table, key=lambda k: table[k]["name"]):
        e = table[iso]
        values = []
        for code in INDICATORS:
            v = e.get(code, "")
            values.append(f"{float(v):.2f}" if v != "" else "")   # round to 2 decimals
        f.write(e["name"] + ";" + iso + ";" + ";".join(values) + "\n")

print("Countries written:", len(table), " Year:", year)
