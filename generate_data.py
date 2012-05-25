#!/usr/bin/python
# -*- coding: utf-8 -*-

# built-ins
import getopt
import os
import struct
import sys

BLOCK_SIZE = 128
MAX_INT_SCORE = 255
BACKOFF_DISCOUNT = 128
FLAGS_HAS_TOKEN_IDS = 1
FLAGS_HAS_BACKOFF = 2

EXT_INDEX = '.index'
EXT_DATA = '.data'

def Usage():
    print sys.argv[0] + ' - generate N-gram data from a sorted N-gram file.'
    print 'Usage: '  + sys.argv[0] + \
        '-i <input filename> ' + \
        '-o <output filename prefix> ' + \
        '-s <min score> ' + \
        '[-v <verification filename>]' + \
        '[-d (only execute verification)]'
    sys.exit()


class BitArray(object):
    def __init__(self):
        self._bytearray = bytearray()
        self._length = 0

    def AddBits(self, bit_seq):
        for b in bit_seq:
            bit = 1 if b else 0

            if self._length % 8 == 0:
                self._bytearray.append(0)

            self._bytearray[-1] |= bit << (self._length % 8)
            self._length += 1

    def IterBits(self):
        for i in range(self._length):
            yield 1 if self._bytearray[int(i / 8)] & (1 << (i % 8)) else 0

    def GetByteArray(self):
        return self._bytearray

    def SetByteArray(self, b_array, length):
        if int(length / 8) != len(b_array):
        self._bytearray = bytearray(array)
        self._length = length


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


def DecodeNumber(b):
    length = 0;
    x = 0
    if b[0] & (1 << 7):
        x = b[0] & ((1 << 7) - 1)
        length = 1
    elif b[0] & (1 << 6):
        x = ((b[0] & ((1 << 6) - 1)) << 8) | b[1]
        length = 2
    elif p[0] & (1 << 5):
        x = ((b[0] & ((1 << 5) - 1)) << 16) | (b[1] << 8) | b[2]
        length = 3
    elif p[0] & (1 << 4):
        x = ((b[0] & ((1 << 4) - 1)) << 24) | (b[1] << 16) | (b[2] << 8) | b[3]

    return (x, len)


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
            self._store_backoff_data = True

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

        if self._size == 0:
            return data

        for i in range(self._max_size - self._size):
            self.AddNgram(self._prev_token_id, self._prev_context_id, 0, 0)

        data.append(self._store_backoff_data * FLAGS_HAS_BACKOFF |
                    self._store_token_data * FLAGS_HAS_TOKEN_IDS)

        data += self._score_data

        if self._store_backoff_data:
            data += self._backoff_bits.GetByteArray()
            data += self._backoff_data

        data += self._token_data.GetByteArray()
        data += self._context_data

        self.__init__(self._max_size)
        return data


