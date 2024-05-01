#!/usr/bin/env python3

import csv
from operator import itemgetter

def read_renderdoc_csv(filename):
    data = []
    with open(filename, newline='') as csvfile:
        reader = csv.reader(csvfile, delimiter=' ')
        next(reader)
        for row in reader:
            fixedRow = []
            for r in row:
                fixedr = r.replace(",","")
                if r != '':
                    fixedRow.append(int(fixedr))
            data.append(fixedRow)
    return data[1:]

drawcommands = read_renderdoc_csv('drawcommands.csv')
culleddrawcommands = read_renderdoc_csv('culleddrawcommands.csv')

for draw in drawcommands:
    drawmap[draw[3]] = draw

for draw in culleddrawcommands:
    cullmap[draw[3]] = draw

for draw in drawmap:
    if draw[2] != cullmap[draw[3]][2]:
        print("Draws differ: ")
        print(draw)
        print(cullmap[draw[3]])
