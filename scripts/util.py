#!/usr/bin/env python
# coding: utf-8

import codecs

def ReadFileToTupleSet(filename, tuple_len):
    result = set()
    fp_in = codecs.open(filename, 'r', 'utf-8')

    for line in fp_in:
        line = line.rstrip('\n')
        fields = line.split('\t')
        if len(fields) != tuple_len:
            raise ValueError

        if tuple_len == 1:
            result.add(fields[0])
        else:
            result.add(tuple(fields))

    fp_in.close()
    return result

def ReadFileToDict(filename, first_len, second_len):
    result = {}
    fp_in = codecs.open(filename, 'r', 'utf-8')

    for line in fp_in:
        line = line.rstrip('\n')
        fields = line.split('\t')
        if len(fields) != first_len + second_len:
            raise ValueError

        key = fields[0] if first_len == 1 else tuple(fields[:first_len])
        value = fields[first_len] if second_len == 1 else tuple(
            fields[first_len:])

        result[key] = value

    fp_in.close()
    return result

