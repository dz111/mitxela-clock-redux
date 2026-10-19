#!/usr/bin/env python3

import datetime
import calendar
from pathlib import Path

START = 2020
END   = 2060

epoch = datetime.datetime(2020, 1, 1, 0, 0, 0, tzinfo=datetime.timezone.utc)
aest_tz = datetime.timezone(datetime.timedelta(hours=10))

filename = "build/time_offset.h"

prolog = """
#pragma once

#ifndef PROGMEM
#define PROGMEM
#endif

static const TzOffset timezone_offsets[] PROGMEM = {
"""

epilog = "};\n"

def get_first_sunday(year, month):
    day = 1
    while True:
        dt = datetime.datetime(year, month, day, 2, 0, 0, tzinfo=aest_tz)
        if dt.weekday() == 6:
            return dt
        day += 1

def dt2sec(dt):
    dt_utc = dt.astimezone(datetime.timezone.utc)
    return int((dt_utc - epoch).total_seconds())

def main():
    path = Path(filename)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('w') as fh:
        fh.write(prolog)
        fh.write("  {0, 660, 1},\n")
        for year in range(START, END):
            fh.write(f"  {{{dt2sec(get_first_sunday(year, 4))}, 600, 0}},\n")
            fh.write(f"  {{{dt2sec(get_first_sunday(year, 10))}, 660, 1}},\n")
        fh.write(epilog)

if __name__ == '__main__':
    main()