def DecodeFiles(output_filename_prefix, verification_filename, block_size, min_score):
    def decode_score(x):
        return x * min_score / MAX_INT_SCORE

    def decode_block(buf, block_size, token_ids, context_ids, scores, backoffs):
        offset = 0
        flags = buf[offset]
        offset += 1

        for i in range(block_size):
            scores[i] = decode_score(buf[offset])
            offset += 1

        if flags & FLAGS_HAS_BACKOFF:
            backoff_bits = BitArray()
            backoff_bits.SetByteArray(buf[offset:offset + int(block_size / 8)],
                                      block_size)
            offset += int(block_size / 8)

            i = 0
            for b in backoff_bits.IterBits():
                if b:
                    backoffs[i] = buf[offset]
                    offset += 1
                else:
                    backoffs[i] = None

        if flags & FLAGS_TOKEN_IDS:
            token_block_len = 0
            token_bits = BitArray()
            token_bits.SetByteArray(buf[offset:], len(buf[offset:]) * 8)

            for i in range(1, block_size):
                count = 0
                bit_len = 0

                for b in token_bits.IterBits():
                    bit_len += 1
                    if b:
                        break
                    count += 1

                token_ids[i] = token_ids[i-1] + count
                token_block_len += count + 1

            offset += int(token_block_len / 8)

        else: # flags & FLAGS_TOKEN_IDS
            token_ids[1:block_size] = [token_ids[0]] * (block_size - 1)

        for i in range(1, block_size):
            (context_id_offset, length) = DecodeNumber(token_ids[offset:])
            context_ids[i] = context_ids[i-1] + context_id_offset
            offset += length

        if offset != len(buf):
            raise ValueError

        return

    f_in_index = open(output_filename_prefix + EXT_INDEX, 'rb')
    f_in_data = open(output_filename_prefix + EXT_DATA, 'rb')
    f_out = open(verification_filename, 'w')

    cur_n = 0
    cur_ngram_count = 0

    while True:
        cur_n += 1
        buf = f_in_index.read(4)
        cur_ngram_count = struct.unpack('=L', buf)
        f_out.write('#%d\t%d' % (cur_n, cur_ngram_count))

        prev_data_offset = 0
        data_offset = None

        token_ids = [0] * block_size
        context_ids = [0] * block_size
        scores_int = [0] * block_size
        scores = [0.0] * block_size
        backoffs_int = [0] * block_size
        backoffs = [None] * block_size

        for i in range(cur_ngram_count):
            if cur_n == 1:
                buf = f_in_index.read(2)
                (score, backoff) = (decode_score(x) for x in buf)
                f_out.write('%d\t%d\t%d\t%f\t%f' % (i, i, 0, score, backoff))

            else: # cur_n != 1
                if i % block_size == 0:
                    buf = f_in_index.read(12)
                    (token_ids[0], context_ids[0], data_offset) = struct.unpack('=LLL', buf)
                    if prev_data_offset != f_in_data.tell():
                        raise ValueError

                    buf = f_in_data.read(data_offset - prev_data_offset)
                    decode_block(buf, block_size, token_ids, context_ids, scores, backoffs)
                    prev_data_offset = data_offset

                f_out.write('%d\t%d\t%d\t%f\t%f' % (i,
                                                    token_ids[i % block_size],
                                                    context_ids[i % block_size],
                                                    scores[i % block_size],
                                                    -99.0 if backoffs[i] is None
                                                    else backoffs[i])

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'hi:o:s:v:')
    except getopt.GetoptError:
        Usage()
        sys.exit(2)

    input_filename = ''
    output_filename_prefix = ''
    verification_filename = ''
    flag_verification_only = False
    min_score = 0.0

    for k, v in opts:
        if k == '-h':
            usage()
            sys.exit()
        elif k == '-d':
            flag_verification_only = True
        elif k == '-i':
            input_filename = v
        elif k == '-o':
            output_filename_prefix = v
        elif k == '-s':
            min_score = float(v)
        elif k == '-v':
            verification_filename = v

    if not output_filename_prefix or not min_score:
        Usage()

    if flag_verification_only:
        if not verification_filename:
            Usage()

        DecodeFiles(output_filename_prefix, verification_filename)
        sys.exit(0)

    if not input_filename:
        Usage()

    f_in = open(input_filename, 'r')
    f_out_index = open(output_filename_prefix + EXT_INDEX, 'wb')
    f_out_data = open(output_filename_prefix + EXT_DATA, 'wb')

    cur_n = 0
    cur_ngram_count = 0
    data_offset = 0
    ngram_block = NgramBlock(BLOCK_SIZE)
    was_full = False

    for line in f_in:
        line = line.rstrip('\n')
        if not line:
            continue

        fields = line.split('\t')

        if fields[0][0] == '#':
            if cur_n == 1:
                if cur_ngram_count % 2 == 1:
                    f_out_index.write(struct.pack('=BB', 0, 0))

            else: # cur_n != 1
                data = ngram_block.FlushData()
                f_out_data.write(data)
                data_offset += len(data)

            new_n = int(fields[0][1:])
            if new_n != cur_n + 1:
                raise ValueError

            cur_n = new_n
            cur_ngram_count = int(fields[1])

            f_out_index.write(struct.pack('=L', cur_ngram_count))
            
            continue

        (id, token_id, context_id) = (int(x) for x in fields[:3])
        (score, backoff) = (float(x) for x in fields[3:5])

        if cur_n == 1 and backoff == -99.0:
            backoff = 0.0

        (score_int, backoff_int) = (int(x / min_score * MAX_SCORE) for x
                                    in (score, backoff))

        backoff_int += BACKOFF_DISCOUNT

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
            f_out_index.write(struct.pack('=BB', score_int, backoff_int))

        else: # cur_n != 1
            is_full = ngram_block.AddNgram(token_id, context_id,
                                           score_int, backoff_int)
            if is_full:
                data_offset += len(data)
                (first_token_id, first_context_id) = ngram_block.GetFirstIds()
                f_out_index.write(struct.pack('=LLL', first_token_id,
                                              first_context_id, data_offset))

                data = ngram_block.FlushData()
                f_out_data.write(data)

    f_in.close()
    f_out_index.close()
    f_out_data.close()
    print 'Done.'

    if verification_filename:
        DecodeFiles(output_filename_prefix, verification_filename)

if __name__ == '__main__':
    main()
