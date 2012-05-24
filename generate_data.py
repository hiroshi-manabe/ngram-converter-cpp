#!/usr/bin/python
# -*- coding: utf-8 -*-

# built-ins
import getopt
import os
import struct
import sys

BLOCK_SIZE = 128

def Usage():
    print sys.argv[0] + ' - generate N-gram data from a sorted N-gram file.'
    print 'Usage: '  + sys.argv[0] + \
        '-i <input filename> ' + \
        '-o <output filename> ' + \
        '-s <min score> '
    sys.exit()


class BitArray(object):
    def __init__(self):
        self._bytearray = bytearray()
        self._length = 0

    def AddBits(self, bit_seq):
        for b in bit_seq:
            self._length += 1
            if self._length % 8 == 1:
                self._bytearray.append(b << 7)
            else:
                self._bytearray[-1] |= b << (8 - ((self._length + 7) % 8 + 1))

    def GetByteArray(self):
        return self._bytearray


def EncodeNumber(x):
    b = bytearray()
    if x < (1 << 7):
        b.append((1 << 7) | x)
    elif x < (1 << 14):
        b.append((1 << 6) | (x >> 8))
        b.append(x & 0xff)
    elif x < (1 << 21):
        b.append((1 << 5) | (x >> 16))
        b.append((x >> 8) & 0xff)
        b.append(x & 0xff)
    elif x < (1 << 28):
        b.append((1 << 4) | (x >> 24))
        b.append((x >> 16) & 0xff)
        b.append((x >> 8) & 0xff)
        b.append(x & 0xff)

    return b


class NgramBlock(object):
    def __init__(self, max_size):
        self._store_token_data = False
        self._store_backoff_data = False

        self._score_data = bytearray(max_size)
        self._backoff_bits = BitArray()
        self._backoff_data = bytearray()

        self._token_data = BitArray()
        self._context_data = bytearray()

        self._size = 0
        self._max_size = max_size

        self._first_token_id = None
        self._first_context_id = None
        self._prev_token_id = None
        self._prev_context_id = None

    def AddNgram(self, token_id, context_id, score_int, backoff_int):
        if self._size >= self._max_size:
            raise ValueError

        self._score_data[self._size] = score_int

        has_backoff = backoff_int != -1
        self._backoff_bits.AddBits((has_backoff,))
        if has_backoff:
            self._backoff_data.append(backoff_int)

        self._size += 1

        if self._first_token_id is None:
            self._first_token_id = self._prev_token_id = token_id
            self._first_context_id = self._prev_context_id = context_id
            return self._size >= self._max_size

        if token_id != self._prev_token_id:
            self._store_token_data = True

        self._token_data.AddBits([0] * (token_id - self._prev_token_id) + [1])
        self._context_data += EncodeNumber(
            context_id - (self._prev_context_id if
                          self._prev_token_id == token_id else 0))
        return self._size >= self._max_size

    def GetFirstIds(self):
        return (self._first_token_id, self._first_context_id)

    def FlushData(self):
        data = bytearray()
        data.append(self._store_backoff_data << 1 | self._store_token_data)

        data += self._score_data

        if self._store_backoff_data:
            data += self._backoff_bits.GetByteArray()
            data += self._backoff_data

        data += self._token_data.GetByteArray()
        data += self._context_data

        self.__init__(self._max_size)
        return data
        

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'hi:o:s:')
    except getopt.GetoptError:
        Usage()
        sys.exit(2)

    input_filename = ''
    output_filename_prefix = ''
    min_score = 0.0

    for k, v in opts:
        if k == '-h':
            usage()
            sys.exit()
        elif k == '-i':
            input_filename = v
        elif k == '-o':
            output_filename_prefix = v
        elif k == '-s':
            min_score = float(v)

    if input_filename == '' or output_filename_prefix == '' or not min_score:
        Usage()

    f_in = open(input_filename, 'r')
    f_out_index = open(output_filename_prefix + '.index', 'wb')
    f_out_data = open(output_filename_prefix + '.data', 'wb')

    cur_n = 0
    cur_ngram_count = 0
    data_offset = 0
    ngram_block = NgramBlock(BLOCK_SIZE)

    for line in f_in:
        line = line.rstrip('\n')
        if not line:
            continue

        fields = line.split('\t')
        if fields[0][0] == '#':
            new_n = int(fields[0][1:])
            if new_n != cur_n + 1:
                raise ValueError

            cur_n = new_n
            cur_ngram_count = int(fields[1])

            f_out_index.write(struct.pack('L', cur_ngram_count))
            
            continue

        (id, token_id, context_id) = (int(x) for x in fields[:3])
        (score, backoff) = (float(x) for x in fields[3:5])

        if cur_n == 1 and backoff == -99.0:
            backoff = 0.0

        (score_int, backoff_int) = (int(x / min_score * 255) for x
                                    in (score, backoff))

        backoff_int += 128

        if score_int < 0:
            score_int = 0
        if score_int > 255:
            score_int = 255
        if backoff_int < 0:
            backoff_int = 0
        if backoff_int > 255:
            backoff_int = 255

        if cur_n != 1 and backoff == -99.0: # does not exist
            backoff_int = -1

        if cur_n == 1:
            f_out_index.write(struct.pack('BB', score_int, backoff_int))

        else: # cur_n != 1
            is_full = ngram_block.AddNgram(token_id, context_id,
                                           score_int, backoff_int)
            if is_full:
                (first_token_id, first_context_id) = ngram_block.GetFirstIds()
                f_out_index.write(struct.pack('LLL', first_token_id,
                                              first_context_id, data_offset))

                data = ngram_block.FlushData()
                f_out_data.write(data)
                data_offset += len(data)

    f_in.close()
    f_out_index.close()
    f_out_data.close()
    print 'Done.'


if __name__ == '__main__':
    main()
